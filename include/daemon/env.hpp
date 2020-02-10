/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_DAEMON_ENV
#define GKFS_DAEMON_ENV

#include <global/configure.hpp>

#define ADD_PREFIX(str) DAEMON_ENV_PREFIX str

/* Environment variables for the GekkoFS daemon */
namespace gkfs {
namespace env {

static constexpr auto HOSTS_FILE = ADD_PREFIX("HOSTS_FILE");

} // namespace env
} // namespace gkfs

#undef ADD_PREFIX

#endif // GKFS_DAEMON_ENV


