/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#ifndef IFS_PRELOAD_UTIL_HPP
#define IFS_PRELOAD_UTIL_HPP

#include <client/preload.hpp>
#include <global/metadata.hpp>
// third party libs
#include <string>
#include <iostream>
#include <map>

struct MetadentryUpdateFlags {
    bool atime = false;
    bool mtime = false;
    bool ctime = false;
    bool uid = false;
    bool gid = false;
    bool mode = false;
    bool link_count = false;
    bool size = false;
    bool blocks = false;
    bool path = false;
};

// Hermes instance
namespace hermes { class async_engine; }
extern std::unique_ptr<hermes::async_engine> ld_network_service;

// RPC IDs
extern hg_id_t rpc_config_id;
extern hg_id_t rpc_mk_node_id;
extern hg_id_t rpc_stat_id;
extern hg_id_t rpc_rm_node_id;
extern hg_id_t rpc_decr_size_id;
extern hg_id_t rpc_update_metadentry_id;
extern hg_id_t rpc_get_metadentry_size_id;
extern hg_id_t rpc_update_metadentry_size_id;
extern hg_id_t rpc_write_data_id;
extern hg_id_t rpc_read_data_id;
extern hg_id_t rpc_trunc_data_id;
extern hg_id_t rpc_get_dirents_id;
extern hg_id_t rpc_chunk_stat_id;

#ifdef HAS_SYMLINKS
extern hg_id_t ipc_mk_symlink_id;
extern hg_id_t rpc_mk_symlink_id;
#endif

// function definitions

int metadata_to_stat(const std::string& path, const Metadata& md, struct stat& attr);

std::vector<std::pair<std::string, std::string>> load_hosts_file(const std::string& lfpath);

hg_addr_t get_local_addr();

void load_hosts();
bool lookup_all_hosts();

#endif //IFS_PRELOAD_UTIL_HPP
