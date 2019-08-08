/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_DAEMON_MAIN_HPP
#define GKFS_DAEMON_MAIN_HPP

// std libs
#include <string>
#include <spdlog/spdlog.h>

#include <global/configure.hpp>
#include <global/global_defs.hpp>
// margo
extern "C" {
#include <abt.h>
#include <mercury.h>
#include <margo.h>
}
#include <daemon/classes/fs_data.hpp>
#include <daemon/classes/rpc_data.hpp>

#define ADAFS_DATA (static_cast<FsData*>(FsData::getInstance()))
#define RPC_DATA (static_cast<RPCData*>(RPCData::getInstance()))

void init_environment();
void destroy_enviroment();

void init_io_tasklet_pool();
void init_rpc_server(const std::string& protocol);

void register_server_rpcs(margo_instance_id mid);

void populate_hosts_file();
void destroy_hosts_file();

#endif // GKFS_DAEMON_MAIN_HPP
