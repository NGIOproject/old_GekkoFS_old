/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/
#include <daemon/util.hpp>
#include <daemon/daemon.hpp>

#include <global/rpc/rpc_util.hpp>

#include <fstream>
#include <iostream>

using namespace std;

namespace gkfs {
namespace util {

void populate_hosts_file() {
    const auto& hosts_file = GKFS_DATA->hosts_file();
    GKFS_DATA->spdlogger()->debug("{}() Populating hosts file: '{}'", __func__, hosts_file);
    ofstream lfstream(hosts_file, ios::out | ios::app);
    if (!lfstream) {
        throw runtime_error(
                fmt::format("Failed to open hosts file '{}': {}", hosts_file, strerror(errno)));
    }
    lfstream << fmt::format("{} {}", gkfs::rpc::get_my_hostname(true), RPC_DATA->self_addr_str()) << std::endl;
    if (!lfstream) {
        throw runtime_error(
                fmt::format("Failed to write on hosts file '{}': {}", hosts_file, strerror(errno)));
    }
    lfstream.close();
}

void destroy_hosts_file() {
    std::remove(GKFS_DATA->hosts_file().c_str());
}

} // namespace util
} // namespace gkfs
