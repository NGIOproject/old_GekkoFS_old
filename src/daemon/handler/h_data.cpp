/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#include <global/rpc/rpc_types.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <global/rpc/distributor.hpp>
#include <global/chunk_calc_util.hpp>
#include <daemon/main.hpp>
#include <daemon/backend/data/chunk_storage.hpp>


using namespace std;

struct write_chunk_args {
    const std::string* path;
    const char* buf;
    rpc_chnk_id_t chnk_id;
    size_t size;
    off64_t off;
    ABT_eventual eventual;
};

/**
 * Used by an argobots threads. Argument args has the following fields:
 * const std::string* path;
   const char* buf;
   const rpc_chnk_id_t* chnk_id;
   size_t size;
   off64_t off;
   ABT_eventual* eventual;
 * This function is driven by the IO pool. so there is a maximum allowed number of concurrent IO operations per daemon.
 * This function is called by tasklets, as this function cannot be allowed to block.
 * @return written_size<ssize_t> is put into eventual and returned that way
 */
void write_file_abt(void* _arg) {
    // Unpack args
    auto* arg = static_cast<struct write_chunk_args*>(_arg);
    const std::string& path = *(arg->path);

    try {
        ADAFS_DATA->storage()->write_chunk(path, arg->chnk_id,
                arg->buf, arg->size, arg->off, arg->eventual);
    } catch (const std::system_error& serr){
        ADAFS_DATA->spdlogger()->error("{}() Error writing chunk {} of file {}", __func__, arg->chnk_id, path);
        ssize_t wrote = -(serr.code().value());
        ABT_eventual_set(arg->eventual, &wrote, sizeof(ssize_t));
    }

}

struct read_chunk_args {
    const std::string* path;
    char* buf;
    rpc_chnk_id_t chnk_id;
    size_t size;
    off64_t off;
    ABT_eventual eventual;
};

/**
 * Used by an argobots threads. Argument args has the following fields:
 * const std::string* path;
   char* buf;
   const rpc_chnk_id_t* chnk_id;
   size_t size;
   off64_t off;
   ABT_eventual* eventual;
 * This function is driven by the IO pool. so there is a maximum allowed number of concurrent IO operations per daemon.
 * This function is called by tasklets, as this function cannot be allowed to block.
 * @return read_size<ssize_t> is put into eventual and returned that way
 */
void read_file_abt(void* _arg) {
    //unpack args
    auto* arg = static_cast<struct read_chunk_args*>(_arg);
    const std::string& path = *(arg->path);

    try {
        ADAFS_DATA->storage()->read_chunk(path, arg->chnk_id,
                arg->buf, arg->size, arg->off, arg->eventual);
    } catch (const std::system_error& serr){
        ADAFS_DATA->spdlogger()->error("{}() Error reading chunk {} of file {}", __func__, arg->chnk_id, path);
        ssize_t read = -(serr.code().value());
        ABT_eventual_set(arg->eventual, &read, sizeof(ssize_t));
    }
}

/**
 * Free Argobots tasks and eventual constructs in a given vector until max_idx.
 * Nothing is done for a vector if nullptr is given
 * @param abt_tasks
 * @param abt_eventuals
 * @param max_idx
 * @return
 */
void cancel_abt_io(vector<ABT_task>* abt_tasks, vector<ABT_eventual>* abt_eventuals, uint64_t max_idx) {
    if (abt_tasks != nullptr) {
        for (uint64_t i = 0; i < max_idx; i++) {
            ABT_task_cancel(abt_tasks->at(i));
            ABT_task_free(&abt_tasks->at(i));
        }
    }
    if (abt_eventuals != nullptr) {
        for (uint64_t i = 0; i < max_idx; i++) {
            ABT_eventual_reset(abt_eventuals->at(i));
            ABT_eventual_free(&abt_eventuals->at(i));
        }
    }
}


static hg_return_t rpc_srv_write_data(hg_handle_t handle) {
    /*
     * 1. Setup
     */
    rpc_write_data_in_t in{};
    rpc_data_out_t out{};
    hg_bulk_t bulk_handle = nullptr;
    // default out for error
    out.err = EIO;
    out.io_size = 0;
    // Getting some information from margo
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}", __func__, ret);
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);
    ADAFS_DATA->spdlogger()->debug("{}() path: {}, size: {}, offset: {}", __func__,
                                   in.path, bulk_size, in.offset);
    /*
     * 2. Set up buffers for pull bulk transfers
     */
    void* bulk_buf; // buffer for bulk transfer
    vector<char*> bulk_buf_ptrs(in.chunk_n); // buffer-chunk offsets
    // create bulk handle and allocated memory for buffer with buf_sizes information
    ret = margo_bulk_create(mid, 1, nullptr, &in.total_chunk_size, HG_BULK_READWRITE, &bulk_handle);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        return rpc_cleanup_respond(&handle, &in, &out, static_cast<hg_bulk_t*>(nullptr));
    }
    // access the internally allocated memory buffer and put it into buf_ptrs
    uint32_t actual_count;
    ret = margo_bulk_access(bulk_handle, 0, in.total_chunk_size, HG_BULK_READWRITE, 1, &bulk_buf,
                            &in.total_chunk_size, &actual_count);
    if (ret != HG_SUCCESS || actual_count != 1) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to access allocated buffer from bulk handle", __func__);
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto const host_id = in.host_id;
    auto const host_size = in.host_size;
    SimpleHashDistributor distributor(host_id, host_size);

    auto path = make_shared<string>(in.path);
    // chnk_ids used by this host
    vector<uint64_t> chnk_ids_host(in.chunk_n);
    // counter to track how many chunks have been assigned
    auto chnk_id_curr = static_cast<uint64_t>(0);
    // chnk sizes per chunk for this host
    vector<uint64_t> chnk_sizes(in.chunk_n);
    // how much size is left to assign chunks for writing
    auto chnk_size_left_host = in.total_chunk_size;
    // temporary traveling pointer
    auto chnk_ptr = static_cast<char*>(bulk_buf);
    /*
     * consider the following cases:
     * 1. Very first chunk has offset or not and is serviced by this node
     * 2. If offset, will still be only 1 chunk written (small IO): (offset + bulk_size <= CHUNKSIZE) ? bulk_size
     * 3. If no offset, will only be 1 chunk written (small IO): (bulk_size <= CHUNKSIZE) ? bulk_size
     * 4. Chunks between start and end chunk have size of the CHUNKSIZE
     * 5. Last chunk (if multiple chunks are written): Don't write CHUNKSIZE but chnk_size_left for this destination
     *    Last chunk can also happen if only one chunk is written. This is covered by 2 and 3.
     */
    // temporary variables
    auto transfer_size = (bulk_size <= CHUNKSIZE) ? bulk_size : CHUNKSIZE;
    uint64_t origin_offset;
    uint64_t local_offset;
    // task structures for async writing
    vector<ABT_task> abt_tasks(in.chunk_n);
    vector<ABT_eventual> task_eventuals(in.chunk_n);
    vector<struct write_chunk_args> task_args(in.chunk_n);
    /*
     * 3. Calculate chunk sizes that correspond to this host, transfer data, and start tasks to write to disk
     */
    // Start to look for a chunk that hashes to this host with the first chunk in the buffer
    for (auto chnk_id_file = in.chunk_start; chnk_id_file < in.chunk_end || chnk_id_curr < in.chunk_n; chnk_id_file++) {
        // Continue if chunk does not hash to this host
        if (distributor.locate_data(in.path, chnk_id_file) != host_id)
            continue;
        chnk_ids_host[chnk_id_curr] = chnk_id_file; // save this id to host chunk list
        // offset case. Only relevant in the first iteration of the loop and if the chunk hashes to this host
        if (chnk_id_file == in.chunk_start && in.offset > 0) {
            // if only 1 destination and 1 chunk (small write) the transfer_size == bulk_size
            auto offset_transfer_size = (in.offset + bulk_size <= CHUNKSIZE) ? bulk_size : static_cast<size_t>(
                    CHUNKSIZE - in.offset);
            ret = margo_bulk_transfer(mid, HG_BULK_PULL, hgi->addr, in.bulk_handle, 0,
                                      bulk_handle, 0, offset_transfer_size);
            if (ret != HG_SUCCESS) {
                ADAFS_DATA->spdlogger()->error(
                        "{}() Failed to pull data from client for chunk {} (startchunk {}; endchunk {}", __func__,
                        chnk_id_file, in.chunk_start, in.chunk_end - 1);
                cancel_abt_io(&abt_tasks, &task_eventuals, chnk_id_curr);
                return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
            }
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = offset_transfer_size;
            chnk_ptr += offset_transfer_size;
            chnk_size_left_host -= offset_transfer_size;
        } else {
            local_offset = in.total_chunk_size - chnk_size_left_host;
            // origin offset of a chunk is dependent on a given offset in a write operation
            if (in.offset > 0)
                origin_offset = (CHUNKSIZE - in.offset) + ((chnk_id_file - in.chunk_start) - 1) * CHUNKSIZE;
            else
                origin_offset = (chnk_id_file - in.chunk_start) * CHUNKSIZE;
            // last chunk might have different transfer_size
            if (chnk_id_curr == in.chunk_n - 1)
                transfer_size = chnk_size_left_host;
            ADAFS_DATA->spdlogger()->trace(
                    "{}() BULK_TRANSFER hostid {} file {} chnkid {} total_Csize {} Csize_left {} origin offset {} local offset {} transfersize {}",
                    __func__, host_id, in.path, chnk_id_file, in.total_chunk_size, chnk_size_left_host,
                    origin_offset, local_offset, transfer_size);
            // RDMA the data to here
            ret = margo_bulk_transfer(mid, HG_BULK_PULL, hgi->addr, in.bulk_handle, origin_offset,
                                      bulk_handle, local_offset, transfer_size);
            if (ret != HG_SUCCESS) {
                ADAFS_DATA->spdlogger()->error(
                        "{}() Failed to pull data from client. file {} chunk {} (startchunk {}; endchunk {})", __func__,
                        *path, chnk_id_file, in.chunk_start, (in.chunk_end - 1));
                cancel_abt_io(&abt_tasks, &task_eventuals, chnk_id_curr);
                return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
            }
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = transfer_size;
            chnk_ptr += transfer_size;
            chnk_size_left_host -= transfer_size;
        }
        // Delegate chunk I/O operation to local FS to an I/O dedicated ABT pool
        // Starting tasklets for parallel I/O
        ABT_eventual_create(sizeof(ssize_t), &task_eventuals[chnk_id_curr]); // written file return value
        auto& task_arg = task_args[chnk_id_curr];
        task_arg.path = path.get();
        task_arg.buf = bulk_buf_ptrs[chnk_id_curr];
        task_arg.chnk_id = chnk_ids_host[chnk_id_curr];
        task_arg.size = chnk_sizes[chnk_id_curr];
        // only the first chunk gets the offset. the chunks are sorted on the client side
        task_arg.off = (chnk_id_file == in.chunk_start) ? in.offset : 0;
        task_arg.eventual = task_eventuals[chnk_id_curr];
        auto abt_ret = ABT_task_create(RPC_DATA->io_pool(), write_file_abt, &task_args[chnk_id_curr],
                                       &abt_tasks[chnk_id_curr]);
        if (abt_ret != ABT_SUCCESS) {
            ADAFS_DATA->spdlogger()->error("{}() task create failed", __func__);
            cancel_abt_io(&abt_tasks, &task_eventuals, chnk_id_curr + 1);
            return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
        }
        // next chunk
        chnk_id_curr++;

    }
    // Sanity check that all chunks where detected in previous loop
    if (chnk_size_left_host != 0)
        ADAFS_DATA->spdlogger()->warn("{}() Not all chunks were detected!!! Size left {}", __func__,
                                      chnk_size_left_host);
    /*
     * 4. Read task results and accumulate in out.io_size
     */
    out.err = 0;
    out.io_size = 0;
    for (chnk_id_curr = 0; chnk_id_curr < in.chunk_n; chnk_id_curr++) {
        ssize_t* task_written_size = nullptr;
        // wait causes the calling ult to go into BLOCKED state, implicitly yielding to the pool scheduler
        auto abt_ret = ABT_eventual_wait(task_eventuals[chnk_id_curr], (void**) &task_written_size);
        if (abt_ret != ABT_SUCCESS) {
            ADAFS_DATA->spdlogger()->error(
                    "{}() Failed to wait for write task for chunk {}",
                    __func__, chnk_id_curr);
            out.err = EIO;
            break;
        }
        assert(task_written_size != nullptr);
        if (*task_written_size < 0) {
            ADAFS_DATA->spdlogger()->error("{}() Write task failed for chunk {}",
                                           __func__, chnk_id_curr);
            out.err = -(*task_written_size);
            break;
        }

        out.io_size += *task_written_size; // add task written size to output size
        ABT_eventual_free(&task_eventuals[chnk_id_curr]);
    }

    // Sanity check to see if all data has been written
    if (in.total_chunk_size != out.io_size) {
        ADAFS_DATA->spdlogger()->warn("{}() total chunk size {} and out.io_size {} mismatch!", __func__,
                                      in.total_chunk_size, out.io_size);
    }

    /*
     * 5. Respond and cleanup
     */
    ADAFS_DATA->spdlogger()->debug("{}() Sending output response {}", __func__, out.err);
    ret = rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    // free tasks after responding
    for (auto&& task : abt_tasks) {
        ABT_task_join(task);
        ABT_task_free(&task);
    }
    return ret;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_write_data)

static hg_return_t rpc_srv_read_data(hg_handle_t handle) {
    /*
     * 1. Setup
     */
    rpc_read_data_in_t in{};
    rpc_data_out_t out{};
    hg_bulk_t bulk_handle = nullptr;
    // Set default out for error
    out.err = EIO;
    out.io_size = 0;
    // Getting some information from margo
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}", __func__, ret);
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);
    ADAFS_DATA->spdlogger()->debug("{}() path: {}, size: {}, offset: {}", __func__,
                                   in.path, bulk_size, in.offset);

    /*
     * 2. Set up buffers for pull bulk transfers
     */
    void* bulk_buf; // buffer for bulk transfer
    vector<char*> bulk_buf_ptrs(in.chunk_n); // buffer-chunk offsets
    // create bulk handle and allocated memory for buffer with buf_sizes information
    ret = margo_bulk_create(mid, 1, nullptr, &in.total_chunk_size, HG_BULK_READWRITE, &bulk_handle);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        return rpc_cleanup_respond(&handle, &in, &out, static_cast<hg_bulk_t*>(nullptr));
    }
    // access the internally allocated memory buffer and put it into buf_ptrs
    uint32_t actual_count;
    ret = margo_bulk_access(bulk_handle, 0, in.total_chunk_size, HG_BULK_READWRITE, 1, &bulk_buf,
                            &in.total_chunk_size, &actual_count);
    if (ret != HG_SUCCESS || actual_count != 1) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to access allocated buffer from bulk handle", __func__);
        return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto const host_id = in.host_id;
    auto const host_size = in.host_size;
    SimpleHashDistributor distributor(host_id, host_size);

    auto path = make_shared<string>(in.path);
    // chnk_ids used by this host
    vector<uint64_t> chnk_ids_host(in.chunk_n);
    // counter to track how many chunks have been assigned
    auto chnk_id_curr = static_cast<uint64_t>(0);
    // chnk sizes per chunk for this host
    vector<uint64_t> chnk_sizes(in.chunk_n);
    // local and origin offsets for bulk operations
    vector<uint64_t> local_offsets(in.chunk_n);
    vector<uint64_t> origin_offsets(in.chunk_n);
    // how much size is left to assign chunks for reading
    auto chnk_size_left_host = in.total_chunk_size;
    // temporary traveling pointer
    auto chnk_ptr = static_cast<char*>(bulk_buf);
    // temporary variables
    auto transfer_size = (bulk_size <= CHUNKSIZE) ? bulk_size : CHUNKSIZE;
    // tasks structures
    vector<ABT_task> abt_tasks(in.chunk_n);
    vector<ABT_eventual> task_eventuals(in.chunk_n);
    vector<struct read_chunk_args> task_args(in.chunk_n);
    /*
     * 3. Calculate chunk sizes that correspond to this host and start tasks to read from disk
     */
    // Start to look for a chunk that hashes to this host with the first chunk in the buffer
    for (auto chnk_id_file = in.chunk_start; chnk_id_file < in.chunk_end || chnk_id_curr < in.chunk_n; chnk_id_file++) {
        // Continue if chunk does not hash to this host
        if (distributor.locate_data(in.path, chnk_id_file) != host_id)
            continue;
        chnk_ids_host[chnk_id_curr] = chnk_id_file; // save this id to host chunk list
        // Only relevant in the first iteration of the loop and if the chunk hashes to this host
        if (chnk_id_file == in.chunk_start && in.offset > 0) {
            // if only 1 destination and 1 chunk (small read) the transfer_size == bulk_size
            auto offset_transfer_size = (in.offset + bulk_size <= CHUNKSIZE) ? bulk_size : static_cast<size_t>(
                    CHUNKSIZE - in.offset);
            // Setting later transfer offsets
            local_offsets[chnk_id_curr] = 0;
            origin_offsets[chnk_id_curr] = 0;
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = offset_transfer_size;
            // util variables
            chnk_ptr += offset_transfer_size;
            chnk_size_left_host -= offset_transfer_size;
        } else {
            local_offsets[chnk_id_curr] = in.total_chunk_size - chnk_size_left_host;
            // origin offset of a chunk is dependent on a given offset in a write operation
            if (in.offset > 0)
                origin_offsets[chnk_id_curr] =
                        (CHUNKSIZE - in.offset) + ((chnk_id_file - in.chunk_start) - 1) * CHUNKSIZE;
            else
                origin_offsets[chnk_id_curr] = (chnk_id_file - in.chunk_start) * CHUNKSIZE;
            // last chunk might have different transfer_size
            if (chnk_id_curr == in.chunk_n - 1)
                transfer_size = chnk_size_left_host;
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = transfer_size;
            // util variables
            chnk_ptr += transfer_size;
            chnk_size_left_host -= transfer_size;
        }
        // Delegate chunk I/O operation to local FS to an I/O dedicated ABT pool
        // Starting tasklets for parallel I/O
        ABT_eventual_create(sizeof(ssize_t), &task_eventuals[chnk_id_curr]); // written file return value
        auto& task_arg = task_args[chnk_id_curr];
        task_arg.path = path.get();
        task_arg.buf = bulk_buf_ptrs[chnk_id_curr];
        task_arg.chnk_id = chnk_ids_host[chnk_id_curr];
        task_arg.size = chnk_sizes[chnk_id_curr];
        // only the first chunk gets the offset. the chunks are sorted on the client side
        task_arg.off = (chnk_id_file == in.chunk_start) ? in.offset : 0;
        task_arg.eventual = task_eventuals[chnk_id_curr];
        auto abt_ret = ABT_task_create(RPC_DATA->io_pool(), read_file_abt, &task_args[chnk_id_curr],
                                       &abt_tasks[chnk_id_curr]);
        if (abt_ret != ABT_SUCCESS) {
            ADAFS_DATA->spdlogger()->error("{}() task create failed", __func__);
            cancel_abt_io(&abt_tasks, &task_eventuals, chnk_id_curr + 1);
            return rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
        }
        chnk_id_curr++;
    }
    // Sanity check that all chunks where detected in previous loop
    if (chnk_size_left_host != 0)
        ADAFS_DATA->spdlogger()->warn("{}() Not all chunks were detected!!! Size left {}", __func__,
                                      chnk_size_left_host);
    /*
     * 4. Read task results and accumulate in out.io_size
     */
    out.err = 0;
    out.io_size = 0;
    for (chnk_id_curr = 0; chnk_id_curr < in.chunk_n; chnk_id_curr++) {
        ssize_t* task_read_size = nullptr;
        // wait causes the calling ult to go into BLOCKED state, implicitly yielding to the pool scheduler
        auto abt_ret = ABT_eventual_wait(task_eventuals[chnk_id_curr], (void**) &task_read_size);
        if (abt_ret != ABT_SUCCESS) {
            ADAFS_DATA->spdlogger()->error(
                    "{}() Failed to wait for read task for chunk {}",
                    __func__, chnk_id_curr);
            out.err = EIO;
            break;
        }
        assert(task_read_size != nullptr);
        if(*task_read_size < 0){
            if(-(*task_read_size) == ENOENT) {
                continue;
            }
            ADAFS_DATA->spdlogger()->warn(
                    "{}() Read task failed for chunk {}",
                    __func__, chnk_id_curr);
            out.err = -(*task_read_size);
            break;
        }
        
        if(*task_read_size == 0) {
            continue;
        }

        ret = margo_bulk_transfer(mid, HG_BULK_PUSH, hgi->addr, in.bulk_handle, origin_offsets[chnk_id_curr],
                bulk_handle, local_offsets[chnk_id_curr], *task_read_size);
        if (ret != HG_SUCCESS) {
            ADAFS_DATA->spdlogger()->error(
                    "{}() Failed push chnkid {} on path {} to client. origin offset {} local offset {} chunk size {}",
                    __func__, chnk_id_curr, in.path, origin_offsets[chnk_id_curr], local_offsets[chnk_id_curr],
                    chnk_sizes[chnk_id_curr]);
            out.err = EIO;
            break;
        }
        out.io_size += *task_read_size; // add task read size to output size
    }

    /*
     * 5. Respond and cleanup
     */
    ADAFS_DATA->spdlogger()->debug("{}() Sending output response, err: {}", __func__, out.err);
    ret = rpc_cleanup_respond(&handle, &in, &out, &bulk_handle);
    // free tasks after responding
    cancel_abt_io(&abt_tasks, &task_eventuals, in.chunk_n);
    return ret;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_read_data)

static hg_return_t rpc_srv_trunc_data(hg_handle_t handle) {
    rpc_trunc_in_t in{};
    rpc_err_out_t out{};

    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}", __func__, ret);
        throw runtime_error("Failed to get RPC input data");
    }
    ADAFS_DATA->spdlogger()->debug("{}() path: '{}', length: {}", __func__, in.path, in.length);

    unsigned int chunk_start = chnk_id_for_offset(in.length, CHUNKSIZE);

    // If we trunc in the the middle of a chunk, do not delete that chunk
    auto left_pad = chnk_lpad(in.length, CHUNKSIZE);
    if(left_pad != 0) {
        ADAFS_DATA->storage()->truncate_chunk(in.path, chunk_start, left_pad);
        ++chunk_start;
    }

    ADAFS_DATA->storage()->trim_chunk_space(in.path, chunk_start);

    ADAFS_DATA->spdlogger()->debug("{}() Sending output {}", __func__, out.err);
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond");
    }
    // Destroy handle when finished
    margo_free_input(handle, &in);
    margo_destroy(handle);
    return HG_SUCCESS;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_trunc_data)

static hg_return_t rpc_srv_chunk_stat(hg_handle_t handle) {
    ADAFS_DATA->spdlogger()->trace("{}() called", __func__);

    rpc_chunk_stat_out_t out{};
    // Get input
    auto chk_stat = ADAFS_DATA->storage()->chunk_stat();
    // Create output and send it
    out.chunk_size = chk_stat.chunk_size;
    out.chunk_total = chk_stat.chunk_total;
    out.chunk_free = chk_stat.chunk_free;
    auto hret = margo_respond(handle, &out);
    if (hret != HG_SUCCESS) {
        ADAFS_DATA->spdlogger()->error("{}() Failed to respond", __func__);
    }

    // Destroy handle when finished
    margo_destroy(handle);
    return hret;
}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_chunk_stat)


