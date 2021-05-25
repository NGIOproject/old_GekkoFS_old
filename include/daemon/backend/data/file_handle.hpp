/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_DAEMON_FILE_HANDLE_HPP
#define GEKKOFS_DAEMON_FILE_HANDLE_HPP

#include <daemon/backend/data/data_module.hpp>

extern "C" {
#include <unistd.h>
}

namespace gkfs {
namespace data {

/**
 * File handle to encapsulate a file descriptor, allowing RAII closing of the file descriptor
 */
class FileHandle {

private:

    constexpr static const int init_value{-1};

    int fd_{init_value};
    std::string path_{};

public:

    FileHandle() = default;

    explicit FileHandle(int fd, std::string path) noexcept :
            fd_(fd) {}

    FileHandle(FileHandle&& rhs) = default;

    FileHandle(const FileHandle& other) = delete;

    FileHandle& operator=(FileHandle&& rhs) = default;

    FileHandle& operator=(const FileHandle& other) = delete;

    explicit operator bool() const noexcept {
        return valid();
    }

    bool operator!() const noexcept {
        return !valid();
    }

    bool
    valid() const noexcept {
        return fd_ != init_value;
    }

    int
    native() const noexcept {
        return fd_;
    }

    /**
     * Closes file descriptor and resets it to initial value
     * @return
     */
    bool close() noexcept {
        if (fd_ != init_value) {
            if (::close(fd_) < 0) {
                GKFS_DATA_MOD->log()->warn("{}() Failed to close file descriptor '{}' path '{}' errno '{}'", __func__,
                                           fd_, path_, ::strerror(errno));
                return false;
            }
        }
        fd_ = init_value;
        return true;
    }

    ~FileHandle() {
        if (fd_ != init_value)
            close();
    }


};

} // namespace data
} // namespace gkfs


#endif //GEKKOFS_DAEMON_FILE_HANDLE_HPP
