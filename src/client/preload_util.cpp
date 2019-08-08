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
#include <global/rpc/distributor.hpp>
#include <global/rpc/rpc_utils.hpp>
#include <global/env_util.hpp>

#include <fstream>
#include <sstream>
#include <regex>
#include <csignal>
#include <random>
#include <sys/sysmacros.h>

#include <sys/types.h>
#include <pwd.h>

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
    CTX->log()->debug("{}() Loading hosts file: '{}'", __func__, lfpath);
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
            spdlog::error("{}() Unrecognized line format: [path: '{}', line: '{}']",
                          __func__, lfpath, line);
            throw runtime_error(
                    fmt::format("unrecognized line format: '{}'", line));
        }
        host = match[1];
        uri = match[2];
        hosts.emplace_back(host, uri);
    }
    return hosts;
}

hg_addr_t margo_addr_lookup_retry(const std::string& uri) {
    CTX->log()->debug("{}() Lookink up address '{}'", __func__, uri);
    // try to look up 3 times before erroring out
    hg_return_t ret;
    hg_addr_t remote_addr = HG_ADDR_NULL;
    ::random_device rd; // obtain a random number from hardware
    unsigned int attempts = 0;
    do {
        ret = margo_addr_lookup(ld_margo_rpc_id, uri.c_str(), &remote_addr);
        if (ret == HG_SUCCESS) {
            return remote_addr;
        }
        CTX->log()->warn("{}() Failed to lookup address '{}'. Attempts [{}/3]", __func__, uri, attempts + 1);
        // Wait a random amount of time and try again
        ::mt19937 g(rd()); // seed the random generator
        ::uniform_int_distribution<> distr(50, 50 * (attempts + 2)); // define the range
        ::this_thread::sleep_for(std::chrono::milliseconds(distr(g)));
    } while (++attempts < 3);
    throw runtime_error(
            fmt::format("Failed to lookup address '{}', error: {}", uri, HG_Error_to_string(ret)));
}

void load_hosts() {
    string hosts_file;
    char* homedir = NULL;
	struct passwd *pw = getpwuid(getuid());
    if (pw)
 	  homedir = pw->pw_dir;

    try {
        hosts_file = gkfs::get_env_own("HOSTS_FILE");
    } catch (const exception& e) {
	hosts_file = string(homedir)+"/gkfs_hosts.txt"s;
	CTX->log()->info("{}() Failed to get hosts file path"
                         " from environment, using default: '{}'",
                         __func__, hosts_file);
    }

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

    CTX->log()->info("{}() Hosts pool size: {}", __func__, hosts.size());

    auto local_hostname = get_my_hostname(true);
    bool local_host_found = false;
    vector<hg_addr_t> addrs(hosts.size());

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
        auto addr = margo_addr_lookup_retry(uri);
        addrs.at(id) = addr;

        if (!local_host_found && hostname == local_hostname) {
            CTX->log()->debug("{}() Found local host: {}", __func__, hostname);
            CTX->local_host_id(id);
            local_host_found = true;
        }
    }

    if (!local_host_found) {
        CTX->log()->warn("{}() Failed to find local host."
                            "Fallback: use host id '0' as local host", __func__);
        CTX->local_host_id(0);
    }
    CTX->hosts(addrs);
}

void cleanup_addresses() {
    for (auto& addr: CTX->hosts()) {
        margo_addr_free(ld_margo_rpc_id, addr);
    }
}



hg_return
margo_create_wrap_helper(const hg_id_t rpc_id, uint64_t recipient, hg_handle_t& handle) {
    auto ret = margo_create(ld_margo_rpc_id, CTX->hosts().at(recipient), rpc_id, &handle);
    if (ret != HG_SUCCESS) {
        CTX->log()->error("{}() creating handle FAILED", __func__);
        return HG_OTHER_ERROR;
    }
    return ret;
}

/**
 * Wraps certain margo functions to create a Mercury handle
 * @param path
 * @param handle
 * @return
 */
hg_return margo_create_wrap(const hg_id_t rpc_id, const std::string& path, hg_handle_t& handle) {
    auto recipient = CTX->distributor()->locate_file_metadata(path);
    return margo_create_wrap_helper(rpc_id, recipient, handle);
}
