/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <daemon/backend/data/data_module.hpp>
#include <daemon/backend/data/chunk_storage.hpp>
#include <daemon/backend/data/file_handle.hpp>
#include <global/path_util.hpp>

#include <cerrno>

#include <boost/filesystem.hpp>
#include <spdlog/spdlog.h>

extern "C" {
#include <sys/statfs.h>
}

namespace bfs = boost::filesystem;
using namespace std;

namespace gkfs {
namespace data {

// private functions

string ChunkStorage::absolute(const string& internal_path) const {
    assert(gkfs::path::is_relative(internal_path));
    return fmt::format("{}/{}", root_path_, internal_path);
}

string ChunkStorage::get_chunks_dir(const string& file_path) {
    assert(gkfs::path::is_absolute(file_path));
    string chunk_dir = file_path.substr(1);
    ::replace(chunk_dir.begin(), chunk_dir.end(), '/', ':');
    return chunk_dir;
}

string ChunkStorage::get_chunk_path(const string& file_path, gkfs::rpc::chnk_id_t chunk_id) {
    return fmt::format("{}/{}", get_chunks_dir(file_path), chunk_id);
}

/**
 * Creates a chunk directory that all chunk files are placed in.
 * The path to the real file will be used as the directory name
 * @param file_path
 * @throws ChunkStorageException on error
 */
void ChunkStorage::init_chunk_space(const string& file_path) const {
    auto chunk_dir = absolute(get_chunks_dir(file_path));
    auto err = mkdir(chunk_dir.c_str(), 0750);
    if (err == -1 && errno != EEXIST) {
        auto err_str = fmt::format("{}() Failed to create chunk directory. File: '{}', Error: '{}'", __func__,
                                   file_path, errno);
        throw ChunkStorageException(errno, err_str);
    }
}

// public functions

/**
 *
 * @param path
 * @param chunksize
 * @throws ChunkStorageException
 */
ChunkStorage::ChunkStorage(string& path, const size_t chunksize) :
        root_path_(path),
        chunksize_(chunksize) {
    /* Get logger instance and set it for data module and chunk storage */
    GKFS_DATA_MOD->log(spdlog::get(GKFS_DATA_MOD->LOGGER_NAME));
    assert(GKFS_DATA_MOD->log());
    log_ = spdlog::get(GKFS_DATA_MOD->LOGGER_NAME);
    assert(log_);
    assert(gkfs::path::is_absolute(root_path_));
    // Verify that we have sufficient access for chunk directories
    if (access(root_path_.c_str(), W_OK | R_OK) != 0) {
        auto err_str = fmt::format("{}() Insufficient permissions to create chunk directories in path '{}'", __func__,
                                   root_path_);
        throw ChunkStorageException(EPERM, err_str);
    }
    log_->debug("{}() Chunk storage initialized with path: '{}'", __func__, root_path_);
}

/**
 * Removes chunk directory with all its files
 * @param file_path
 * @throws ChunkStorageException
 */
void ChunkStorage::destroy_chunk_space(const string& file_path) const {
    auto chunk_dir = absolute(get_chunks_dir(file_path));
    try {
        // Note: remove_all does not throw an error when path doesn't exist.
        auto n = bfs::remove_all(chunk_dir);
        log_->debug("{}() Removed '{}' files from '{}'", __func__, n, chunk_dir);
    } catch (const bfs::filesystem_error& e) {
        auto err_str = fmt::format("{}() Failed to remove chunk directory. Path: '{}', Error: '{}'", __func__,
                                   chunk_dir, e.what());
        throw ChunkStorageException(e.code().value(), err_str);
    }
}

/**
 * Writes a chunk file.
 * On failure throws ChunkStorageException with encapsulated error code
 *
 * Refer to https://www.gnu.org/software/libc/manual/html_node/I_002fO-Primitives.html for pwrite behavior
 *
 * @param file_path
 * @param chunk_id
 * @param buf
 * @param size
 * @param offset
 * @param eventual
 * @throws ChunkStorageException (caller will handle eventual signalling)
 */
ssize_t
ChunkStorage::write_chunk(const string& file_path, gkfs::rpc::chnk_id_t chunk_id, const char* buf, size_t size,
                          off64_t offset) const {

    assert((offset + size) <= chunksize_);
    // may throw ChunkStorageException on failure
    init_chunk_space(file_path);

    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));

    FileHandle fh(open(chunk_path.c_str(), O_WRONLY | O_CREAT, 0640), chunk_path);
    if (!fh.valid()) {
        auto err_str = fmt::format("{}() Failed to open chunk file for write. File: '{}', Error: '{}'", __func__,
                                   chunk_path, ::strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }

    size_t wrote_total{};
    ssize_t wrote{};

    do {
        wrote = pwrite(fh.native(),
                       buf + wrote_total,
                       size - wrote_total,
                       offset + wrote_total);

        if (wrote < 0) {
            // retry if a signal or anything else has interrupted the read system call
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            auto err_str = fmt::format(
                    "{}() Failed to write chunk file. File: '{}', size: '{}', offset: '{}', Error: '{}'",
                    __func__, chunk_path, size, offset, ::strerror(errno));
            throw ChunkStorageException(errno, err_str);
        }
        wrote_total += wrote;
    } while (wrote_total != size);

    // file is closed via the file handle's destructor.
    return wrote_total;
}

/**
 * Read from a chunk file.
 * On failure throws ChunkStorageException with encapsulated error code
 *
 * Refer to https://www.gnu.org/software/libc/manual/html_node/I_002fO-Primitives.html for pread behavior
 * @param file_path
 * @param chunk_id
 * @param buf
 * @param size
 * @param offset
 * @param eventual
 * @throws ChunkStorageException (caller will handle eventual signalling)
 */
ssize_t
ChunkStorage::read_chunk(const string& file_path, gkfs::rpc::chnk_id_t chunk_id, char* buf, size_t size,
                         off64_t offset) const {
    assert((offset + size) <= chunksize_);
    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));

    FileHandle fh(open(chunk_path.c_str(), O_RDONLY), chunk_path);
    if (!fh.valid()) {
        auto err_str = fmt::format("{}() Failed to open chunk file for read. File: '{}', Error: '{}'", __func__,
                                   chunk_path, ::strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }
    size_t read_total = 0;
    ssize_t read = 0;

    do {
        read = pread64(fh.native(),
                       buf + read_total,
                       size - read_total,
                       offset + read_total);
        if (read == 0) {
            /*
             * A value of zero indicates end-of-file (except if the value of the size argument is also zero).
             * This is not considered an error. If you keep calling read while at end-of-file,
             * it will keep returning zero and doing nothing else.
             * Hence, we break here.
             */
            break;
        }

        if (read < 0) {
            // retry if a signal or anything else has interrupted the read system call
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            auto err_str = fmt::format("Failed to read chunk file. File: '{}', size: '{}', offset: '{}', Error: '{}'",
                                       chunk_path, size, offset, ::strerror(errno));
            throw ChunkStorageException(errno, err_str);
        }

#ifndef NDEBUG
        if (read_total + read < size) {
            log_->debug("Read less bytes than requested: '{}'/{}. Total read was '{}'. This is not an error!", read,
                        size - read_total, size);
        }
#endif
        assert(read > 0);
        read_total += read;
    } while (read_total != size);

    // file is closed via the file handle's destructor.
    return read_total;
}

/**
* Delete all chunks starting with chunk a chunk id.
* Note eventual consistency here: While chunks are removed, there is no lock that prevents
* other processes from modifying anything in that directory.
* It is the application's responsibility to stop modifying the file while truncate is executed
*
* If an error is encountered when removing a chunk file, the function will still remove all files and
* report the error afterwards with ChunkStorageException.
 * @param file_path
 * @param chunk_start
 * @throws ChunkStorageException
 */
void ChunkStorage::trim_chunk_space(const string& file_path, gkfs::rpc::chnk_id_t chunk_start) {

    auto chunk_dir = absolute(get_chunks_dir(file_path));
    const bfs::directory_iterator end;
    auto err_flag = false;
    for (bfs::directory_iterator chunk_file(chunk_dir); chunk_file != end; ++chunk_file) {
        auto chunk_path = chunk_file->path();
        auto chunk_id = std::stoul(chunk_path.filename().c_str());
        if (chunk_id >= chunk_start) {
            auto err = unlink(chunk_path.c_str());
            if (err == -1 && errno != ENOENT) {
                err_flag = true;
                log_->warn("{}() Failed to remove chunk file. File: '{}', Error: '{}'", __func__, chunk_path.native(),
                           ::strerror(errno));
            }
        }
    }
    if (err_flag)
        throw ChunkStorageException(EIO, fmt::format("{}() One or more errors occurred when truncating '{}'", __func__,
                                                     file_path));
}

/**
 * Truncates a single chunk file to a given length
 * @param file_path
 * @param chunk_id
 * @param length
 * @throws ChunkStorageException
 */
void ChunkStorage::truncate_chunk_file(const string& file_path, gkfs::rpc::chnk_id_t chunk_id, off_t length) {
    auto chunk_path = absolute(get_chunk_path(file_path, chunk_id));
    assert(length > 0 && static_cast<gkfs::rpc::chnk_id_t>(length) <= chunksize_);
    auto ret = truncate(chunk_path.c_str(), length);
    if (ret == -1) {
        auto err_str = fmt::format("Failed to truncate chunk file. File: '{}', Error: '{}'", chunk_path,
                                   ::strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }
}

/**
 * Calls statfs on the chunk directory to get statistic on its used and free size left
 * @return ChunkStat
 * @throws ChunkStorageException
 */
ChunkStat ChunkStorage::chunk_stat() const {
    struct statfs sfs{};

    if (statfs(root_path_.c_str(), &sfs) != 0) {
        auto err_str = fmt::format("Failed to get filesystem statistic for chunk directory. Error: '{}'",
                                   ::strerror(errno));
        throw ChunkStorageException(errno, err_str);
    }

    log_->debug("Chunksize '{}', total '{}', free '{}'", sfs.f_bsize, sfs.f_blocks, sfs.f_bavail);
    auto bytes_total =
            static_cast<unsigned long long>(sfs.f_bsize) *
            static_cast<unsigned long long>(sfs.f_blocks);
    auto bytes_free =
            static_cast<unsigned long long>(sfs.f_bsize) *
            static_cast<unsigned long long>(sfs.f_bavail);
    return {chunksize_,
            bytes_total / chunksize_,
            bytes_free / chunksize_};
}

} // namespace data
} // namespace gkfs