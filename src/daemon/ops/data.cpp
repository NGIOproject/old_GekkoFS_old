/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <daemon/ops/data.hpp>
#include <daemon/backend/data/chunk_storage.hpp>
#include <global/chunk_calc_util.hpp>
#include <utility>

extern "C" {
#include <mercury_types.h>
}

using namespace std;

namespace gkfs {
namespace data {

/* ------------------------------------------------------------------------
 * -------------------------- TRUNCATE ------------------------------------
 * ------------------------------------------------------------------------*/


/**
 * Used by an argobots tasklet. Argument args has the following fields:
 * const string* path;
   size_t size;
   ABT_eventual* eventual;
 * This function is driven by the IO pool. so there is a maximum allowed number of concurrent operations per daemon.
 * @return error<int> is put into eventual to signal that it finished
 */
void ChunkTruncateOperation::truncate_abt(void* _arg) {
    assert(_arg);
    // Unpack args
    auto* arg = static_cast<struct chunk_truncate_args*>(_arg);
    const string& path = *(arg->path);
    const size_t size = arg->size;
    int err_response = 0;
    try {
        // get chunk from where to cut off
        auto chunk_id_start = gkfs::util::chnk_id_for_offset(size, gkfs::config::rpc::chunksize);
        // do not last delete chunk if it is in the middle of a chunk
        auto left_pad = gkfs::util::chnk_lpad(size, gkfs::config::rpc::chunksize);
        if (left_pad != 0) {
            GKFS_DATA->storage()->truncate_chunk_file(path, chunk_id_start, left_pad);
            chunk_id_start++;
        }
        GKFS_DATA->storage()->trim_chunk_space(path, chunk_id_start);
    } catch (const ChunkStorageException& err) {
        GKFS_DATA->spdlogger()->error("{}() {}", __func__, err.what());
        err_response = err.code().value();
    } catch (const ::exception& err) {
        GKFS_DATA->spdlogger()->error("{}() Unexpected error truncating file '{}' to length '{}'", __func__, path,
                                      size);
        err_response = EIO;
    }
    ABT_eventual_set(arg->eventual, &err_response, sizeof(err_response));
}

void ChunkTruncateOperation::clear_task_args() {
    task_arg_ = {};
}

ChunkTruncateOperation::ChunkTruncateOperation(const string& path) : ChunkOperation{path, 1} {}

/**
 * Starts a tasklet for requested truncate. In essence all chunk files after the given offset is removed
 * Only one truncate call is allowed at a time
 */
void ChunkTruncateOperation::truncate(size_t size) {
    assert(!task_eventuals_[0]);
    GKFS_DATA->spdlogger()->trace("ChunkTruncateOperation::{}() enter: path '{}' size '{}'", __func__, path_,
                                  size);
    // sizeof(int) comes from truncate's return type
    auto abt_err = ABT_eventual_create(sizeof(int), &task_eventuals_[0]); // truncate file return value
    if (abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format("ChunkTruncateOperation::{}() Failed to create ABT eventual with abt_err '{}'",
                                   __func__, abt_err);
        throw ChunkMetaOpException(err_str);
    }

    auto& task_arg = task_arg_;
    task_arg.path = &path_;
    task_arg.size = size;
    task_arg.eventual = task_eventuals_[0];

    abt_err = ABT_task_create(RPC_DATA->io_pool(), truncate_abt, &task_arg_, &abt_tasks_[0]);
    if (abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format("ChunkTruncateOperation::{}() Failed to create ABT task with abt_err '{}'", __func__,
                                   abt_err);
        throw ChunkMetaOpException(err_str);
    }

}


int ChunkTruncateOperation::wait_for_task() {
    GKFS_DATA->spdlogger()->trace("ChunkTruncateOperation::{}() enter: path '{}'", __func__, path_);
    int trunc_err = 0;

    int* task_err = nullptr;
    auto abt_err = ABT_eventual_wait(task_eventuals_[0], (void**) &task_err);
    if (abt_err != ABT_SUCCESS) {
        GKFS_DATA->spdlogger()->error("ChunkTruncateOperation::{}() Error when waiting on ABT eventual", __func__);
        ABT_eventual_free(&task_eventuals_[0]);
        return EIO;
    }
    assert(task_err != nullptr);
    if (*task_err != 0) {
        trunc_err = *task_err;
    }
    ABT_eventual_free(&task_eventuals_[0]);
    return trunc_err;
}

/* ------------------------------------------------------------------------
 * ----------------------------- WRITE ------------------------------------
 * ------------------------------------------------------------------------*/

/**
 * Used by an argobots tasklet. Argument args has the following fields:
 * const string* path;
   const char* buf;
   const gkfs::rpc::chnk_id_t* chnk_id;
   size_t size;
   off64_t off;
   ABT_eventual* eventual;
 * This function is driven by the IO pool. so there is a maximum allowed number of concurrent IO operations per daemon.
 * This function is called by tasklets, as this function cannot be allowed to block.
 * @return written_size<ssize_t> is put into eventual to signal that it finished
 */
void ChunkWriteOperation::write_file_abt(void* _arg) {
    assert(_arg);
    // Unpack args
    auto* arg = static_cast<struct chunk_write_args*>(_arg);
    const string& path = *(arg->path);
    ssize_t wrote{0};
    try {
        wrote = GKFS_DATA->storage()->write_chunk(path, arg->chnk_id, arg->buf, arg->size, arg->off);
    } catch (const ChunkStorageException& err) {
        GKFS_DATA->spdlogger()->error("{}() {}", __func__, err.what());
        wrote = -(err.code().value());
    } catch (const ::exception& err) {
        GKFS_DATA->spdlogger()->error("{}() Unexpected error writing chunk {} of file {}", __func__, arg->chnk_id,
                                      path);
        wrote = -EIO;
    }
    ABT_eventual_set(arg->eventual, &wrote, sizeof(wrote));
}

void ChunkWriteOperation::clear_task_args() {
    task_args_.clear();
}

ChunkWriteOperation::ChunkWriteOperation(const string& path, size_t n) : ChunkOperation{path, n} {
    task_args_.resize(n);
}

/**
 * Write buffer from a single chunk referenced by its ID. Put task into IO queue.
 * On failure the write operations is aborted, throwing an error, and cleaned up.
 * The caller may repeat a failed call.
 * @param chunk_id
 * @param bulk_buf_ptr
 * @param size
 * @param offset
 * @throws ChunkWriteOpException
 */
void
ChunkWriteOperation::write_nonblock(size_t idx, const uint64_t chunk_id, const char* bulk_buf_ptr,
                                    const size_t size,
                                    const off64_t offset) {
    assert(idx < task_args_.size());
    GKFS_DATA->spdlogger()->trace("ChunkWriteOperation::{}() enter: idx '{}' path '{}' size '{}' offset '{}'", __func__,
                                  idx, path_, size, offset);
    // sizeof(ssize_t) comes from pwrite's return type
    auto abt_err = ABT_eventual_create(sizeof(ssize_t), &task_eventuals_[idx]); // written file return value
    if (abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format("ChunkWriteOperation::{}() Failed to create ABT eventual with abt_err '{}'",
                                   __func__, abt_err);
        throw ChunkWriteOpException(err_str);
    }

    auto& task_arg = task_args_[idx];
    task_arg.path = &path_;
    task_arg.buf = bulk_buf_ptr;
    task_arg.chnk_id = chunk_id;
    task_arg.size = size;
    task_arg.off = offset;
    task_arg.eventual = task_eventuals_[idx];

    abt_err = ABT_task_create(RPC_DATA->io_pool(), write_file_abt, &task_args_[idx], &abt_tasks_[idx]);
    if (abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format("ChunkWriteOperation::{}() Failed to create ABT task with abt_err '{}'", __func__,
                                   abt_err);
        throw ChunkWriteOpException(err_str);
    }
}

/**
 * Waits for all Argobots tasklets to finish and report back the write error code and the size written.
 * @return <int, size_t>
 */
pair<int, size_t> ChunkWriteOperation::wait_for_tasks() {
    GKFS_DATA->spdlogger()->trace("ChunkWriteOperation::{}() enter: path '{}'", __func__, path_);
    size_t total_written = 0;
    int io_err = 0;
    /*
     * gather all Eventual's information. do not throw here to properly cleanup all eventuals
     * On error, cleanup eventuals and set written data to 0 as written data is corrupted
     */
    for (auto& e : task_eventuals_) {
        ssize_t* task_size = nullptr;
        auto abt_err = ABT_eventual_wait(e, (void**) &task_size);
        if (abt_err != ABT_SUCCESS) {
            GKFS_DATA->spdlogger()->error("ChunkWriteOperation::{}() Error when waiting on ABT eventual", __func__);
            io_err = EIO;
            ABT_eventual_free(&e);
            continue;
        }
        if (io_err != 0) {
            ABT_eventual_free(&e);
            continue;
        }
        assert(task_size != nullptr);
        if (*task_size < 0) {
            io_err = -(*task_size);
        } else {
            total_written += *task_size;
        }
        ABT_eventual_free(&e);
    }
    // in case of error set written size to zero as data would be corrupted
    if (io_err != 0)
        total_written = 0;
    return make_pair(io_err, total_written);
}

/* ------------------------------------------------------------------------
 * -------------------------- READ ----------------------------------------
 * ------------------------------------------------------------------------*/

/**
 * Used by an argobots tasklet. Argument args has the following fields:
 * const string* path;
   char* buf;
   const gkfs::rpc::chnk_id_t* chnk_id;
   size_t size;
   off64_t off;
   ABT_eventual* eventual;
 * This function is driven by the IO pool. so there is a maximum allowed number of concurrent IO operations per daemon.
 * This function is called by tasklets, as this function cannot be allowed to block.
 * @return read_size<ssize_t> is put into eventual to signal that it finished
 */
void ChunkReadOperation::read_file_abt(void* _arg) {
    assert(_arg);
    //unpack args
    auto* arg = static_cast<struct chunk_read_args*>(_arg);
    const string& path = *(arg->path);
    ssize_t read = 0;
    try {
        // Under expected circumstances (error or no error) read_chunk will signal the eventual
        read = GKFS_DATA->storage()->read_chunk(path, arg->chnk_id, arg->buf, arg->size, arg->off);
    } catch (const ChunkStorageException& err) {
        GKFS_DATA->spdlogger()->error("{}() {}", __func__, err.what());
        read = -(err.code().value());
    } catch (const ::exception& err) {
        GKFS_DATA->spdlogger()->error("{}() Unexpected error reading chunk {} of file {}", __func__, arg->chnk_id,
                                      path);
        read = -EIO;
    }
    ABT_eventual_set(arg->eventual, &read, sizeof(read));
}

void ChunkReadOperation::clear_task_args() {
    task_args_.clear();
}

ChunkReadOperation::ChunkReadOperation(const string& path, size_t n) : ChunkOperation{path, n} {
    task_args_.resize(n);
}

/**
 * Read buffer to a single chunk referenced by its ID. Put task into IO queue.
 * On failure the read operations is aborted, throwing an error, and cleaned up.
 * The caller may repeat a failed call.
 * @param chunk_id
 * @param bulk_buf_ptr
 * @param size
 * @param offset
 */
void
ChunkReadOperation::read_nonblock(size_t idx, const uint64_t chunk_id, char* bulk_buf_ptr, const size_t size,
                                  const off64_t offset) {
    assert(idx < task_args_.size());
    GKFS_DATA->spdlogger()->trace("ChunkReadOperation::{}() enter: idx '{}' path '{}' size '{}' offset '{}'", __func__,
                                  idx, path_, size, offset);
    // sizeof(ssize_t) comes from pread's return type
    auto abt_err = ABT_eventual_create(sizeof(ssize_t), &task_eventuals_[idx]); // read file return value
    if (abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format("ChunkReadOperation::{}() Failed to create ABT eventual with abt_err '{}'",
                                   __func__, abt_err);
        throw ChunkReadOpException(err_str);
    }

    auto& task_arg = task_args_[idx];
    task_arg.path = &path_;
    task_arg.buf = bulk_buf_ptr;
    task_arg.chnk_id = chunk_id;
    task_arg.size = size;
    task_arg.off = offset;
    task_arg.eventual = task_eventuals_[idx];

    abt_err = ABT_task_create(RPC_DATA->io_pool(), read_file_abt, &task_args_[idx], &abt_tasks_[idx]);
    if (abt_err != ABT_SUCCESS) {
        auto err_str = fmt::format("ChunkReadOperation::{}() Failed to create ABT task with abt_err '{}'", __func__,
                                   abt_err);
        throw ChunkReadOpException(err_str);
    }

}

pair<int, size_t> ChunkReadOperation::wait_for_tasks_and_push_back(const bulk_args& args) {
    GKFS_DATA->spdlogger()->trace("ChunkReadOperation::{}() enter: path '{}'", __func__, path_);
    assert(args.chunk_ids->size() == task_args_.size());
    size_t total_read = 0;
    int io_err = 0;

    /*
     * gather all Eventual's information. do not throw here to properly cleanup all eventuals
     * As soon as an error is encountered, bulk_transfers will no longer be executed as the data would be corrupted
     * The loop continues until all eventuals have been cleaned and freed.
     */
    for (uint64_t idx = 0; idx < task_args_.size(); idx++) {
        ssize_t* task_size = nullptr;
        auto abt_err = ABT_eventual_wait(task_eventuals_[idx], (void**) &task_size);
        if (abt_err != ABT_SUCCESS) {
            GKFS_DATA->spdlogger()->error("ChunkReadOperation::{}() Error when waiting on ABT eventual", __func__);
            io_err = EIO;
            ABT_eventual_free(&task_eventuals_[idx]);
            continue;
        }
        // error occured. stop processing but clean up
        if (io_err != 0) {
            ABT_eventual_free(&task_eventuals_[idx]);
            continue;
        }
        assert(task_size != nullptr);
        if (*task_size < 0) {
            // sparse regions do not have chunk files and are therefore skipped
            if (-(*task_size) == ENOENT) {
                ABT_eventual_free(&task_eventuals_[idx]);
                continue;
            }
            io_err = -(*task_size); // make error code > 0
        } else if (*task_size == 0) {
            // read size of 0 is not an error and can happen because reading the end-of-file
            ABT_eventual_free(&task_eventuals_[idx]);
            continue;
        } else {
            // successful case, push read data back to client
            GKFS_DATA->spdlogger()->trace(
                    "ChunkReadOperation::{}() BULK_TRANSFER_PUSH file '{}' chnkid '{}' origin offset '{}' local offset '{}' transfersize '{}'",
                    __func__, path_, args.chunk_ids->at(idx), args.origin_offsets->at(idx), args.local_offsets->at(idx),
                    *task_size);
            assert(task_args_[idx].chnk_id == args.chunk_ids->at(idx));
            auto margo_err = margo_bulk_transfer(args.mid, HG_BULK_PUSH, args.origin_addr, args.origin_bulk_handle,
                                                 args.origin_offsets->at(idx), args.local_bulk_handle,
                                                 args.local_offsets->at(idx), *task_size);
            if (margo_err != HG_SUCCESS) {
                GKFS_DATA->spdlogger()->error(
                        "ChunkReadOperation::{}() Failed to margo_bulk_transfer with margo err: '{}'", __func__,
                        margo_err);
                io_err = EBUSY;
                continue;
            }
            total_read += *task_size;
        }
        ABT_eventual_free(&task_eventuals_[idx]);
    }
    // in case of error set read size to zero as data would be corrupted
    if (io_err != 0)
        total_read = 0;
    return make_pair(io_err, total_read);
}

} // namespace data
} // namespace gkfs
