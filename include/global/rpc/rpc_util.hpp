/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef GEKKOFS_GLOBAL_RPC_UTILS_HPP
#define GEKKOFS_GLOBAL_RPC_UTILS_HPP

extern "C" {
#include <mercury_types.h>
#include <mercury_proc_string.h>
}

#include <string>

namespace gkfs {
namespace rpc {

hg_bool_t bool_to_merc_bool(bool state);

std::string get_my_hostname(bool short_hostname = false);

std::string get_host_by_name(const std::string& hostname);

} // namespace rpc
} // namespace gkfs

#endif //GEKKOFS_GLOBAL_RPC_UTILS_HPP
