/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_COMMON_ENV_UTIL_HPP
#define GKFS_COMMON_ENV_UTIL_HPP

#include <string>

namespace gkfs {
namespace env {

std::string
get_var(const std::string& name, 
        const std::string& default_value = "");

} // namespace env
} // namespace gkfs

#endif // GKFS_COMMON_ENV_UTIL_HPP
