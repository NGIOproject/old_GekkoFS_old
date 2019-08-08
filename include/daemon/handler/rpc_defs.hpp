/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef LFS_RPC_DEFS_HPP
#define LFS_RPC_DEFS_HPP

extern "C" {
#include <margo.h>
}

/* visible API for RPC operations */

DECLARE_MARGO_RPC_HANDLER(rpc_srv_fs_config)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_mk_node)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_stat)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_decr_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_rm_node)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_metadentry_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_update_metadentry_size)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_get_dirents)

#ifdef HAS_SYMLINKS
DECLARE_MARGO_RPC_HANDLER(rpc_srv_mk_symlink)
#endif


// data
DECLARE_MARGO_RPC_HANDLER(rpc_srv_read_data)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_write_data)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_trunc_data)

DECLARE_MARGO_RPC_HANDLER(rpc_srv_chunk_stat)

#endif //LFS_RPC_DEFS_HPP
