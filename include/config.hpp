/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_CONFIG_HPP
#define GEKKOFS_CONFIG_HPP

#include <global/cmake_configure.hpp>

// environment prefixes (are concatenated in env module at compile time)
#define CLIENT_ENV_PREFIX "LIBGKFS_"
#define DAEMON_ENV_PREFIX "GKFS_"

namespace gkfs {
namespace config {

constexpr auto hostfile_path = "./gkfs_hosts.txt";
constexpr auto forwarding_file_path = "./gkfs_forwarding.map";

namespace io {
/*
 * Zero buffer before read. This is relevant if sparse files are used.
 * If buffer is not zeroed, sparse regions contain invalid data.
 */
constexpr auto zero_buffer_before_read = false;
} // namespace io

namespace log {
constexpr auto client_log_path = "/tmp/gkfs_client.log";
constexpr auto daemon_log_path = "/tmp/gkfs_daemon.log";

constexpr auto client_log_level = "info,errors,critical,hermes";
constexpr auto daemon_log_level = 4; //info
} // namespace logging

namespace metadata {
// which metadata should be considered apart from size and mode
constexpr auto use_atime = false;
constexpr auto use_ctime = false;
constexpr auto use_mtime = false;
constexpr auto use_link_cnt = false;
constexpr auto use_blocks = false;
} // namespace metadata

namespace rpc {
constexpr auto chunksize = 524288; // in bytes (e.g., 524288 == 512KB)
//size of preallocated buffer to hold directory entries in rpc call
constexpr auto dirents_buff_size = (8 * 1024 * 1024); // 8 mega
/*
 * Indicates the number of concurrent progress to drive I/O operations of chunk files to and from local file systems
 * The value is directly mapped to created Argobots xstreams, controlled in a single pool with ABT_snoozer scheduler
 */
constexpr auto daemon_io_xstreams = 8;
// Number of threads used for RPC handlers at the daemon
constexpr auto daemon_handler_xstreams = 8;
} // namespace rpc

namespace rocksdb {
// Write-ahead logging of rocksdb
constexpr auto use_write_ahead_log = false;
} // namespace rocksdb

} // namespace gkfs
} // namespace config

#endif //GEKKOFS_CONFIG_HPP
