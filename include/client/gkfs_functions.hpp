/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_GKFS_FUNCTIONS_HPP
#define GEKKOFS_GKFS_FUNCTIONS_HPP

#include <client/open_file_map.hpp>
#include <global/metadata.hpp>

struct statfs;
struct statvfs;
struct linux_dirent;
struct linux_dirent64;

namespace gkfs {
namespace syscall {

int gkfs_open(const std::string& path, mode_t mode, int flags);

int gkfs_create(const std::string& path, mode_t mode);

int gkfs_remove(const std::string& path);

// Implementation of access,
// Follow links is true by default 
int gkfs_access(const std::string& path, int mask, bool follow_links = true);

// Implementation of stat, 
// Follow links is true by default 
int gkfs_stat(const std::string& path, struct stat* buf, bool follow_links = true);

// Implementation of statx, it uses the normal stat and maps the information to the statx structure
// Follow links is true by default 
#ifdef STATX_TYPE

int gkfs_statx(int dirfd, const std::string& path, int flags, unsigned int mask, struct statx* buf,
               bool follow_links = true);

#endif

int gkfs_statfs(struct statfs* buf);

int gkfs_statvfs(struct statvfs* buf);

off64_t gkfs_lseek(unsigned int fd, off64_t offset, unsigned int whence);

off64_t gkfs_lseek(std::shared_ptr<gkfs::filemap::OpenFile> gkfs_fd, off64_t offset, unsigned int whence);

int gkfs_truncate(const std::string& path, off_t offset);

int gkfs_truncate(const std::string& path, off_t old_size, off_t new_size);

int gkfs_dup(int oldfd);

int gkfs_dup2(int oldfd, int newfd);

#ifdef HAS_SYMLINKS

int gkfs_mk_symlink(const std::string& path, const std::string& target_path);

int gkfs_readlink(const std::string& path, char* buf, int bufsize);

#endif

ssize_t gkfs_pwrite(std::shared_ptr<gkfs::filemap::OpenFile> file,
                    const char* buf, size_t count, off64_t offset);

ssize_t gkfs_pwrite_ws(int fd, const void* buf, size_t count, off64_t offset);

ssize_t gkfs_write(int fd, const void* buf, size_t count);

ssize_t gkfs_pwritev(int fd, const struct iovec* iov, int iovcnt, off_t offset);

ssize_t gkfs_writev(int fd, const struct iovec* iov, int iovcnt);

ssize_t gkfs_pread(std::shared_ptr<gkfs::filemap::OpenFile> file, char* buf, size_t count, off64_t offset);

ssize_t gkfs_pread_ws(int fd, void* buf, size_t count, off64_t offset);

ssize_t gkfs_read(int fd, void* buf, size_t count);

ssize_t gkfs_readv(int fd, const struct iovec* iov, int iovcnt);

ssize_t gkfs_preadv(int fd, const struct iovec* iov, int iovcnt, off_t offset);

int gkfs_opendir(const std::string& path);

int gkfs_getdents(unsigned int fd, struct linux_dirent* dirp, unsigned int count);

int gkfs_getdents64(unsigned int fd, struct linux_dirent64* dirp, unsigned int count);

int gkfs_rmdir(const std::string& path);

} // namespace syscall
} // namespace gkfs

#endif //GEKKOFS_GKFS_FUNCTIONS_HPP
