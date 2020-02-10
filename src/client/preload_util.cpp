/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <client/preload_util.hpp>
#include <client/env.hpp>
#include <client/logging.hpp>
#include <global/rpc/distributor.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <global/env_util.hpp>
#include <hermes.hpp>

#include <fstream>
#include <sstream>
#include <regex>
#include <csignal>
#include <random>
#include <sys/sysmacros.h>

using namespace std;

/**
 * Converts the Metadata object into a stat struct, which is needed by Linux
 * @param path
 * @param md
 * @param attr
 * @return
 */
int metadata_to_stat(const std::string& path, const Metadata& md, struct stat& attr) {

    /* Populate default values */
    attr.st_dev = makedev(0, 0);
    attr.st_ino = std::hash<std::string>{}(path);
    attr.st_nlink = 1;
    attr.st_uid = CTX->fs_conf()->uid;
    attr.st_gid = CTX->fs_conf()->gid;
    attr.st_rdev = 0;
    attr.st_blksize = CHUNKSIZE;
    attr.st_blocks = 0;

    memset(&attr.st_atim, 0, sizeof(timespec));
    memset(&attr.st_mtim, 0, sizeof(timespec));
    memset(&attr.st_ctim, 0, sizeof(timespec));

    attr.st_mode = md.mode();

#ifdef HAS_SYMLINKS
    if (md.is_link())
        attr.st_size = md.target_path().size() + CTX->mountdir().size();
    else
#endif
    attr.st_size = md.size();

    if (CTX->fs_conf()->atime_state) {
        attr.st_atim.tv_sec = md.atime();
    }
    if (CTX->fs_conf()->mtime_state) {
        attr.st_mtim.tv_sec = md.mtime();
    }
    if (CTX->fs_conf()->ctime_state) {
        attr.st_ctim.tv_sec = md.ctime();
    }
    if (CTX->fs_conf()->link_cnt_state) {
        attr.st_nlink = md.link_count();
    }
    if (CTX->fs_conf()->blocks_state) { // last one will not encounter a delimiter anymore
        attr.st_blocks = md.blocks();
    }
    return 0;
}

vector<pair<string, string>> load_hosts_file(const std::string& lfpath) {

    LOG(DEBUG, "Loading hosts file: \"{}\"", lfpath);

    ifstream lf(lfpath);
    if (!lf) {
        throw runtime_error(fmt::format("Failed to open hosts file '{}': {}",
                            lfpath, strerror(errno)));
    }
    vector<pair<string, string>> hosts;
    const regex line_re("^(\\S+)\\s+(\\S+)$",
                        regex::ECMAScript | regex::optimize);
    string line;
    string host;
    string uri;
    std::smatch match;
    while (getline(lf, line)) {
        if (!regex_match(line, match, line_re)) {

            LOG(ERROR, "Unrecognized line format: [path: '{}', line: '{}']",
                lfpath, line);

            throw runtime_error(
                    fmt::format("unrecognized line format: '{}'", line));
        }
        host = match[1];
        uri = match[2];
        hosts.emplace_back(host, uri);
    }
    return hosts;
}

hermes::endpoint lookup_endpoint(const std::string& uri, 
                                 std::size_t max_retries = 3) {

    LOG(DEBUG, "Looking up address \"{}\"", uri);

    std::random_device rd; // obtain a random number from hardware
    std::size_t attempts = 0;
    std::string error_msg;

    do {
        try {
            return ld_network_service->lookup(uri);
        } catch (const exception& ex) {
            error_msg = ex.what();

            LOG(WARNING, "Failed to lookup address '{}'. Attempts [{}/{}]", 
                uri, attempts + 1, max_retries);

            // Wait a random amount of time and try again
            std::mt19937 g(rd()); // seed the random generator
            std::uniform_int_distribution<> distr(50, 50 * (attempts + 2)); // define the range
            std::this_thread::sleep_for(std::chrono::milliseconds(distr(g)));
            continue;
        }
    } while (++attempts < max_retries);

    throw std::runtime_error(
            fmt::format("Endpoint for address '{}' could not be found ({})", 
                        uri, error_msg));
}

void load_hosts() {
    string hosts_file;

    hosts_file = gkfs::env::get_var(gkfs::env::HOSTS_FILE, DEFAULT_HOSTS_FILE);

    vector<pair<string, string>> hosts;
    try {
        hosts = load_hosts_file(hosts_file);
    } catch (const exception& e) {
        auto emsg = fmt::format("Failed to load hosts file: {}", e.what());
        throw runtime_error(emsg);
    }

    if (hosts.size() == 0) {
        throw runtime_error(fmt::format("Host file empty: '{}'", hosts_file));
    }

    LOG(INFO, "Hosts pool size: {}", hosts.size());

    auto local_hostname = get_my_hostname(true);
    bool local_host_found = false;

    std::vector<hermes::endpoint> addrs;
    addrs.resize(hosts.size());

    vector<uint64_t> host_ids(hosts.size());
    // populate vector with [0, ..., host_size - 1]
    ::iota(::begin(host_ids), ::end(host_ids), 0);
    /*
     * Shuffle hosts to balance addr lookups to all hosts
     * Too many concurrent lookups send to same host
     * could overwhelm the server,
     * returning error when addr lookup
     */
    ::random_device rd; // obtain a random number from hardware
    ::mt19937 g(rd()); // seed the random generator
    ::shuffle(host_ids.begin(), host_ids.end(), g); // Shuffle hosts vector
    // lookup addresses and put abstract server addresses into rpc_addressesre

    for (const auto& id: host_ids) {
         const auto& hostname = hosts.at(id).first;
         const auto& uri = hosts.at(id).second;

        addrs[id] = ::lookup_endpoint(uri);

        if (!local_host_found && hostname == local_hostname) {
            LOG(DEBUG, "Found local host: {}", hostname);
            CTX->local_host_id(id);
            local_host_found = true;
        }

        LOG(DEBUG, "Found peer: {}", addrs[id].to_string()); 
    }

    if (!local_host_found) {
        LOG(WARNING, "Failed to find local host. Using host '0' as local host");
        CTX->local_host_id(0);
    }

    CTX->hosts(addrs);
}
