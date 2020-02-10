/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef SYSCALLS_DETAIL_SYSCALL_INFO_H
#define SYSCALLS_DETAIL_SYSCALL_INFO_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define MAX_SYSCALL_ARGS 6u

typedef enum {
    none = 0,       /* no argument */
    fd,             /* fd argument for non *at() syscalls */
    atfd,           /* fd argument for *at() syscalls */ 
    cstr,           /* a \0-terminated C string */
    open_flags,     /* flags for open/create/mq_open ... */
    octal_mode,     /* mode_t arguments */
    ptr,            /* pointer arguments */
    dec,            /* signed decimal number (aka. long) */
    dec32,          /* signed 32-bit decimal number (aka. int) */
    offset,         /* off_t arguments */
    whence,         /* 'whence' argument in lseek-style syscalls */
    mmap_prot,      /* protections for the mmap() family of syscalls */
    mmap_flags,     /* flags for the mmap() family of syscalls */
    clone_flags,    /* flags for the clone() syscall */
    signum,         /* signal numbers */
    sigproc_how,    /* sigprocmask argument */
    arg,            /* generic argument, no special formatting */
    arg_type_max
} arg_type_t;

typedef struct {
    const arg_type_t a_type;
    const char * const a_name;
} syscall_arg_t;

typedef enum {
    rnone,
    rptr,
    rdec,
    ret_type_max,
} ret_type_t;

typedef struct {
    const ret_type_t r_type;
} syscall_ret_t;

struct syscall_info {
    const long s_nr;
    const char * const s_name;
    const int s_nargs;
    const syscall_ret_t s_return_type;
    const syscall_arg_t s_args[MAX_SYSCALL_ARGS];
};

extern const struct syscall_info syscall_table[];

extern const struct syscall_info* 
get_syscall_info(const long syscall_number, 
                 const long* argv);

extern const struct syscall_info* 
get_syscall_info_by_name(const char* syscall_name);

extern bool 
syscall_never_returns(long);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SYSCALLS_DETAIL_SYSCALL_INFO_H
