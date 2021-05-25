/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_DAEMON_UTIL_HPP
#define GEKKOFS_DAEMON_UTIL_HPP

namespace gkfs {
namespace util {
void populate_hosts_file();

void destroy_hosts_file();
}
}

#endif //GEKKOFS_DAEMON_UTIL_HPP
