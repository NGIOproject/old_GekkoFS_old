/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GKFS_DAEMON_DAEMON_HPP
#define GKFS_DAEMON_DAEMON_HPP

// std libs
#include <string>
#include <spdlog/spdlog.h>

#include <config.hpp>
#include <global/global_defs.hpp>
// margo
extern "C" {
#include <abt.h>
#include <mercury.h>
#include <margo.h>
}

#include <daemon/classes/fs_data.hpp>
#include <daemon/classes/rpc_data.hpp>

#define GKFS_DATA (static_cast<gkfs::daemon::FsData*>(gkfs::daemon::FsData::getInstance()))
#define RPC_DATA (static_cast<gkfs::daemon::RPCData*>(gkfs::daemon::RPCData::getInstance()))

#endif // GKFS_DAEMON_DAEMON_HPP
