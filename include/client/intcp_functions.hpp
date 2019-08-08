/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef IFS_INTCP_FUNCTIONS_HPP
#define IFS_INTCP_FUNCTIONS_HPP

#include <dirent.h>

extern "C" {

# define weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));

# define strong_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((alias (#name)));

/**
 * In the glibc headers the following two functions (readdir & opendir)
 * marks the @dirp parameter with a non-null attribute.
 * If we try to implement them directly instead of the weak aliased function,
 * the compiler will assume  that the parameter is actually null and
 * will optimized expression like `(dirp == nullptr)`.
*/

struct dirent* intcp_readdir(DIR* dirp);
weak_alias(intcp_readdir, readdir)

int intcp_dirfd(DIR* dirp);
weak_alias(intcp_dirfd, dirfd)

int intcp_closedir(DIR* dirp);
weak_alias(intcp_closedir, closedir)

size_t intcp_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
strong_alias(intcp_fread, fread)
strong_alias(intcp_fread, fread_unlocked)
size_t intcp_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
strong_alias(intcp_fwrite, fwrite)
strong_alias(intcp_fwrite, fwrite_unlocked)

int intcp_open(const char* path, int flags, ...);
strong_alias(intcp_open, open)
strong_alias(intcp_open, __open_2)
int intcp_open64(const char* path, int flags, ...);
strong_alias(intcp_open64, open64)
strong_alias(intcp_open64, __open64_2)
int intcp_openat(int dirfd, const char *cpath, int flags, ...);
strong_alias(intcp_openat, openat)
strong_alias(intcp_openat, __openat_2)
int intcp_openat64(int dirfd, const char *path, int flags, ...);
strong_alias(intcp_openat64, openat64)
strong_alias(intcp_openat64, __openat64_2)
int intcp_symlink(const char* oldname, const char* newname) noexcept;
strong_alias(intcp_symlink, symlink)
strong_alias(intcp_symlink, __symlink)
int intcp_symlinkat(const char* oldname, int newfd, const char* newname) noexcept;
strong_alias(intcp_symlinkat, symlinkat)
ssize_t intcp_readlink(const char * cpath, char * buf, size_t bufsize) noexcept;
strong_alias(intcp_readlink, readlink)
ssize_t intcp_readlinkat(int dirfd, const char * cpath, char * buf, size_t bufsize) noexcept;
strong_alias(intcp_readlinkat, readlinkat)

int intcp_statvfs(const char *path, struct statvfs *buf) noexcept;
strong_alias(intcp_statvfs, statvfs)
int intcp_fstatvfs(int fd, struct statvfs *buf) noexcept;
strong_alias(intcp_fstatvfs, fstatvfs)

#endif // IFS_INTCP_FUNCTIONS_HPP

} // extern C
