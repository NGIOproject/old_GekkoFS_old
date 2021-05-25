/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef GKFS_DAEMON_RPC_DEFS_HPP
#define GKFS_DAEMON_RPC_DEFS_HPP

extern "C" {
#include <margo.h>
}

/* visible API for RPC operations */

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_fs_config)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_create)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_stat)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_decr_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_remove)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_metadentry_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_dirents)

#ifdef HAS_SYMLINKS

DECLARE_MARGO_RPC_HANDLER(rpc_srv_mk_symlink)

#endif


// data
DECLARE_MARGO_RPC_HANDLER(rpc_srv_read)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_write)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_truncate)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_chunk_stat)

#endif //GKFS_DAEMON_RPC_DEFS_HPP
