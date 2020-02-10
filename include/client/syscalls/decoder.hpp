/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_SYSCALLS_DECODER_HPP
#define GKFS_SYSCALLS_DECODER_HPP

#include <client/syscalls/syscall.hpp>
#include <client/syscalls/args.hpp>
#include <client/syscalls/rets.hpp>

namespace gkfs {
namespace syscall {

namespace detail {

/** a RAII saver/restorer of errno values */
struct errno_saver {
    errno_saver(int errnum) :
        saved_errno_(errnum) { }

    ~errno_saver() {
        errno = saved_errno_;
    }

    const int saved_errno_;
};

} // namespace detail

template <typename FmtBuffer>
inline void
decode(FmtBuffer& buffer, 
       const long syscall_number,
       const long argv[MAX_ARGS]) {

    detail::errno_saver _(errno);

    const auto sc = lookup_by_number(syscall_number, argv);

    fmt::format_to(buffer, "{}(", sc.name());

    for(int i = 0; i < sc.num_args(); ++i) {
        const auto& arg = sc.args().at(i);

        arg.formatter<FmtBuffer>()(buffer, {arg.name(), argv[i]});

        if(i < sc.num_args() - 1) {
            fmt::format_to(buffer, ", ");
        }
    }

    fmt::format_to(buffer, ") = ?");
}

template <typename FmtBuffer>
inline void
decode(FmtBuffer& buffer, 
       const long syscall_number,
       const long argv[MAX_ARGS],
       const long result) {

    detail::errno_saver _(errno);

    const auto sc = lookup_by_number(syscall_number, argv);

    fmt::format_to(buffer, "{}(", sc.name());

    for(int i = 0; i < sc.num_args(); ++i) {
        const auto& arg = sc.args().at(i);

        arg.formatter<FmtBuffer>()(buffer, {arg.name(), argv[i]});

        if(i < sc.num_args() - 1) {
            fmt::format_to(buffer, ", ");
        }
    }

    if(never_returns(syscall_number)) {
        fmt::format_to(buffer, ") = ?");
        return;
    }

	if(error_code(result) != 0) {
        fmt::format_to(buffer, ") = {} {} ({})", 
                static_cast<int>(-1), 
                errno_name(-result),
                errno_message(-result));
        return;
    }

    fmt::format_to(buffer, ") = ");
    const auto& ret = sc.return_type();
    ret.formatter<FmtBuffer>()(buffer, result);

}

} // namespace syscall
} // namespace gkfs

#endif // GKFS_SYSCALLS_DECODER_HPP
