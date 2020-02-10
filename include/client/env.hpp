/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_CLIENT_ENV
#define GKFS_CLIENT_ENV

#include <global/configure.hpp>

#define ADD_PREFIX(str) CLIENT_ENV_PREFIX str

/* Environment variables for the GekkoFS client */
namespace gkfs {
namespace env {

static constexpr auto LOG                 = ADD_PREFIX("LOG");

#ifdef GKFS_DEBUG_BUILD
static constexpr auto LOG_DEBUG_VERBOSITY = ADD_PREFIX("LOG_DEBUG_VERBOSITY");
static constexpr auto LOG_SYSCALL_FILTER  = ADD_PREFIX("LOG_SYSCALL_FILTER");
#endif

static constexpr auto LOG_OUTPUT          = ADD_PREFIX("LOG_OUTPUT");
static constexpr auto LOG_OUTPUT_TRUNC    = ADD_PREFIX("LOG_OUTPUT_TRUNC");
static constexpr auto CWD                 = ADD_PREFIX("CWD");
static constexpr auto HOSTS_FILE          = ADD_PREFIX("HOSTS_FILE");

} // namespace env
} // namespace gkfs

#undef ADD_PREFIX

#endif // GKFS_CLIENT_ENV

