/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_DAEMON_DATA_HPP
#define GEKKOFS_DAEMON_DATA_HPP

#include <daemon/daemon.hpp>
#include <global/global_defs.hpp>

#include <string>
#include <vector>

extern "C" {
#include <abt.h>
#include <margo.h>
}

namespace gkfs {
namespace data {

class ChunkOpException : public std::runtime_error {
public:
    explicit ChunkOpException(const std::string& s) : std::runtime_error(s) {};
};

class ChunkWriteOpException : public ChunkOpException {
public:
    explicit ChunkWriteOpException(const std::string& s) : ChunkOpException(s) {};
};

class ChunkReadOpException : public ChunkOpException {
public:
    explicit ChunkReadOpException(const std::string& s) : ChunkOpException(s) {};
};

class ChunkMetaOpException : public ChunkOpException {
public:
    explicit ChunkMetaOpException(const std::string& s) : ChunkOpException(s) {};
};

/**
 * Classes to encapsulate asynchronous chunk operations.
 * All operations on chunk files must go through the Argobots' task queues.
 * Otherwise operations may overtake operations in the queues.
 * This applies to write, read, and truncate which may modify the middle of a chunk, essentially a write operation.
 *
 * Note: This class is not thread-safe.
 *
 * In the future, this class may be used to provide failure tolerance for IO tasks
 *
 * Base class using the CRTP idiom
 */
template<class OperationType>
class ChunkOperation {

protected:

    const std::string path_;

    std::vector<ABT_task> abt_tasks_;
    std::vector<ABT_eventual> task_eventuals_;

public:

    explicit ChunkOperation(const std::string& path) : ChunkOperation(path, 1) {};

    ChunkOperation(std::string path, size_t n) : path_(std::move(path)) {
        // Knowing n beforehand is important and cannot be dynamic. Otherwise eventuals cause seg faults
        abt_tasks_.resize(n);
        task_eventuals_.resize(n);
    };

    ~ChunkOperation() {
        cancel_all_tasks();
    }

    /**
     * Cleans up and cancels all tasks in flight
     */
    void cancel_all_tasks() {
        GKFS_DATA->spdlogger()->trace("{}() enter", __func__);
        for (auto& task : abt_tasks_) {
            if (task) {
                ABT_task_cancel(task);
                ABT_task_free(&task);
            }
        }
        for (auto& eventual : task_eventuals_) {
            if (eventual) {
                ABT_eventual_reset(eventual);
                ABT_eventual_free(&eventual);
            }
        }
        abt_tasks_.clear();
        task_eventuals_.clear();
        static_cast<OperationType*>(this)->clear_task_args();
    }
};


class ChunkTruncateOperation : public ChunkOperation<ChunkTruncateOperation> {
    friend class ChunkOperation<ChunkTruncateOperation>;

private:
    struct chunk_truncate_args {
        const std::string* path;
        size_t size;
        ABT_eventual eventual;
    };

    struct chunk_truncate_args task_arg_{};

    static void truncate_abt(void* _arg);

    void clear_task_args();

public:

    explicit ChunkTruncateOperation(const std::string& path);

    ~ChunkTruncateOperation() = default;

    void truncate(size_t size);

    int wait_for_task();
};

class ChunkWriteOperation : public ChunkOperation<ChunkWriteOperation> {
    friend class ChunkOperation<ChunkWriteOperation>;

private:

    struct chunk_write_args {
        const std::string* path;
        const char* buf;
        gkfs::rpc::chnk_id_t chnk_id;
        size_t size;
        off64_t off;
        ABT_eventual eventual;
    };

    std::vector<struct chunk_write_args> task_args_;

    static void write_file_abt(void* _arg);

    void clear_task_args();

public:

    ChunkWriteOperation(const std::string& path, size_t n);

    ~ChunkWriteOperation() = default;

    void write_nonblock(size_t idx, uint64_t chunk_id, const char* bulk_buf_ptr, size_t size, off64_t offset);

    std::pair<int, size_t> wait_for_tasks();

};


class ChunkReadOperation : public ChunkOperation<ChunkReadOperation> {
    friend class ChunkOperation<ChunkReadOperation>;

private:

    struct chunk_read_args {
        const std::string* path;
        char* buf;
        gkfs::rpc::chnk_id_t chnk_id;
        size_t size;
        off64_t off;
        ABT_eventual eventual;
    };

    std::vector<struct chunk_read_args> task_args_;

    static void read_file_abt(void* _arg);

    void clear_task_args();

public:

    struct bulk_args {
        margo_instance_id mid;
        hg_addr_t origin_addr;
        hg_bulk_t origin_bulk_handle;
        std::vector<size_t>* origin_offsets;
        hg_bulk_t local_bulk_handle;
        std::vector<size_t>* local_offsets;
        std::vector<uint64_t>* chunk_ids;
    };

    ChunkReadOperation(const std::string& path, size_t n);

    ~ChunkReadOperation() = default;

    void read_nonblock(size_t idx, uint64_t chunk_id, char* bulk_buf_ptr, size_t size, off64_t offset);

    std::pair<int, size_t>
    wait_for_tasks_and_push_back(const bulk_args& args);

};

} // namespace data
} // namespace gkfs

#endif //GEKKOFS_DAEMON_DATA_HPP
