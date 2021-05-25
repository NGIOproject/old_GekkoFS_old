/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_INTERCEPT_HPP
#define GEKKOFS_INTERCEPT_HPP

namespace gkfs {
namespace preload {

int
internal_hook_guard_wrapper(long syscall_number,
                            long arg0, long arg1, long arg2,
                            long arg3, long arg4, long arg5,
                            long* syscall_return_value);

int
hook_guard_wrapper(long syscall_number,
                   long arg0, long arg1, long arg2,
                   long arg3, long arg4, long arg5,
                   long* syscall_return_value);

void start_self_interception();

void start_interception();

void stop_interception();

} // namespace preload
} // namespace gkfs

#endif
