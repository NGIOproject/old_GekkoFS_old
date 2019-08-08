/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IFS_HOOKS_HPP
#define IFS_HOOKS_HPP

#include <fcntl.h>


int hook_openat(int dirfd, const char *cpath, int flags, mode_t mode);
int hook_close(int fd);
int hook_stat(const char* path, struct stat* buf);
int hook_lstat(const char* path, struct stat* buf);
int hook_fstat(unsigned int fd, struct stat* buf);
int hook_fstatat(int dirfd, const char * cpath, struct stat * buf, int flags);
int hook_read(unsigned int fd, void* buf, size_t count);
int hook_pread(unsigned int fd, char * buf, size_t count, loff_t pos);
int hook_write(unsigned int fd, const char * buf, size_t count);
int hook_pwrite(unsigned int fd, const char * buf, size_t count, loff_t pos);
int hook_writev(unsigned long fd, const struct iovec * iov, unsigned long iovcnt);
int hook_pwritev(unsigned long fd, const struct iovec * iov, unsigned long iovcnt,
                 unsigned long pos_l, unsigned long pos_h);
int hook_unlinkat(int dirfd, const char * cpath, int flags);
int hook_symlinkat(const char * oldname, int newdfd, const char * newname);
int hook_access(const char* path, int mask);
int hook_faccessat(int dirfd, const char * cpath, int mode);
off_t hook_lseek(unsigned int fd, off_t offset, unsigned int whence);
int hook_truncate(const char *path, long length);
int hook_ftruncate(unsigned int fd, unsigned long length);
int hook_dup(unsigned int fd);
int hook_dup2(unsigned int oldfd, unsigned int newfd);
int hook_dup3(unsigned int oldfd, unsigned int newfd, int flags);
int hook_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count);
int hook_mkdirat(int dirfd, const char * cpath, mode_t mode);
int hook_fchmodat(int dirfd, const char* path, mode_t mode);
int hook_fchmod(unsigned int dirfd, mode_t mode);
int hook_chdir(const char* path);
int hook_fchdir(unsigned int fd);
int hook_getcwd(char * buf, unsigned long size);
int hook_readlinkat(int dirfd, const char * cpath, char * buf, int bufsiz);
int hook_fcntl(unsigned int fd, unsigned int cmd, unsigned long arg);
int hook_renameat(int olddfd, const char * oldname, int newdfd, const char * newname,
                  unsigned int flags);
int hook_statfs(const char * path, struct statfs * buf);
int hook_fstatfs(unsigned int fd, struct statfs * buf);


#endif
