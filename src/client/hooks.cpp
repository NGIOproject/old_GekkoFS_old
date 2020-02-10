/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include "client/hooks.hpp"
#include "client/preload.hpp"
#include "client/logging.hpp"

#include "client/adafs_functions.hpp"
#include "client/resolve.hpp"
#include "client/open_dir.hpp"
#include "global/path_util.hpp"

#include <libsyscall_intercept_hook_point.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>

static inline int with_errno(int ret) {
    return (ret < 0)? -errno : ret;
}


int hook_openat(int dirfd, const char *cpath, int flags, mode_t mode) {

    LOG(DEBUG, "{}() called with fd: {}, path: \"{}\", flags: {}, mode: {}",
        __func__, dirfd, cpath, flags, mode);

    std::string resolved;
    auto rstatus = CTX->relativize_fd_path(dirfd, cpath, resolved);
    switch(rstatus) {
        case RelativizeStatus::fd_unknown:
            return syscall_no_intercept(SYS_openat, dirfd, cpath, flags, mode);

        case RelativizeStatus::external:
            return syscall_no_intercept(SYS_openat, dirfd, resolved.c_str(), flags, mode);

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            return with_errno(adafs_open(resolved, mode, flags));

        default:
            LOG(ERROR, "{}() relativize status unknown: {}", __func__);
            return -EINVAL;
    }
}

int hook_close(int fd) {

    LOG(DEBUG, "{}() called with fd: {}", __func__, fd);

    if(CTX->file_map()->exist(fd)) {
        // No call to the daemon is required
        CTX->file_map()->remove(fd);
        return 0;
    }

    if(CTX->is_internal_fd(fd)) {
        // the client application (for some reason) is trying to close an 
        // internal fd: ignore it
        return 0;
    }

    return syscall_no_intercept(SYS_close, fd);
}

int hook_stat(const char* path, struct stat* buf) {

    LOG(DEBUG, "{}() called with path: \"{}\", buf: {}", 
        __func__, path, fmt::ptr(buf));

    std::string rel_path;
    if (CTX->relativize_path(path, rel_path, false)) {
            return with_errno(adafs_stat(rel_path, buf));
    }
    return syscall_no_intercept(SYS_stat, rel_path.c_str(), buf);
}

int hook_lstat(const char* path, struct stat* buf) {

    LOG(DEBUG, "{}() called with path: \"{}\", buf: {}", 
        __func__, path, fmt::ptr(buf));

    std::string rel_path;
    if (CTX->relativize_path(path, rel_path)) {
        return with_errno(adafs_stat(rel_path, buf));
    }
    return syscall_no_intercept(SYS_lstat, rel_path.c_str(), buf);
}

int hook_fstat(unsigned int fd, struct stat* buf) {

    LOG(DEBUG, "{}() called with fd: {}, buf: {}",
        __func__, fd, fmt::ptr(buf));

    if (CTX->file_map()->exist(fd)) {
        auto path = CTX->file_map()->get(fd)->path();
        return with_errno(adafs_stat(path, buf));
    }
    return syscall_no_intercept(SYS_fstat, fd, buf);
}

int hook_fstatat(int dirfd, const char * cpath, struct stat * buf, int flags) {

    LOG(DEBUG, "{}() called with path: \"{}\", fd: {}, buf: {}, flags: {}",
        __func__, cpath, dirfd, fmt::ptr(buf), flags);

    if(flags & AT_EMPTY_PATH) {
        LOG(ERROR, "{}() AT_EMPTY_PATH flag not supported", __func__);
        return -ENOTSUP;
    }

    std::string resolved;
    auto rstatus = CTX->relativize_fd_path(dirfd, cpath, resolved);
    switch(rstatus) {
        case RelativizeStatus::fd_unknown:
            return syscall_no_intercept(SYS_newfstatat, dirfd, cpath, buf, flags);

        case RelativizeStatus::external:
            return syscall_no_intercept(SYS_newfstatat, dirfd, resolved.c_str(), buf, flags);

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            return with_errno(adafs_stat(resolved, buf));

        default:
            LOG(ERROR, "{}() relativize status unknown: {}", __func__);
            return -EINVAL;
    }
}

int hook_read(unsigned int fd, void* buf, size_t count) {

    LOG(DEBUG, "{}() called with fd: {}, buf: {} count: {}", 
        __func__, fd, fmt::ptr(buf), count);

    if (CTX->file_map()->exist(fd)) {
        return  with_errno(adafs_read(fd, buf, count));
    }
    return syscall_no_intercept(SYS_read, fd, buf, count);
}

int hook_pread(unsigned int fd, char * buf, size_t count, loff_t pos) {

    LOG(DEBUG, "{}() called with fd: {}, buf: {}, count: {}, pos: {}",
        __func__, fd, fmt::ptr(buf), count, pos);

    if (CTX->file_map()->exist(fd)) {
        return with_errno(adafs_pread_ws(fd, buf, count, pos));
    }
    /* Since kernel 2.6: pread() became pread64(), and pwrite() became pwrite64(). */
    return syscall_no_intercept(SYS_pread64, fd, buf, count, pos);
}

int hook_write(unsigned int fd, const char * buf, size_t count) {

    LOG(DEBUG, "{}() called with fd: {}, buf: {}, count {}", 
        __func__, fd, fmt::ptr(buf), count);

    if (CTX->file_map()->exist(fd)) {
        return with_errno(adafs_write(fd, buf, count));
    }
    return syscall_no_intercept(SYS_write, fd, buf, count);
}

int hook_pwrite(unsigned int fd, const char * buf, size_t count, loff_t pos) {

    LOG(DEBUG, "{}() called with fd: {}, buf: {}, count: {}, pos: {}",
        __func__, fd, fmt::ptr(buf), count, pos);

    if (CTX->file_map()->exist(fd)) {
        return with_errno(adafs_pwrite_ws(fd, buf, count, pos));
    }
    /* Since kernel 2.6: pread() became pread64(), and pwrite() became pwrite64(). */
    return syscall_no_intercept(SYS_pwrite64, fd, buf, count, pos);
}

int hook_writev(unsigned long fd, const struct iovec * iov, unsigned long iovcnt) {

    LOG(DEBUG, "{}() called with fd: {}, iov: {}, iovcnt: {}", 
        __func__, fd, fmt::ptr(iov), iovcnt);

    if (CTX->file_map()->exist(fd)) {
        return with_errno(adafs_writev(fd, iov, iovcnt));
    }
    return syscall_no_intercept(SYS_writev, fd, iov, iovcnt);
}

int hook_pwritev(unsigned long fd, const struct iovec * iov, unsigned long iovcnt,
                 unsigned long pos_l, unsigned long pos_h) {

    LOG(DEBUG, "{}() called with fd: {}, iov: {}, iovcnt: {}, "
        "pos_l: {}," "pos_h: {}", 
        __func__, fd, fmt::ptr(iov), iovcnt, pos_l, pos_h);

    if (CTX->file_map()->exist(fd)) {
        LOG(WARNING, "{}() Not supported", __func__);
        return -ENOTSUP;
    }
    return syscall_no_intercept(SYS_pwritev, fd, iov, iovcnt);
}

int hook_unlinkat(int dirfd, const char * cpath, int flags) {

    LOG(DEBUG, "{}() called with dirfd: {}, path: \"{}\", flags: {}",
        __func__, dirfd, cpath, flags);

    if ((flags & ~AT_REMOVEDIR) != 0) {
        LOG(ERROR, "{}() Flags unknown: {}", __func__, flags);
        return -EINVAL;
    }

    std::string resolved;
    auto rstatus = CTX->relativize_fd_path(dirfd, cpath, resolved, false);
    switch(rstatus) {
        case RelativizeStatus::fd_unknown:
            return syscall_no_intercept(SYS_unlinkat, dirfd, cpath, flags);

        case RelativizeStatus::external:
            return syscall_no_intercept(SYS_unlinkat, dirfd, resolved.c_str(), flags);

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            if(flags & AT_REMOVEDIR) {
                return with_errno(adafs_rmdir(resolved));
            } else {
                return with_errno(adafs_rm_node(resolved));
            }

        default:
            LOG(ERROR, "{}() relativize status unknown: {}", __func__);
            return -EINVAL;
    }
}

int hook_symlinkat(const char * oldname, int newdfd, const char * newname) {

    LOG(DEBUG, "{}() called with oldname: \"{}\", newfd: {}, newname: \"{}\"",
        __func__, oldname, newdfd, newname);

    std::string oldname_resolved;
    if (CTX->relativize_path(oldname, oldname_resolved)) {
        LOG(WARNING, "{}() operation not supported", __func__);
        return -ENOTSUP;
    }

    std::string newname_resolved;
    auto rstatus = CTX->relativize_fd_path(newdfd, newname, newname_resolved, false);
    switch(rstatus) {
        case RelativizeStatus::fd_unknown:
            return syscall_no_intercept(SYS_symlinkat, oldname, newdfd, newname);

        case RelativizeStatus::external:
            return syscall_no_intercept(SYS_symlinkat, oldname, newdfd, newname_resolved.c_str());

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            LOG(WARNING, "{}() operation not supported", __func__);
            return -ENOTSUP;

        default:
            LOG(ERROR, "{}() relativize status unknown", __func__);
            return -EINVAL;
    }
}


int hook_access(const char* path, int mask) {

    LOG(DEBUG, "{}() called path: \"{}\", mask: {}", 
        __func__, path, mask);

    std::string rel_path;
    if (CTX->relativize_path(path, rel_path)) {
        auto ret = adafs_access(rel_path, mask);
        if(ret < 0) {
            return -errno;
        }
        return ret;
    }
    return syscall_no_intercept(SYS_access, rel_path.c_str(), mask);
}

int hook_faccessat(int dirfd, const char * cpath, int mode) {

    LOG(DEBUG, "{}() called with dirfd: {}, path: \"{}\", mode: {}",
        __func__, dirfd, cpath, mode);

    std::string resolved;
    auto rstatus = CTX->relativize_fd_path(dirfd, cpath, resolved);
    switch(rstatus) {
        case RelativizeStatus::fd_unknown:
            return syscall_no_intercept(SYS_faccessat, dirfd, cpath, mode);

        case RelativizeStatus::external:
            return syscall_no_intercept(SYS_faccessat, dirfd, resolved.c_str(), mode);

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            return with_errno(adafs_access(resolved, mode));

        default:
            LOG(ERROR, "{}() relativize status unknown: {}", __func__);
            return -EINVAL;
    }
}

off_t hook_lseek(unsigned int fd, off_t offset, unsigned int whence) {

    LOG(DEBUG, "{}() called with fd: {}, offset: {}, whence: {}", 
        __func__, fd, offset, whence);

    if (CTX->file_map()->exist(fd)) {
        auto off_ret = adafs_lseek(fd, static_cast<off64_t>(offset), whence);
        if (off_ret > std::numeric_limits<off_t>::max()) {
            return -EOVERFLOW;
        } else if(off_ret < 0) {
            return -errno;
        }
        LOG(DEBUG, "{}() returning {}", __func__, off_ret);
        return off_ret;
    }
   return syscall_no_intercept(SYS_lseek, fd, offset, whence);
}

int hook_truncate(const char* path, long length) {

    LOG(DEBUG, "{}() called with path: {}, offset: {}", 
        __func__, path, length);

    std::string rel_path;
    if (CTX->relativize_path(path, rel_path)) {
        return with_errno(adafs_truncate(rel_path, length));
    }
    return syscall_no_intercept(SYS_truncate, rel_path.c_str(), length);
}

int hook_ftruncate(unsigned int fd, unsigned long length) {

    LOG(DEBUG, "{}() called with fd: {}, offset: {}", 
        __func__, fd, length);

    if (CTX->file_map()->exist(fd)) {
        auto path = CTX->file_map()->get(fd)->path();
        return with_errno(adafs_truncate(path, length));
    }
    return syscall_no_intercept(SYS_ftruncate, fd, length);
}

int hook_dup(unsigned int fd) {

    LOG(DEBUG, "{}() called with oldfd: {}", 
        __func__, fd);

    if (CTX->file_map()->exist(fd)) {
        return with_errno(adafs_dup(fd));
    }
    return syscall_no_intercept(SYS_dup, fd);
}

int hook_dup2(unsigned int oldfd, unsigned int newfd) {

    LOG(DEBUG, "{}() called with oldfd: {}, newfd: {}", 
        __func__, oldfd, newfd);

    if (CTX->file_map()->exist(oldfd)) {
        return with_errno(adafs_dup2(oldfd, newfd));
    }
    return syscall_no_intercept(SYS_dup2, oldfd, newfd);
}

int hook_dup3(unsigned int oldfd, unsigned int newfd, int flags) {

    LOG(DEBUG, "{}() called with oldfd: {}, newfd: {}, flags: {}", 
        __func__, oldfd, newfd, flags);

    if (CTX->file_map()->exist(oldfd)) {
        // TODO implement O_CLOEXEC flag first which is used with fcntl(2)
        // It is in glibc since kernel 2.9. So maybe not that important :)
        LOG(WARNING, "{}() Not supported", __func__);
        return -ENOTSUP;
    }
    return syscall_no_intercept(SYS_dup3, oldfd, newfd, flags);
}

int hook_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count) {

    LOG(DEBUG, "{}() called with fd: {}, dirp: {}, count: {}", 
        __func__, fd, fmt::ptr(dirp), count);

    if (CTX->file_map()->exist(fd)) {
        return with_errno(getdents(fd, dirp, count));
    }
    return syscall_no_intercept(SYS_getdents, fd, dirp, count);
}


int hook_getdents64(unsigned int fd, struct linux_dirent64 *dirp, unsigned int count) {

    LOG(DEBUG, "{}() called with fd: {}, dirp: {}, count: {}", 
        __func__, fd, fmt::ptr(dirp), count);

    if (CTX->file_map()->exist(fd)) {
        return with_errno(getdents64(fd, dirp, count));
    }
    return syscall_no_intercept(SYS_getdents64, fd, dirp, count);
}


int hook_mkdirat(int dirfd, const char * cpath, mode_t mode) {

    LOG(DEBUG, "{}() called with dirfd: {}, path: \"{}\", mode: {}",
        __func__, dirfd, cpath, mode);

    std::string resolved;
    auto rstatus = CTX->relativize_fd_path(dirfd, cpath, resolved);
    switch(rstatus) {
        case RelativizeStatus::external:
            return syscall_no_intercept(SYS_mkdirat, dirfd, resolved.c_str(), mode);

        case RelativizeStatus::fd_unknown:
            return syscall_no_intercept(SYS_mkdirat, dirfd, cpath, mode);

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            return with_errno(adafs_mk_node(resolved, mode | S_IFDIR));

        default:
            LOG(ERROR, "{}() relativize status unknown: {}", __func__);
            return -EINVAL;
    }
}

int hook_fchmodat(int dirfd, const char * cpath, mode_t mode) {

    LOG(DEBUG, "{}() called dirfd: {}, path: \"{}\", mode: {}", 
        __func__, dirfd, cpath, mode);

    std::string resolved;
    auto rstatus = CTX->relativize_fd_path(dirfd, cpath, resolved);
    switch(rstatus) {
        case RelativizeStatus::fd_unknown:
            return syscall_no_intercept(SYS_fchmodat, dirfd, cpath, mode);

        case RelativizeStatus::external:
            return syscall_no_intercept(SYS_fchmodat, dirfd, resolved.c_str(), mode);

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            LOG(WARNING, "{}() operation not supported", __func__);
            return -ENOTSUP;

        default:
            LOG(ERROR, "{}() relativize status unknown: {}", __func__);
            return -EINVAL;
    }
}

int hook_fchmod(unsigned int fd, mode_t mode) {

    LOG(DEBUG, "{}() called with fd: {}, mode: {}", 
        __func__, fd, mode);

    if (CTX->file_map()->exist(fd)) {
        LOG(WARNING, "{}() operation not supported", __func__);
        return -ENOTSUP;
    }
    return syscall_no_intercept(SYS_fchmod, fd, mode);
}

int hook_chdir(const char * path) {

    LOG(DEBUG, "{}() called with path: \"{}\"", 
        __func__, path);

    std::string rel_path;
    bool internal = CTX->relativize_path(path, rel_path);
    if (internal) {
        //path falls in our namespace
        auto md = adafs_metadata(rel_path);
        if (md == nullptr) {
            LOG(ERROR, "{}() path does not exists", __func__);
            return -ENOENT;
        }
        if(!S_ISDIR(md->mode())) {
            LOG(ERROR, "{}() path is not a directory", __func__);
            return -ENOTDIR;
        }
        //TODO get complete path from relativize_path instead of
        // removing mountdir and then adding again here
        rel_path.insert(0, CTX->mountdir());
        if (has_trailing_slash(rel_path)) {
            // open_dir is '/'
            rel_path.pop_back();
        }
    }
    try {
        set_cwd(rel_path, internal);
    } catch (const std::system_error& se) {
        return -(se.code().value());
    }
    return 0;
}

int hook_fchdir(unsigned int fd) {

    LOG(DEBUG, "{}() called with fd: {}", 
        __func__, fd);

    if (CTX->file_map()->exist(fd)) {
        auto open_dir = CTX->file_map()->get_dir(fd);
        if (open_dir == nullptr) {
            //Cast did not succeeded: open_file is a regular file
            LOG(ERROR, "{}() file descriptor refers to a normal file: '{}'",
                    __func__, open_dir->path());
            return -EBADF;
        }

        std::string new_path = CTX->mountdir() + open_dir->path();
        if (has_trailing_slash(new_path)) {
            // open_dir is '/'
            new_path.pop_back();
        }
        try {
            set_cwd(new_path, true);
        } catch (const std::system_error& se) {
            return -(se.code().value());
        }
    } else {
        long ret = syscall_no_intercept(SYS_fchdir, fd);
        if (ret < 0) {
            throw std::system_error(syscall_error_code(ret),
                                    std::system_category(),
                                    "Failed to change directory (fchdir syscall)");
        }
        unset_env_cwd();
        CTX->cwd(get_sys_cwd());
    }
    return 0;
}

int hook_getcwd(char * buf, unsigned long size) {

    LOG(DEBUG, "{}() called with buf: {}, size: {}", 
        __func__, fmt::ptr(buf), size);

    if(CTX->cwd().size() + 1 > size) {
        LOG(ERROR, "{}() buffer too small to host current working dir", __func__);
        return -ERANGE;
    }

    strcpy(buf, CTX->cwd().c_str());
    return (CTX->cwd().size() + 1);
}

int hook_readlinkat(int dirfd, const char * cpath, char * buf, int bufsiz) {

    LOG(DEBUG, "{}() called with dirfd: {}, path \"{}\", buf: {}, bufsize: {}",
        __func__, dirfd, cpath, fmt::ptr(buf), bufsiz);

    std::string resolved;
    auto rstatus = CTX->relativize_fd_path(dirfd, cpath, resolved, false);
    switch(rstatus) {
        case RelativizeStatus::fd_unknown:
            return syscall_no_intercept(SYS_readlinkat, dirfd, cpath, buf, bufsiz);

        case RelativizeStatus::external:
            return syscall_no_intercept(SYS_readlinkat, dirfd, resolved.c_str(), buf, bufsiz);

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            LOG(WARNING, "{}() not supported", __func__);
            return -ENOTSUP;

        default:
            LOG(ERROR, "{}() relativize status unknown: {}", __func__);
            return -EINVAL;
    }
}

int hook_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg) {

    LOG(DEBUG, "{}() called with fd: {}, cmd: {}, arg: {}", 
        __func__, fd, cmd, arg);

    if (!CTX->file_map()->exist(fd)) {
        return syscall_no_intercept(SYS_fcntl, fd, cmd, arg);
    }
    int ret;
    switch (cmd) {

        case F_DUPFD:
            LOG(DEBUG, "{}() F_DUPFD on fd {}", __func__, fd);
            return with_errno(adafs_dup(fd));

        case F_DUPFD_CLOEXEC:
            LOG(DEBUG, "{}() F_DUPFD_CLOEXEC on fd {}", __func__, fd);
            ret = adafs_dup(fd);
            if(ret == -1) {
                return -errno;
            }
            CTX->file_map()->get(fd)->set_flag(OpenFile_flags::cloexec, true);
            return ret;

        case F_GETFD:
            LOG(DEBUG, "{}() F_GETFD on fd {}", __func__, fd);
            if(CTX->file_map()->get(fd)
                    ->get_flag(OpenFile_flags::cloexec)) {
                return FD_CLOEXEC;
            }
            return 0;

        case F_GETFL:
            LOG(DEBUG, "{}() F_GETFL on fd {}", __func__, fd);
            ret = 0;
            if(CTX->file_map()->get(fd)
                    ->get_flag(OpenFile_flags::rdonly)) {
                ret |= O_RDONLY;
            }
            if(CTX->file_map()->get(fd)
                    ->get_flag(OpenFile_flags::wronly)) {
                ret |= O_WRONLY;
            }
            if(CTX->file_map()->get(fd)
                    ->get_flag(OpenFile_flags::rdwr)) {
                ret |= O_RDWR;
            }
            return ret;

        case F_SETFD:
            LOG(DEBUG, "{}() [fd: {}, cmd: F_SETFD, FD_CLOEXEC: {}]",
                __func__, fd, (arg & FD_CLOEXEC));
            CTX->file_map()->get(fd)
                ->set_flag(OpenFile_flags::cloexec, (arg & FD_CLOEXEC));
            return 0;


        default:
            LOG(ERROR, "{}() unrecognized command {} on fd {}",
                    __func__, cmd, fd);
            return -ENOTSUP;
    }
}

int hook_renameat(int olddfd, const char * oldname,
                  int newdfd, const char * newname,
                  unsigned int flags) {

    LOG(DEBUG, "{}() called with olddfd: {}, oldname: \"{}\", newfd: {}, "
        "newname \"{}\", flags {}", 
        __func__, olddfd, oldname, newdfd, newname, flags);

    const char * oldpath_pass;
    std::string oldpath_resolved;
    auto oldpath_status = CTX->relativize_fd_path(olddfd, oldname, oldpath_resolved);
    switch(oldpath_status) {
        case RelativizeStatus::fd_unknown:
            oldpath_pass = oldname;
            break;

        case RelativizeStatus::external:
            oldpath_pass = oldpath_resolved.c_str();
            break;

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            LOG(WARNING, "{}() not supported", __func__);
            return -ENOTSUP;

        default:
            LOG(ERROR, "{}() relativize status unknown", __func__);
            return -EINVAL;
    }

    const char * newpath_pass;
    std::string newpath_resolved;
    auto newpath_status = CTX->relativize_fd_path(newdfd, newname, newpath_resolved);
    switch(newpath_status) {
        case RelativizeStatus::fd_unknown:
            newpath_pass = newname;
            break;

        case RelativizeStatus::external:
            newpath_pass = newpath_resolved.c_str();
            break;

        case RelativizeStatus::fd_not_a_dir:
            return -ENOTDIR;

        case RelativizeStatus::internal:
            LOG(WARNING, "{}() not supported", __func__);
            return -ENOTSUP;

        default:
            LOG(ERROR, "{}() relativize status unknown", __func__);
            return -EINVAL;
    }

   return syscall_no_intercept(SYS_renameat2, olddfd, oldpath_pass, newdfd, newpath_pass, flags);
}

int hook_statfs(const char * path, struct statfs * buf) {

    LOG(DEBUG, "{}() called with path: \"{}\", buf: {}", 
        __func__, path, fmt::ptr(buf));

    std::string rel_path;
    if (CTX->relativize_path(path, rel_path)) {
        return with_errno(adafs_statfs(buf));
    }
    return syscall_no_intercept(SYS_statfs, rel_path.c_str(), buf);
}

int hook_fstatfs(unsigned int fd, struct statfs * buf) {

    LOG(DEBUG, "{}() called with fd: {}, buf: {}", 
        __func__, fd, fmt::ptr(buf));

    if (CTX->file_map()->exist(fd)) {
        return with_errno(adafs_statfs(buf));
    }
    return syscall_no_intercept(SYS_fstatfs, fd, buf);
}
