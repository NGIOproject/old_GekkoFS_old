/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/env_util.hpp>
#include <global/path_util.hpp>
#include <global/global_defs.hpp>
#include <global/configure.hpp>
#include <client/preload.hpp>
#include <client/resolve.hpp>
#include <global/rpc/distributor.hpp>
#include "global/rpc/rpc_types.hpp"
#include <client/logging.hpp>
#include <client/rpc/ld_rpc_management.hpp>
#include <client/preload_util.hpp>
#include <client/intercept.hpp>
#include <client/rpc/hg_rpcs.hpp>
#include <hermes.hpp>

#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>

#include <fstream>


using namespace std;

// make sure that things are only initialized once
static pthread_once_t init_env_thread = PTHREAD_ONCE_INIT;

// RPC IDs
hg_id_t rpc_config_id;
hg_id_t rpc_mk_node_id;
hg_id_t rpc_stat_id;
hg_id_t rpc_rm_node_id;
hg_id_t rpc_decr_size_id;
hg_id_t rpc_update_metadentry_id;
hg_id_t rpc_get_metadentry_size_id;
hg_id_t rpc_update_metadentry_size_id;
hg_id_t rpc_mk_symlink_id;
hg_id_t rpc_write_data_id;
hg_id_t rpc_read_data_id;
hg_id_t rpc_trunc_data_id;
hg_id_t rpc_get_dirents_id;
hg_id_t rpc_chunk_stat_id;

std::unique_ptr<hermes::async_engine> ld_network_service;

static inline void exit_error_msg(int errcode, const string& msg) {

    LOG_ERROR("{}", msg);
    gkfs::log::logger::log_message(stderr, "{}\n", msg);

    // if we don't disable interception before calling ::exit()
    // syscall hooks may find an inconsistent in shared state
    // (e.g. the logger) and thus, crash
    stop_interception();
    CTX->disable_interception();
    ::exit(errcode);
}

/**
 * Initializes the Hermes client for a given transport prefix
 * @param transport_prefix
 * @return true if succesfully initialized; false otherwise
 */
bool init_hermes_client(const std::string& transport_prefix) {

    try {

        hermes::engine_options opts{};

#if USE_SHM
        opts |= hermes::use_auto_sm;
#endif

        ld_network_service = 
            std::make_unique<hermes::async_engine>(
                    hermes::get_transport_type(transport_prefix), opts);
        ld_network_service->run();
    } catch (const std::exception& ex) {
        fmt::print(stderr, "Failed to initialize Hermes RPC client {}\n", 
                   ex.what());
        return false;
    }

    rpc_config_id = gkfs::rpc::fs_config::public_id;
    rpc_mk_node_id = gkfs::rpc::create::public_id;
    rpc_stat_id = gkfs::rpc::stat::public_id;
    rpc_rm_node_id = gkfs::rpc::remove::public_id;
    rpc_decr_size_id = gkfs::rpc::decr_size::public_id;
    rpc_update_metadentry_id = gkfs::rpc::update_metadentry::public_id;
    rpc_get_metadentry_size_id = gkfs::rpc::get_metadentry_size::public_id;
    rpc_update_metadentry_size_id = gkfs::rpc::update_metadentry::public_id;

#ifdef HAS_SYMLINKS
    rpc_mk_symlink_id = gkfs::rpc::mk_symlink::public_id;
#endif // HAS_SYMLINKS

    rpc_write_data_id = gkfs::rpc::write_data::public_id;
    rpc_read_data_id = gkfs::rpc::read_data::public_id;
    rpc_trunc_data_id = gkfs::rpc::trunc_data::public_id;
    rpc_get_dirents_id = gkfs::rpc::get_dirents::public_id;
    rpc_chunk_stat_id = gkfs::rpc::chunk_stat::public_id;

    return true;
}

/**
 * This function is only called in the preload constructor and initializes 
 * the file system client
 */
void init_ld_environment_() {

    // initialize Hermes interface to Mercury
    LOG(INFO, "Initializing RPC subsystem...");

    if (!init_hermes_client(RPC_PROTOCOL)) {
        exit_error_msg(EXIT_FAILURE, "Unable to initialize RPC subsystem");
    }

    try {
        LOG(INFO, "Loading peer addresses...");
        load_hosts();
    } catch (const std::exception& e) {
        exit_error_msg(EXIT_FAILURE, "Failed to load hosts addresses: "s + e.what());
    }

    /* Setup distributor */
    auto simple_hash_dist = std::make_shared<SimpleHashDistributor>(CTX->local_host_id(), CTX->hosts().size());
    CTX->distributor(simple_hash_dist);

    LOG(INFO, "Retrieving file system configuration...");

    if (!rpc_send::get_fs_config()) {
        exit_error_msg(EXIT_FAILURE, "Unable to fetch file system configurations from daemon process through RPC.");
    }

    LOG(INFO, "Environment initialization successful.");
}

void init_ld_env_if_needed() {
    pthread_once(&init_env_thread, init_ld_environment_);
}

void log_prog_name() {
    std::string line;
    std::ifstream cmdline("/proc/self/cmdline");
    if (!cmdline.is_open()) {
        LOG(ERROR, "Unable to open cmdline file");
        throw std::runtime_error("Unable to open cmdline file");
    }
    if(!getline(cmdline, line)) {
        throw std::runtime_error("Unable to read cmdline file");
    }
    std::replace(line.begin(), line.end(), '\0', ' ');
    line.erase(line.length() - 1, line.length());
    LOG(INFO, "Process cmdline: '{}'", line);
    cmdline.close();
}

/**
 * Called initially ONCE when preload library is used with the LD_PRELOAD environment variable
 */
void init_preload() {

    CTX->enable_interception();
    start_self_interception();

    CTX->init_logging();
    // from here ownwards it is safe to print messages
    LOG(DEBUG, "Logging subsystem initialized");

    // Kernel modules such as ib_uverbs may create fds in kernel space and pass
    // them to user-space processes using ioctl()-like interfaces. if this 
    // happens during our internal initialization, there's no way for us to 
    // control this creation and the fd will be created in the 
    // [0, MAX_USER_FDS) range rather than in our private 
    // [MAX_USER_FDS, MAX_OPEN_FDS) range. To prevent this for our internal 
    // initialization code, we forcefully occupy the user fd range to force 
    // such modules to create fds in our private range.
    CTX->protect_user_fds();

    log_prog_name();
    init_cwd();

    LOG(DEBUG, "Current working directory: '{}'", CTX->cwd());
    init_ld_env_if_needed();
    CTX->enable_interception();

    CTX->unprotect_user_fds();

    start_interception();
}

/**
 * Called last when preload library is used with the LD_PRELOAD environment variable
 */
void destroy_preload() {

    CTX->clear_hosts();
    LOG(DEBUG, "Peer information deleted");

    ld_network_service.reset();
    LOG(DEBUG, "RPC subsystem shut down");

    stop_interception();
    CTX->disable_interception();
    LOG(DEBUG, "Syscall interception stopped");

    LOG(INFO, "All subsystems shut down. Client shutdown complete.");
}
