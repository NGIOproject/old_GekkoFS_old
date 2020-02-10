/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_SYSCALL_HPP
#define GKFS_SYSCALL_HPP

#include <client/syscalls/args.hpp>
#include <client/syscalls/rets.hpp>
#include <client/syscalls/errno.hpp>
#include <client/syscalls/detail/syscall_info.h>

namespace gkfs {
namespace syscall {

static const auto constexpr MAX_ARGS = 6u;
using arg_list = std::array<arg::desc, MAX_ARGS>;

struct descriptor : private ::syscall_info {

    long
    number() const {
        return s_nr;
    }

    const char*
    name() const {
        return s_name;
    }

    int
    num_args() const {
        return s_nargs;
    }

    arg_list
    args() const {

        std::array<arg::desc, MAX_ARGS> args;

        for(auto i = 0u; i < MAX_ARGS; ++i) {
            args[i] = {static_cast<arg::type>(s_args[i].a_type),
                       s_args[i].a_name};
        }

        return args;
    }

    ret::desc
    return_type() const {
        return ret::desc{static_cast<ret::type>(s_return_type.r_type)};
    }
};

static inline descriptor
lookup_by_number(const long syscall_number) {
    const auto* info = ::get_syscall_info(syscall_number, nullptr);
    return *reinterpret_cast<const descriptor*>(info);
}

static inline descriptor
lookup_by_number(const long syscall_number,
               const long argv[MAX_ARGS]) {
    const auto* info = ::get_syscall_info(syscall_number, argv);
    return *reinterpret_cast<const descriptor*>(info);
}

static inline descriptor
lookup_by_name(const std::string syscall_name) {
    const auto* info = ::get_syscall_info_by_name(syscall_name.c_str());
    return *reinterpret_cast<const descriptor*>(info);
}

static inline bool
never_returns(const long syscall_number) {
    const auto desc = lookup_by_number(syscall_number);
    return desc.return_type() == ret::none;
}

static inline bool
always_returns(const long syscall_number) {
    return !never_returns(syscall_number);
}

static inline bool
may_not_return(const long syscall_number) {
    return syscall_number == SYS_execve 
#ifdef SYS_execveat
        || syscall_number == SYS_execveat
#endif
        ;
}


// information about a syscall
enum class info : int {
    unknown        = 0x00000000, // no info (reset)
    
    // syscall origin
    internal       = 0x00000001, // syscall originates from GekkoFS' internals
    external       = 0x00000002, // syscall originates from client application

    // syscall target
    kernel         = 0x00000010, // syscall forwarded to the kernel
    hook           = 0x00000020, // syscall handled by GekkoFS

    // syscall state
    executed       = 0x00000100, // syscall has been executed
    not_executed   = 0x00000000, // syscall has not been executed

    // masks
    origin_mask    = 0x00000003, // mask for syscall's origin information
    target_mask    = 0x7ffffefc, // mask for syscall's target information
    execution_mask = 0x00000100 // mask for syscall's execution state
};


inline constexpr info
operator&(info t1, info t2) { 
    return info(static_cast<int>(t1) & static_cast<int>(t2)); 
}

inline constexpr info
operator|(info t1, info t2) { 
    return info(static_cast<int>(t1) | static_cast<int>(t2)); 
}

inline constexpr info
operator^(info t1, info t2) { 
    return info(static_cast<int>(t1) ^ static_cast<int>(t2)); 
}

inline constexpr info
operator~(info t1) { 
    return info(~static_cast<int>(t1)); 
}

inline const info&
operator|=(info& t1, info t2) { 
    return t1 = t1 | t2; 
}

inline const info&
operator&=(info& t1, info t2) { 
    return t1 = t1 & t2; 
}

inline const info&
operator^=(info& t1, info t2) { 
    return t1 = t1 ^ t2; 
}


static const auto constexpr no_info            = info::unknown;
static const auto constexpr from_internal_code = info::internal;
static const auto constexpr from_external_code = info::external;
static const auto constexpr to_kernel          = info::kernel;
static const auto constexpr to_hook            = info::hook;

static const auto constexpr executed           = info::executed;
static const auto constexpr not_executed       = info::not_executed;

static const auto constexpr origin_mask        = info::origin_mask;
static const auto constexpr target_mask        = info::target_mask;
static const auto constexpr execution_mask     = info::execution_mask;

enum {
    hooked            = 0x0,
    forward_to_kernel = 0x1
};

static constexpr auto
origin(syscall::info info) {
    return info & origin_mask;
}

static constexpr auto
target(syscall::info info) {
    return info & target_mask;
}

static constexpr bool 
is_handled_by_kernel(syscall::info info) {
    return (info & target_mask) == to_kernel;
}

static constexpr auto
execution_is_pending(syscall::info info) {
    return (info & execution_mask) == not_executed;
}

/*
 * error_code - examines a return value from a syscall execution
 * and returns an error code if said return value indicates an error.
 */
static inline int
error_code(long result) {
	if (result < 0 && result >= -0x1000)
		return (int)-result;

	return 0;
}

} // namespace syscall
} // namespace gkfs

#endif // GKFS_SYSCALL_HPP
