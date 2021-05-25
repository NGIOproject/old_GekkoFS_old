/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef LFS_RPC_TYPES_HPP
#define LFS_RPC_TYPES_HPP

extern "C" {
#include <mercury_proc_string.h>
#include <margo.h>
}

/* visible API for RPC data types used in RPCS */

// misc generic rpc types
MERCURY_GEN_PROC(rpc_err_out_t,
                 ((hg_int32_t) (err)))

// Metadentry
MERCURY_GEN_PROC(rpc_mk_node_in_t,
                 ((hg_const_string_t) (path))
                         ((uint32_t) (mode)))

MERCURY_GEN_PROC(rpc_path_only_in_t,
                 ((hg_const_string_t) (path)))

MERCURY_GEN_PROC(rpc_stat_out_t,
                 ((hg_int32_t) (err))
                         ((hg_const_string_t) (db_val)))

MERCURY_GEN_PROC(rpc_rm_node_in_t,
                 ((hg_const_string_t) (path)))

MERCURY_GEN_PROC(rpc_trunc_in_t,
                 ((hg_const_string_t) (path))
                         ((hg_uint64_t) (length)))

MERCURY_GEN_PROC(rpc_update_metadentry_in_t,
                 ((hg_const_string_t) (path))
                         ((uint64_t) (nlink))
                         ((hg_uint32_t) (mode))
                         ((hg_uint32_t) (uid))
                         ((hg_uint32_t) (gid))
                         ((hg_int64_t) (size))
                         ((hg_int64_t) (blocks))
                         ((hg_int64_t) (atime))
                         ((hg_int64_t) (mtime))
                         ((hg_int64_t) (ctime))
                         ((hg_bool_t) (nlink_flag))
                         ((hg_bool_t) (mode_flag))
                         ((hg_bool_t) (size_flag))
                         ((hg_bool_t) (block_flag))
                         ((hg_bool_t) (atime_flag))
                         ((hg_bool_t) (mtime_flag))
                         ((hg_bool_t) (ctime_flag)))

MERCURY_GEN_PROC(rpc_update_metadentry_size_in_t,
                 ((hg_const_string_t) (path))
                         ((hg_uint64_t) (size))
                         ((hg_int64_t) (offset))
                         ((hg_bool_t) (append)))

MERCURY_GEN_PROC(rpc_update_metadentry_size_out_t,
                 ((hg_int32_t) (err))
                         ((hg_int64_t) (ret_size)))

MERCURY_GEN_PROC(rpc_get_metadentry_size_out_t,
                 ((hg_int32_t) (err))
                         ((hg_int64_t) (ret_size)))

#ifdef HAS_SYMLINKS
MERCURY_GEN_PROC(rpc_mk_symlink_in_t,
                 ((hg_const_string_t) (path))
                         ((hg_const_string_t) (target_path))
)

#endif

// data
MERCURY_GEN_PROC(rpc_read_data_in_t,
                 ((hg_const_string_t) (path))
                         ((int64_t) (offset))
                         ((hg_uint64_t) (host_id))
                         ((hg_uint64_t) (host_size))
                         ((hg_uint64_t) (chunk_n))
                         ((hg_uint64_t) (chunk_start))
                         ((hg_uint64_t) (chunk_end))
                         ((hg_uint64_t) (total_chunk_size))
                         ((hg_bulk_t) (bulk_handle)))

MERCURY_GEN_PROC(rpc_data_out_t,
                 ((int32_t) (err))
                         ((hg_size_t) (io_size)))

MERCURY_GEN_PROC(rpc_write_data_in_t,
                 ((hg_const_string_t) (path))
                         ((int64_t) (offset))
                         ((hg_uint64_t) (host_id))
                         ((hg_uint64_t) (host_size))
                         ((hg_uint64_t) (chunk_n))
                         ((hg_uint64_t) (chunk_start))
                         ((hg_uint64_t) (chunk_end))
                         ((hg_uint64_t) (total_chunk_size))
                         ((hg_bulk_t) (bulk_handle)))

MERCURY_GEN_PROC(rpc_get_dirents_in_t,
                 ((hg_const_string_t) (path))
                         ((hg_bulk_t) (bulk_handle))
)

MERCURY_GEN_PROC(rpc_get_dirents_out_t,
                 ((hg_int32_t) (err))
                         ((hg_size_t) (dirents_size))
)


MERCURY_GEN_PROC(rpc_config_out_t,
                 ((hg_const_string_t) (mountdir))
                         ((hg_const_string_t) (rootdir))
                         ((hg_bool_t) (atime_state))
                         ((hg_bool_t) (mtime_state))
                         ((hg_bool_t) (ctime_state))
                         ((hg_bool_t) (link_cnt_state))
                         ((hg_bool_t) (blocks_state))
                         ((hg_uint32_t) (uid))
                         ((hg_uint32_t) (gid))
)


MERCURY_GEN_PROC(rpc_chunk_stat_in_t,
                 ((hg_int32_t) (dummy))
)

MERCURY_GEN_PROC(rpc_chunk_stat_out_t,
                 ((hg_int32_t) (err))
                         ((hg_uint64_t) (chunk_size))
                         ((hg_uint64_t) (chunk_total))
                         ((hg_uint64_t) (chunk_free))
)

#endif //LFS_RPC_TYPES_HPP
