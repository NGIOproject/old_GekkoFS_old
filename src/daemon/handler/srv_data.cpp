/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#include <daemon/daemon.hpp>
#include <daemon/handler/rpc_defs.hpp>
#include <daemon/handler/rpc_util.hpp>
#include <daemon/backend/data/chunk_storage.hpp>
#include <daemon/ops/data.hpp>

#include <global/rpc/rpc_types.hpp>
#include <global/rpc/distributor.hpp>
#include <global/chunk_calc_util.hpp>

#ifdef GKFS_ENABLE_AGIOS
#include <daemon/scheduler/agios.hpp>

#define AGIOS_READ 0
#define AGIOS_WRITE 1
#define AGIOS_SERVER_ID_IGNORE 0
#endif

using namespace std;

/*
 * This file contains all Margo RPC handlers that are concerning management operations
 */

namespace {

/**
 * RPC handler for an incoming write RPC
 * @param handle
 * @return
 */
hg_return_t rpc_srv_write(hg_handle_t handle) {
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
        GKFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}", __func__, ret);
        return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);
    GKFS_DATA->spdlogger()->debug(
            "{}() path: '{}' chunk_start '{}' chunk_end '{}' chunk_n '{}' total_chunk_size '{}' bulk_size: '{}' offset: '{}'",
            __func__, in.path, in.chunk_start, in.chunk_end, in.chunk_n, in.total_chunk_size, bulk_size, in.offset);
#ifdef GKFS_ENABLE_AGIOS
    int *data;
    ABT_eventual eventual = ABT_EVENTUAL_NULL;

    /* creating eventual */
    ABT_eventual_create(sizeof(int64_t), &eventual);

    unsigned long long int request_id = generate_unique_id();
    char *agios_path = (char*) in.path;

    // We should call AGIOS before chunking (as that is an internal way to handle the requests)
    if (!agios_add_request(agios_path, AGIOS_WRITE, in.offset, in.total_chunk_size, request_id, AGIOS_SERVER_ID_IGNORE, agios_eventual_callback, eventual)) {
        GKFS_DATA->spdlogger()->error("{}() Failed to send request to AGIOS", __func__);
    } else {
        GKFS_DATA->spdlogger()->debug("{}() request {} was sent to AGIOS", __func__, request_id);
    }

    /* Block until the eventual is signaled */
    ABT_eventual_wait(eventual, (void **)&data);

    unsigned long long int result = *data;
    GKFS_DATA->spdlogger()->debug("{}() request {} was unblocked (offset = {})!", __func__, result, in.offset);

    ABT_eventual_free(&eventual);

    // Let AGIOS knows it can release the request, as it is completed
    if (!agios_release_request(agios_path, AGIOS_WRITE, in.total_chunk_size, in.offset)) {
        GKFS_DATA->spdlogger()->error("{}() Failed to release request from AGIOS", __func__);
    }
#endif

    /*
     * 2. Set up buffers for pull bulk transfers
     */
    void* bulk_buf; // buffer for bulk transfer
    vector<char*> bulk_buf_ptrs(in.chunk_n); // buffer-chunk offsets
    // create bulk handle and allocated memory for buffer with buf_sizes information
    ret = margo_bulk_create(mid, 1, nullptr, &in.total_chunk_size, HG_BULK_READWRITE, &bulk_handle);
    if (ret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        return gkfs::rpc::cleanup_respond(&handle, &in, &out, static_cast<hg_bulk_t*>(nullptr));
    }
    // access the internally allocated memory buffer and put it into buf_ptrs
    uint32_t actual_count;
    ret = margo_bulk_access(bulk_handle, 0, in.total_chunk_size, HG_BULK_READWRITE, 1, &bulk_buf,
                            &in.total_chunk_size, &actual_count);
    if (ret != HG_SUCCESS || actual_count != 1) {
        GKFS_DATA->spdlogger()->error("{}() Failed to access allocated buffer from bulk handle", __func__);
        return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto const host_id = in.host_id;
    auto const host_size = in.host_size;
    gkfs::rpc::SimpleHashDistributor distributor(host_id, host_size);

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
    auto transfer_size = (bulk_size <= gkfs::config::rpc::chunksize) ? bulk_size : gkfs::config::rpc::chunksize;
    uint64_t origin_offset;
    uint64_t local_offset;
    // object for asynchronous disk IO
    gkfs::data::ChunkWriteOperation chunk_op{in.path, in.chunk_n};

    /*
     * 3. Calculate chunk sizes that correspond to this host, transfer data, and start tasks to write to disk
     */
    // Start to look for a chunk that hashes to this host with the first chunk in the buffer
    for (auto chnk_id_file = in.chunk_start;
         chnk_id_file <= in.chunk_end && chnk_id_curr < in.chunk_n; chnk_id_file++) {
        // Continue if chunk does not hash to this host
#ifndef GKFS_ENABLE_FORWARDING
        if (distributor.locate_data(in.path, chnk_id_file) != host_id) {
            GKFS_DATA->spdlogger()->trace(
                    "{}() chunkid '{}' ignored as it does not match to this host with id '{}'. chnk_id_curr '{}'",
                    __func__, chnk_id_file, host_id, chnk_id_curr);
            continue;
        }
#endif

        chnk_ids_host[chnk_id_curr] = chnk_id_file; // save this id to host chunk list
        // offset case. Only relevant in the first iteration of the loop and if the chunk hashes to this host
        if (chnk_id_file == in.chunk_start && in.offset > 0) {
            // if only 1 destination and 1 chunk (small write) the transfer_size == bulk_size
            size_t offset_transfer_size = 0;
            if (in.offset + bulk_size <= gkfs::config::rpc::chunksize)
                offset_transfer_size = bulk_size;
            else
                offset_transfer_size = static_cast<size_t>(gkfs::config::rpc::chunksize - in.offset);
            ret = margo_bulk_transfer(mid, HG_BULK_PULL, hgi->addr, in.bulk_handle, 0,
                                      bulk_handle, 0, offset_transfer_size);
            if (ret != HG_SUCCESS) {
                GKFS_DATA->spdlogger()->error(
                        "{}() Failed to pull data from client for chunk {} (startchunk {}; endchunk {}", __func__,
                        chnk_id_file, in.chunk_start, in.chunk_end - 1);
                out.err = EBUSY;
                return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
            }
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = offset_transfer_size;
            chnk_ptr += offset_transfer_size;
            chnk_size_left_host -= offset_transfer_size;
        } else {
            local_offset = in.total_chunk_size - chnk_size_left_host;
            // origin offset of a chunk is dependent on a given offset in a write operation
            if (in.offset > 0)
                origin_offset = (gkfs::config::rpc::chunksize - in.offset) +
                                ((chnk_id_file - in.chunk_start) - 1) * gkfs::config::rpc::chunksize;
            else
                origin_offset = (chnk_id_file - in.chunk_start) * gkfs::config::rpc::chunksize;
            // last chunk might have different transfer_size
            if (chnk_id_curr == in.chunk_n - 1)
                transfer_size = chnk_size_left_host;
            GKFS_DATA->spdlogger()->trace(
                    "{}() BULK_TRANSFER_PULL hostid {} file {} chnkid {} total_Csize {} Csize_left {} origin offset {} local offset {} transfersize {}",
                    __func__, host_id, in.path, chnk_id_file, in.total_chunk_size, chnk_size_left_host,
                    origin_offset, local_offset, transfer_size);
            // RDMA the data to here
            ret = margo_bulk_transfer(mid, HG_BULK_PULL, hgi->addr, in.bulk_handle, origin_offset,
                                      bulk_handle, local_offset, transfer_size);
            if (ret != HG_SUCCESS) {
                GKFS_DATA->spdlogger()->error(
                        "{}() Failed to pull data from client. file {} chunk {} (startchunk {}; endchunk {})", __func__,
                        in.path, chnk_id_file, in.chunk_start, (in.chunk_end - 1));
                out.err = EBUSY;
                return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
            }
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = transfer_size;
            chnk_ptr += transfer_size;
            chnk_size_left_host -= transfer_size;
        }
        try {
            // start tasklet for writing chunk
            chunk_op.write_nonblock(chnk_id_curr, chnk_ids_host[chnk_id_curr], bulk_buf_ptrs[chnk_id_curr],
                                    chnk_sizes[chnk_id_curr], (chnk_id_file == in.chunk_start) ? in.offset : 0);
        } catch (const gkfs::data::ChunkWriteOpException& e) {
            // This exception is caused by setup of Argobots variables. If this fails, something is really wrong
            GKFS_DATA->spdlogger()->error("{}() while write_nonblock err '{}'", __func__, e.what());
            return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
        }
        // next chunk
        chnk_id_curr++;

    }
    // Sanity check that all chunks where detected in previous loop
    // TODO don't proceed if that happens.
    if (chnk_size_left_host != 0)
        GKFS_DATA->spdlogger()->warn("{}() Not all chunks were detected!!! Size left {}", __func__,
                                     chnk_size_left_host);
    /*
     * 4. Read task results and accumulate in out.io_size
     */
    auto write_result = chunk_op.wait_for_tasks();
    out.err = write_result.first;
    out.io_size = write_result.second;

    // Sanity check to see if all data has been written
    if (in.total_chunk_size != out.io_size) {
        GKFS_DATA->spdlogger()->warn("{}() total chunk size {} and out.io_size {} mismatch!", __func__,
                                     in.total_chunk_size, out.io_size);
    }

    /*
     * 5. Respond and cleanup
     */
    GKFS_DATA->spdlogger()->debug("{}() Sending output response {}", __func__, out.err);
    return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
}

/**
 * RPC handler for an incoming read RPC
 * @param handle
 * @return
 */
hg_return_t rpc_srv_read(hg_handle_t handle) {
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
        GKFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}", __func__, ret);
        return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
    auto hgi = margo_get_info(handle);
    auto mid = margo_hg_info_get_instance(hgi);
    auto bulk_size = margo_bulk_get_size(in.bulk_handle);

    GKFS_DATA->spdlogger()->debug(
            "{}() path: '{}' chunk_start '{}' chunk_end '{}' chunk_n '{}' total_chunk_size '{}' bulk_size: '{}' offset: '{}'",
            __func__, in.path, in.chunk_start, in.chunk_end, in.chunk_n, in.total_chunk_size, bulk_size, in.offset);
#ifdef GKFS_ENABLE_AGIOS
    int *data;
    ABT_eventual eventual = ABT_EVENTUAL_NULL;

    /* creating eventual */
    ABT_eventual_create(sizeof(int64_t), &eventual);

    unsigned long long int request_id = generate_unique_id();
    char *agios_path = (char*) in.path;

    // We should call AGIOS before chunking (as that is an internal way to handle the requests)
    if (!agios_add_request(agios_path, AGIOS_READ, in.offset, in.total_chunk_size, request_id, AGIOS_SERVER_ID_IGNORE, agios_eventual_callback, eventual)) {
        GKFS_DATA->spdlogger()->error("{}() Failed to send request to AGIOS", __func__);
    } else {
        GKFS_DATA->spdlogger()->debug("{}() request {} was sent to AGIOS", __func__, request_id);
    }

    /* block until the eventual is signaled */
    ABT_eventual_wait(eventual, (void **)&data);

    unsigned long long int result = *data;
    GKFS_DATA->spdlogger()->debug("{}() request {} was unblocked (offset = {})!", __func__, result, in.offset);

    ABT_eventual_free(&eventual);

    // let AGIOS knows it can release the request, as it is completed
    if (!agios_release_request(agios_path, AGIOS_READ, in.total_chunk_size, in.offset)) {
        GKFS_DATA->spdlogger()->error("{}() Failed to release request from AGIOS", __func__);
    }
#endif

    /*
     * 2. Set up buffers for pull bulk transfers
     */
    void* bulk_buf; // buffer for bulk transfer
    vector<char*> bulk_buf_ptrs(in.chunk_n); // buffer-chunk offsets
    // create bulk handle and allocated memory for buffer with buf_sizes information
    ret = margo_bulk_create(mid, 1, nullptr, &in.total_chunk_size, HG_BULK_READWRITE, &bulk_handle);
    if (ret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Failed to create bulk handle", __func__);
        return gkfs::rpc::cleanup_respond(&handle, &in, &out, static_cast<hg_bulk_t*>(nullptr));
    }
    // access the internally allocated memory buffer and put it into buf_ptrs
    uint32_t actual_count;
    ret = margo_bulk_access(bulk_handle, 0, in.total_chunk_size, HG_BULK_READWRITE, 1, &bulk_buf,
                            &in.total_chunk_size, &actual_count);
    if (ret != HG_SUCCESS || actual_count != 1) {
        GKFS_DATA->spdlogger()->error("{}() Failed to access allocated buffer from bulk handle", __func__);
        return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
    }
#ifndef GKFS_ENABLE_FORWARDING
    auto const host_id = in.host_id;
    auto const host_size = in.host_size;
    gkfs::rpc::SimpleHashDistributor distributor(host_id, host_size);
#endif

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
    auto transfer_size = (bulk_size <= gkfs::config::rpc::chunksize) ? bulk_size : gkfs::config::rpc::chunksize;
    // object for asynchronous disk IO
    gkfs::data::ChunkReadOperation chunk_read_op{in.path, in.chunk_n};
    /*
     * 3. Calculate chunk sizes that correspond to this host and start tasks to read from disk
     */
    // Start to look for a chunk that hashes to this host with the first chunk in the buffer
    for (auto chnk_id_file = in.chunk_start;
         chnk_id_file <= in.chunk_end && chnk_id_curr < in.chunk_n; chnk_id_file++) {
        // Continue if chunk does not hash to this host
#ifndef GKFS_ENABLE_FORWARDING
        if (distributor.locate_data(in.path, chnk_id_file) != host_id) {
            GKFS_DATA->spdlogger()->trace(
                    "{}() chunkid '{}' ignored as it does not match to this host with id '{}'. chnk_id_curr '{}'",
                    __func__, chnk_id_file, host_id, chnk_id_curr);
            continue;
        }
#endif

        chnk_ids_host[chnk_id_curr] = chnk_id_file; // save this id to host chunk list
        // Only relevant in the first iteration of the loop and if the chunk hashes to this host
        if (chnk_id_file == in.chunk_start && in.offset > 0) {
            // if only 1 destination and 1 chunk (small read) the transfer_size == bulk_size
            size_t offset_transfer_size = 0;
            if (in.offset + bulk_size <= gkfs::config::rpc::chunksize)
                offset_transfer_size = bulk_size;
            else
                offset_transfer_size = static_cast<size_t>(gkfs::config::rpc::chunksize - in.offset);
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
                        (gkfs::config::rpc::chunksize - in.offset) +
                        ((chnk_id_file - in.chunk_start) - 1) * gkfs::config::rpc::chunksize;
            else
                origin_offsets[chnk_id_curr] = (chnk_id_file - in.chunk_start) * gkfs::config::rpc::chunksize;
            // last chunk might have different transfer_size
            if (chnk_id_curr == in.chunk_n - 1)
                transfer_size = chnk_size_left_host;
            bulk_buf_ptrs[chnk_id_curr] = chnk_ptr;
            chnk_sizes[chnk_id_curr] = transfer_size;
            // util variables
            chnk_ptr += transfer_size;
            chnk_size_left_host -= transfer_size;
        }
        try {
            // start tasklet for read operation
            chunk_read_op.read_nonblock(chnk_id_curr, chnk_ids_host[chnk_id_curr], bulk_buf_ptrs[chnk_id_curr],
                                        chnk_sizes[chnk_id_curr], (chnk_id_file == in.chunk_start) ? in.offset : 0);
        } catch (const gkfs::data::ChunkReadOpException& e) {
            // This exception is caused by setup of Argobots variables. If this fails, something is really wrong
            GKFS_DATA->spdlogger()->error("{}() while read_nonblock err '{}'", __func__, e.what());
            return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
        }
        chnk_id_curr++;
    }
    // Sanity check that all chunks where detected in previous loop
    // TODO error out. If we continue this will crash the server when sending results back that don't exist.
    if (chnk_size_left_host != 0)
        GKFS_DATA->spdlogger()->warn("{}() Not all chunks were detected!!! Size left {}", __func__,
                                     chnk_size_left_host);
    /*
     * 4. Read task results and accumulate in out.io_size
     */
    gkfs::data::ChunkReadOperation::bulk_args bulk_args{};
    bulk_args.mid = mid;
    bulk_args.origin_addr = hgi->addr;
    bulk_args.origin_bulk_handle = in.bulk_handle;
    bulk_args.origin_offsets = &origin_offsets;
    bulk_args.local_bulk_handle = bulk_handle;
    bulk_args.local_offsets = &local_offsets;
    bulk_args.chunk_ids = &chnk_ids_host;
    // wait for all tasklets and push read data back to client
    auto read_result = chunk_read_op.wait_for_tasks_and_push_back(bulk_args);
    out.err = read_result.first;
    out.io_size = read_result.second;

    /*
     * 5. Respond and cleanup
     */
    GKFS_DATA->spdlogger()->debug("{}() Sending output response, err: {}", __func__, out.err);
    return gkfs::rpc::cleanup_respond(&handle, &in, &out, &bulk_handle);
}


/**
 * RPC handler for an incoming truncate RPC
 * @param handle
 * @return
 */
hg_return_t rpc_srv_truncate(hg_handle_t handle) {
    rpc_trunc_in_t in{};
    rpc_err_out_t out{};
    out.err = EIO;
    // Getting some information from margo
    auto ret = margo_get_input(handle, &in);
    if (ret != HG_SUCCESS) {
        GKFS_DATA->spdlogger()->error("{}() Could not get RPC input data with err {}", __func__, ret);
        return gkfs::rpc::cleanup_respond(&handle, &in, &out);
    }
    GKFS_DATA->spdlogger()->debug("{}() path: '{}', length: '{}'", __func__, in.path, in.length);

    gkfs::data::ChunkTruncateOperation chunk_op{in.path};
    try {
        // start tasklet for truncate operation
        chunk_op.truncate(in.length);
    } catch (const gkfs::data::ChunkMetaOpException& e) {
        // This exception is caused by setup of Argobots variables. If this fails, something is really wrong
        GKFS_DATA->spdlogger()->error("{}() while truncate err '{}'", __func__, e.what());
        return gkfs::rpc::cleanup_respond(&handle, &in, &out);
    }

    // wait and get output
    out.err = chunk_op.wait_for_task();

    GKFS_DATA->spdlogger()->debug("{}() Sending output response '{}'", __func__, out.err);
    return gkfs::rpc::cleanup_respond(&handle, &in, &out);
}


/**
 * RPC handler for an incoming chunk stat RPC
 * @param handle
 * @return
 */
hg_return_t rpc_srv_get_chunk_stat(hg_handle_t handle) {
    GKFS_DATA->spdlogger()->debug("{}() enter", __func__);
    rpc_chunk_stat_out_t out{};
    out.err = EIO;
    try {
        auto chk_stat = GKFS_DATA->storage()->chunk_stat();
        out.chunk_size = chk_stat.chunk_size;
        out.chunk_total = chk_stat.chunk_total;
        out.chunk_free = chk_stat.chunk_free;
        out.err = 0;
    } catch (const gkfs::data::ChunkStorageException& err) {
        GKFS_DATA->spdlogger()->error("{}() {}", __func__, err.what());
        out.err = err.code().value();
    } catch (const ::exception& err) {
        GKFS_DATA->spdlogger()->error("{}() Unexpected error when chunk stat '{}'", __func__, err.what());
        out.err = EAGAIN;
    }

    // Create output and send it back
    return gkfs::rpc::cleanup_respond(&handle, &out);
}

}

DEFINE_MARGO_RPC_HANDLER(rpc_srv_write)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_read)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_truncate)

DEFINE_MARGO_RPC_HANDLER(rpc_srv_get_chunk_stat)

#ifdef GKFS_ENABLE_AGIOS
void *agios_eventual_callback(int64_t request_id, void* info) {
    GKFS_DATA->spdlogger()->debug("{}() custom callback request {} is ready", __func__, request_id);

    ABT_eventual_set((ABT_eventual) info, &request_id, sizeof(int64_t));

    return 0;
}
#endif

