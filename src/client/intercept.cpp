/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include "client/intercept.hpp"
#include "client/preload.hpp"
#include "client/hooks.hpp"

#include <client/logging.hpp>

#include <libsyscall_intercept_hook_point.h>
#include <syscall.h>
#include <errno.h>
#include <boost/optional.hpp>
#include <sys/types.h>
#include <sys/socket.h>

#include <printf.h>

#include <fmt/format.h>

static thread_local bool reentrance_guard_flag;
static thread_local gkfs::syscall::info saved_syscall_info;


static constexpr void
save_current_syscall_info(gkfs::syscall::info info) {
    saved_syscall_info = info;
}

static constexpr void
reset_current_syscall_info() {
    saved_syscall_info = gkfs::syscall::no_info;
}

static inline gkfs::syscall::info
get_current_syscall_info() {
    return saved_syscall_info;
}


/*
 * hook_internal -- interception hook for internal syscalls
 *
 * This hook is basically used to keep track of file descriptors created 
 * internally by the library itself. This is important because some 
 * applications (e.g. ssh) may attempt to close all open file descriptors
 * which would leave the library internals in an inconsistent state.
 * We forward syscalls to the kernel but we keep track of any syscalls that may
 * create or destroy a file descriptor so that we can mark them as 'internal'.
 */
static inline int 
hook_internal(long syscall_number,
              long arg0, long arg1, long arg2,
              long arg3, long arg4, long arg5,
              long *result) {

#if defined(GKFS_ENABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
	const long args[gkfs::syscall::MAX_ARGS] = {
	    arg0, arg1, arg2, arg3, arg4, arg5
	};
#endif

    LOG(SYSCALL, gkfs::syscall::from_internal_code | gkfs::syscall::to_hook |
        gkfs::syscall::not_executed, syscall_number, args);

    switch (syscall_number) {

        case SYS_open:
            *result = syscall_no_intercept(syscall_number, 
                                reinterpret_cast<char*>(arg0),
                                static_cast<int>(arg1),
                                static_cast<mode_t>(arg2));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_creat:
            *result = syscall_no_intercept(syscall_number,
                                reinterpret_cast<const char*>(arg0),
                                O_WRONLY | O_CREAT | O_TRUNC,
                                static_cast<mode_t>(arg1));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_openat:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<int>(arg0),
                                reinterpret_cast<const char*>(arg1),
                                static_cast<int>(arg2),
                                static_cast<mode_t>(arg3));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_epoll_create:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<int>(arg0));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_epoll_create1:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<int>(arg0));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_dup:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<unsigned int>(arg0));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_dup2:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<unsigned int>(arg0),
                                static_cast<unsigned int>(arg1));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_dup3:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<unsigned int>(arg0),
                                static_cast<unsigned int>(arg1),
                                static_cast<int>(arg2));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_inotify_init:
            *result = syscall_no_intercept(syscall_number);

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_inotify_init1:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<int>(arg0));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }

            break;

        case SYS_perf_event_open:
            *result = syscall_no_intercept(syscall_number,
                                reinterpret_cast<struct perf_event_attr*>(arg0),
                                static_cast<pid_t>(arg1),
                                static_cast<int>(arg2),
                                static_cast<int>(arg3),
                                static_cast<unsigned long>(arg4));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_signalfd:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<int>(arg0),
                                reinterpret_cast<const sigset_t*>(arg1));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_signalfd4:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<int>(arg0),
                                reinterpret_cast<const sigset_t*>(arg1),
                                static_cast<int>(arg2));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_timerfd_create:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<int>(arg0),
                                static_cast<int>(arg1));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;


        case SYS_socket:
            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0),
                                           static_cast<int>(arg1),
                                           static_cast<int>(arg2));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_socketpair:

            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0),
                                           static_cast<int>(arg1),
                                           static_cast<int>(arg2),
                                           reinterpret_cast<int*>(arg3));

            if(*result >= 0) {
                reinterpret_cast<int*>(arg3)[0] = 
                    CTX->register_internal_fd(reinterpret_cast<int*>(arg3)[0]);
                reinterpret_cast<int*>(arg3)[1] = 
                    CTX->register_internal_fd(reinterpret_cast<int*>(arg3)[1]);
            }

            break;

        case SYS_pipe:
            *result = syscall_no_intercept(syscall_number,
                                           reinterpret_cast<int*>(arg0));

            if(*result >= 0) {
                reinterpret_cast<int*>(arg0)[0] = 
                    CTX->register_internal_fd(reinterpret_cast<int*>(arg0)[0]);
                reinterpret_cast<int*>(arg0)[1] = 
                    CTX->register_internal_fd(reinterpret_cast<int*>(arg0)[1]);
            }

            break;

        case SYS_pipe2:

            *result = syscall_no_intercept(syscall_number,
                                           reinterpret_cast<int*>(arg0),
                                           static_cast<int>(arg1));
            if(*result >= 0) {
                reinterpret_cast<int*>(arg0)[0] = 
                    CTX->register_internal_fd(reinterpret_cast<int*>(arg0)[0]);
                reinterpret_cast<int*>(arg0)[1] = 
                    CTX->register_internal_fd(reinterpret_cast<int*>(arg0)[1]);
            }

            break;

        case SYS_eventfd:

            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_eventfd2:

            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0),
                                           static_cast<int>(arg1));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_recvmsg:
        {
            *result = syscall_no_intercept(syscall_number,
                                        static_cast<int>(arg0),
                                        reinterpret_cast<struct msghdr*>(arg1),
                                        static_cast<int>(arg2));

            // The recvmsg() syscall can receive file descriptors from another
            // process that the kernel automatically adds to the client's fds
            // as if dup2 had been called. Whenever that happens, we need to
            // make sure that we register these additional fds as internal, or
            // we could inadvertently overwrite them
            if(*result >= 0) {
                auto* hdr = reinterpret_cast<struct msghdr*>(arg1);
                struct cmsghdr* cmsg = CMSG_FIRSTHDR(hdr);

                for(; cmsg != NULL; cmsg = CMSG_NXTHDR(hdr, cmsg)) {
                    if(cmsg->cmsg_type == SCM_RIGHTS) {

                        size_t nfd = cmsg->cmsg_len > CMSG_LEN(0) ? 
                            (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int) : 
                            0;

                        int* fds =
                            reinterpret_cast<int*>(CMSG_DATA(cmsg));

                        for(size_t i = 0; i < nfd; ++i) {
                            LOG(DEBUG, "recvmsg() provided extra fd {}", fds[i]);

                            // ensure we update the fds in cmsg
                            // if they have been relocated
                            fds[i] = CTX->register_internal_fd(fds[i]);
                        }
                    }
                }
            }

            break;
        }

        case SYS_accept:
            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0),
                                           reinterpret_cast<struct sockaddr*>(arg1),
                                           reinterpret_cast<int*>(arg2));

            if(*result >= 0) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_fcntl:
            *result = syscall_no_intercept(syscall_number,
                                           static_cast<int>(arg0),
                                           static_cast<int>(arg1),
                                           arg2);

            if(*result >= 0 &&
               (static_cast<int>(arg1) == F_DUPFD ||
                static_cast<int>(arg1) == F_DUPFD_CLOEXEC)) {
                *result = CTX->register_internal_fd(*result);
            }
            break;

        case SYS_close:
            *result = syscall_no_intercept(syscall_number,
                                static_cast<int>(arg0));

            if(*result == 0) {
                CTX->unregister_internal_fd(arg0);
            }
            break;

        default:
            // ignore any other syscalls, i.e.: pass them on to the kernel
            // (syscalls forwarded to the kernel that return are logged in 
            // hook_forwarded_syscall())
            ::save_current_syscall_info(
                    gkfs::syscall::from_internal_code | 
                    gkfs::syscall::to_kernel |
                    gkfs::syscall::not_executed);
            return gkfs::syscall::forward_to_kernel;
    }

    LOG(SYSCALL, gkfs::syscall::from_internal_code |
        gkfs::syscall::to_hook | gkfs::syscall::executed,
        syscall_number, args, *result);

    return gkfs::syscall::hooked;

}

/*
 * hook -- interception hook for application syscalls
 *
 * This hook is used to implement any application filesystem-related syscalls.
 */
static inline 
int hook(long syscall_number,
         long arg0, long arg1, long arg2,
         long arg3, long arg4, long arg5,
         long *result) {

#if defined(GKFS_ENABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
	const long args[gkfs::syscall::MAX_ARGS] = {
	    arg0, arg1, arg2, arg3, arg4, arg5
	};
#endif

    LOG(SYSCALL, gkfs::syscall::from_external_code | 
        gkfs::syscall::to_hook | gkfs::syscall::not_executed,
        syscall_number, args);

    switch (syscall_number) {

        case SYS_execve:
            *result = syscall_no_intercept(syscall_number,
                                reinterpret_cast<const char*>(arg0),
                                reinterpret_cast<const char* const*>(arg1),
                                reinterpret_cast<const char* const*>(arg2));
            break;

#ifdef SYS_execveat
        case SYS_execveat:
            *result = syscall_no_intercept(syscall_number,
                                arg0,
                                reinterpret_cast<const char*>(arg1),
                                reinterpret_cast<const char* const*>(arg2),
                                reinterpret_cast<const char* const*>(arg3),
                                arg4);
            break;
#endif

        case SYS_open:
            *result = hook_openat(AT_FDCWD,
                                reinterpret_cast<char*>(arg0),
                                static_cast<int>(arg1),
                                static_cast<mode_t>(arg2));
            break;

        case SYS_creat:
            *result = hook_openat(AT_FDCWD,
                                reinterpret_cast<const char*>(arg0),
                                O_WRONLY | O_CREAT | O_TRUNC,
                                static_cast<mode_t>(arg1));
            break;

        case SYS_openat:
            *result = hook_openat(static_cast<int>(arg0),
                                reinterpret_cast<const char*>(arg1),
                                static_cast<int>(arg2),
                                static_cast<mode_t>(arg3));
            break;

        case SYS_close:
            *result = hook_close(static_cast<int>(arg0));
            break;

        case SYS_stat:
            *result = hook_stat(reinterpret_cast<char*>(arg0),
                                reinterpret_cast<struct stat*>(arg1));
            break;

        case SYS_lstat:
            *result = hook_lstat(reinterpret_cast<char*>(arg0),
                                 reinterpret_cast<struct stat*>(arg1));
            break;

        case SYS_fstat:
            *result = hook_fstat(static_cast<int>(arg0),
                                reinterpret_cast<struct stat*>(arg1));
            break;

        case SYS_newfstatat:
            *result = hook_fstatat(static_cast<int>(arg0),
                                reinterpret_cast<const char*>(arg1),
                                reinterpret_cast<struct stat *>(arg2),
                                static_cast<int>(arg3));
            break;

        case SYS_read:
            *result = hook_read(static_cast<unsigned int>(arg0),
                                reinterpret_cast<void*>(arg1),
                                static_cast<size_t>(arg2));
            break;

        case SYS_pread64:
            *result = hook_pread(static_cast<unsigned int>(arg0),
                                reinterpret_cast<char *>(arg1),
                                static_cast<size_t>(arg2),
                                static_cast<loff_t>(arg3));
            break;

        case SYS_pwrite64:
            *result = hook_pwrite(static_cast<unsigned int>(arg0),
                                reinterpret_cast<const char *>(arg1),
                                static_cast<size_t>(arg2),
                                static_cast<loff_t>(arg3));
            break;
        case SYS_write:
            *result = hook_write(static_cast<unsigned int>(arg0),
                                reinterpret_cast<const char *>(arg1),
                                static_cast<size_t>(arg2));
            break;

        case SYS_writev:
            *result = hook_writev(static_cast<unsigned long>(arg0),
                                reinterpret_cast<const struct iovec *>(arg1),
                                static_cast<unsigned long>(arg2));
            break;

        case SYS_pwritev:
            *result = hook_pwritev(static_cast<unsigned long>(arg0),
                                reinterpret_cast<const struct iovec *>(arg1),
                                static_cast<unsigned long>(arg2),
                                static_cast<unsigned long>(arg3),
                                static_cast<unsigned long>(arg4));
            break;

        case SYS_unlink:
            *result = hook_unlinkat(AT_FDCWD,
                                    reinterpret_cast<const char *>(arg0),
                                    0);
            break;

        case SYS_unlinkat:
            *result = hook_unlinkat(static_cast<int>(arg0),
                                    reinterpret_cast<const char*>(arg1),
                                    static_cast<int>(arg2));
            break;

        case SYS_rmdir:
            *result = hook_unlinkat(AT_FDCWD,
                                    reinterpret_cast<const char *>(arg0),
                                    AT_REMOVEDIR);
            break;

        case SYS_symlink:
            *result = hook_symlinkat(reinterpret_cast<const char *>(arg0),
                                    AT_FDCWD,
                                    reinterpret_cast<const char *>(arg1));
            break;

        case SYS_symlinkat:
            *result = hook_symlinkat(reinterpret_cast<const char *>(arg0),
                                    static_cast<int>(arg1),
                                    reinterpret_cast<const char *>(arg2));
            break;

        case SYS_access:
            *result = hook_access(reinterpret_cast<const char*>(arg0),
                                static_cast<int>(arg1));
            break;

        case SYS_faccessat:
            *result = hook_faccessat(static_cast<int>(arg0),
                                    reinterpret_cast<const char*>(arg1),
                                    static_cast<int>(arg2));
            break;

        case SYS_lseek:
            *result = hook_lseek(static_cast<unsigned int>(arg0),
                                static_cast<off_t>(arg1),
                                static_cast<unsigned int>(arg2));
            break;

        case SYS_truncate:
            *result = hook_truncate(reinterpret_cast<const char*>(arg0),
                                    static_cast<long>(arg1));
            break;

        case SYS_ftruncate:
            *result = hook_ftruncate(static_cast<unsigned int>(arg0),
                                    static_cast<unsigned long>(arg1));
            break;

        case SYS_dup:
            *result = hook_dup(static_cast<unsigned int>(arg0));
            break;

        case SYS_dup2:
            *result = hook_dup2(static_cast<unsigned int>(arg0),
                                static_cast<unsigned int>(arg1));
            break;

        case SYS_dup3:
            *result = hook_dup3(static_cast<unsigned int>(arg0),
                                static_cast<unsigned int>(arg1),
                                static_cast<int>(arg2));
            break;

        case SYS_getdents:
            *result = hook_getdents(static_cast<unsigned int>(arg0),
                                    reinterpret_cast<struct linux_dirent *>(arg1),
                                    static_cast<unsigned int>(arg2));
            break;

        case SYS_getdents64:
            *result = hook_getdents64(static_cast<unsigned int>(arg0),
                                      reinterpret_cast<struct linux_dirent64 *>(arg1),
                                      static_cast<unsigned int>(arg2));
            break;

        case SYS_mkdirat:
            *result = hook_mkdirat(static_cast<unsigned int>(arg0),
                                   reinterpret_cast<const char *>(arg1),
                                   static_cast<mode_t>(arg2));
            break;

        case SYS_mkdir:
            *result = hook_mkdirat(AT_FDCWD,
                                reinterpret_cast<const char *>(arg0),
                                static_cast<mode_t>(arg1));
            break;

        case SYS_chmod:
            *result = hook_fchmodat(AT_FDCWD,
                                    reinterpret_cast<char*>(arg0),
                                    static_cast<mode_t>(arg1));
            break;

        case SYS_fchmod:
            *result = hook_fchmod(static_cast<unsigned int>(arg0),
                                  static_cast<mode_t>(arg1));
            break;

        case SYS_fchmodat:
            *result = hook_fchmodat(static_cast<unsigned int>(arg0),
                                    reinterpret_cast<char*>(arg1),
                                    static_cast<mode_t>(arg2));
            break;

        case SYS_chdir:
            *result = hook_chdir(reinterpret_cast<const char *>(arg0));
            break;

        case SYS_fchdir:
            *result = hook_fchdir(static_cast<unsigned int>(arg0));
            break;

        case SYS_getcwd:
            *result = hook_getcwd(reinterpret_cast<char *>(arg0),
                                  static_cast<unsigned long>(arg1));
            break;

        case SYS_readlink:
            *result = hook_readlinkat(AT_FDCWD,
                                      reinterpret_cast<const char *>(arg0),
                                      reinterpret_cast<char *>(arg1),
                                      static_cast<int>(arg2));
            break;

        case SYS_readlinkat:
            *result = hook_readlinkat(static_cast<int>(arg0),
                                      reinterpret_cast<const char *>(arg1),
                                      reinterpret_cast<char *>(arg2),
                                      static_cast<int>(arg3));
            break;

        case SYS_fcntl:
            *result = hook_fcntl(static_cast<unsigned int>(arg0),
                                 static_cast<unsigned int>(arg1),
                                 static_cast<unsigned long>(arg2));
            break;

        case SYS_rename:
            *result = hook_renameat(AT_FDCWD,
                                    reinterpret_cast<const char *>(arg0),
                                    AT_FDCWD,
                                    reinterpret_cast<const char *>(arg1),
                                    0);
            break;

        case SYS_renameat:
            *result = hook_renameat(static_cast<int>(arg0),
                                    reinterpret_cast<const char *>(arg1),
                                    static_cast<int>(arg2),
                                    reinterpret_cast<const char *>(arg3),
                                    0);
            break;

        case SYS_renameat2:
            *result = hook_renameat(static_cast<int>(arg0),
                                    reinterpret_cast<const char *>(arg1),
                                    static_cast<int>(arg2),
                                    reinterpret_cast<const char *>(arg3),
                                    static_cast<unsigned int>(arg4));
            break;

        case SYS_fstatfs:
            *result = hook_fstatfs(static_cast<unsigned int>(arg0),
                                reinterpret_cast<struct statfs *>(arg1));
            break;

        case SYS_statfs:
            *result = hook_statfs(reinterpret_cast<const char *>(arg0),
                                reinterpret_cast<struct statfs *>(arg1));
            break;

        default:
            // ignore any other syscalls, i.e.: pass them on to the kernel
            // (syscalls forwarded to the kernel that return are logged in 
            // hook_forwarded_syscall())
            ::save_current_syscall_info(
                    gkfs::syscall::from_external_code | 
                    gkfs::syscall::to_kernel |
                    gkfs::syscall::not_executed);
            return gkfs::syscall::forward_to_kernel;
    }

    LOG(SYSCALL, gkfs::syscall::from_external_code |
        gkfs::syscall::to_hook | gkfs::syscall::executed,
        syscall_number, args, *result);

    return gkfs::syscall::hooked;
}

static void
hook_forwarded_syscall(long syscall_number,
                       long arg0, long arg1, long arg2,
                       long arg3, long arg4, long arg5,
                       long result)
{

    if(::get_current_syscall_info() == gkfs::syscall::no_info) {
        return;
    }

#if defined(GKFS_ENABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
	const long args[gkfs::syscall::MAX_ARGS] = {
	    arg0, arg1, arg2, arg3, arg4, arg5
	};
#endif

    LOG(SYSCALL, 
        ::get_current_syscall_info() | 
        gkfs::syscall::executed, 
        syscall_number, args, result);

    ::reset_current_syscall_info();
}

static void
hook_clone_at_child(unsigned long flags, 
                    void* child_stack,
		            int* ptid, 
		            int* ctid,
		            long newtls) {

#if defined(GKFS_ENABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
    const long args[gkfs::syscall::MAX_ARGS] = {
        static_cast<long>(flags), 
        reinterpret_cast<long>(child_stack), 
        reinterpret_cast<long>(ptid),
        reinterpret_cast<long>(ctid),
        static_cast<long>(newtls), 
        0};
#endif

    reentrance_guard_flag = true;

    LOG(SYSCALL, 
        ::get_current_syscall_info() | 
        gkfs::syscall::executed, 
        SYS_clone, args, 0);

    reentrance_guard_flag = false;
}

static void
hook_clone_at_parent(unsigned long flags, 
                     void* child_stack,
		             int* ptid, 
		             int* ctid,
 		             long newtls, 
 		             long returned_pid) {

#if defined(GKFS_ENABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
    const long args[gkfs::syscall::MAX_ARGS] = {
        static_cast<long>(flags), 
        reinterpret_cast<long>(child_stack), 
        reinterpret_cast<long>(ptid),
        reinterpret_cast<long>(ctid),
        static_cast<long>(newtls), 
        0};
#endif

    reentrance_guard_flag = true;

    LOG(SYSCALL, 
        ::get_current_syscall_info() | 
        gkfs::syscall::executed, 
        SYS_clone, args, returned_pid);

    reentrance_guard_flag = false;
}


int
internal_hook_guard_wrapper(long syscall_number,
                            long arg0, long arg1, long arg2,
                            long arg3, long arg4, long arg5,
                            long *syscall_return_value) {
    assert(CTX->interception_enabled());


    if (reentrance_guard_flag) {
        ::save_current_syscall_info(
                gkfs::syscall::from_internal_code | 
                gkfs::syscall::to_kernel |
                gkfs::syscall::not_executed);
        return gkfs::syscall::forward_to_kernel;
    }

    int was_hooked = 0;

    reentrance_guard_flag = true;
    int oerrno = errno;
    was_hooked = hook_internal(syscall_number,
                               arg0, arg1, arg2, arg3, arg4, arg5,
                               syscall_return_value);
    errno = oerrno;
    reentrance_guard_flag = false;

    return was_hooked;
}


/*
 * hook_guard_wrapper -- a wrapper which can notice reentrance.
 *
 * The reentrance_guard_flag flag allows the library to distinguish the hooking
 * of its own syscalls. E.g. while handling an open() syscall,
 * libgkfs_intercept might call fopen(), which in turn uses an open()
 * syscall internally. This internally used open() syscall is once again
 * forwarded to libgkfs_intercept, but using this flag we can notice this
 * case of reentering itself.
 *
 * XXX This approach still contains a very significant bug, as libgkfs_intercept
 * being called inside a signal handler might easily forward a mock fd to the
 * kernel.
 */
int
hook_guard_wrapper(long syscall_number,
                   long arg0, long arg1, long arg2,
                   long arg3, long arg4, long arg5,
                   long *syscall_return_value) {

    assert(CTX->interception_enabled());

    int was_hooked = 0;

    if (reentrance_guard_flag) {
        int oerrno = errno;
        was_hooked =  hook_internal(syscall_number,
                                    arg0, arg1, arg2, arg3, arg4, arg5,
                                    syscall_return_value);
        errno = oerrno;
        return was_hooked;
    }

    reentrance_guard_flag = true;
    int oerrno = errno;
    was_hooked = hook(syscall_number,
                      arg0, arg1, arg2, arg3, arg4, arg5,
                      syscall_return_value);
    errno = oerrno;
    reentrance_guard_flag = false;

    return was_hooked;
}

void start_self_interception() {

    LOG(DEBUG, "Enabling syscall interception for self");

    intercept_hook_point = internal_hook_guard_wrapper;
    intercept_hook_point_post_kernel = hook_forwarded_syscall;
    intercept_hook_point_clone_child = hook_clone_at_child;
    intercept_hook_point_clone_parent = hook_clone_at_parent;
}

void start_interception() {

    assert(CTX->interception_enabled());

    LOG(DEBUG, "Enabling syscall interception for client process");

    // Set up the callback function pointer
    intercept_hook_point = hook_guard_wrapper;
    intercept_hook_point_post_kernel = hook_forwarded_syscall;
    intercept_hook_point_clone_child = hook_clone_at_child;
    intercept_hook_point_clone_parent = hook_clone_at_parent;
}

void stop_interception() {
    assert(CTX->interception_enabled());

    LOG(DEBUG, "Disabling syscall interception for client process");

    // Reset callback function pointer
    intercept_hook_point = nullptr;
    intercept_hook_point_post_kernel = nullptr;
    intercept_hook_point_clone_child = nullptr;
    intercept_hook_point_clone_parent = nullptr;
}
