/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <global/log_util.hpp>
#include <global/env_util.hpp>
#include <global/path_util.hpp>
#include <global/global_defs.hpp>
#include <global/configure.hpp>
#include <client/preload.hpp>
#include <client/resolve.hpp>
#include <global/rpc/distributor.hpp>
#include "global/rpc/rpc_types.hpp"
#include <client/rpc/ld_rpc_management.hpp>
#include <client/preload_util.hpp>
#include <client/intercept.hpp>

#include <fstream>

using namespace std;
//
// thread to initialize the whole margo shazaam only once per process
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
// Margo instances
margo_instance_id ld_margo_rpc_id;


static inline void exit_error_msg(int errcode, const string& msg) {
    CTX->log()->error(msg);
    cerr << "GekkoFS error: " << msg << endl;
    exit(errcode);
}

/**
 * Registers a margo instance with all used RPC
 * Note that the r(pc tags are redundant for rpc
 * @param mid
 * @param mode
 */
void register_client_rpcs(margo_instance_id mid) {

    rpc_config_id = MARGO_REGISTER(mid,
        hg_tag::fs_config,
        void,
        rpc_config_out_t,
        NULL);

    rpc_mk_node_id = MARGO_REGISTER(mid, hg_tag::create, rpc_mk_node_in_t, rpc_err_out_t, NULL);
    rpc_stat_id = MARGO_REGISTER(mid, hg_tag::stat, rpc_path_only_in_t, rpc_stat_out_t, NULL);
    rpc_rm_node_id = MARGO_REGISTER(mid, hg_tag::remove, rpc_rm_node_in_t,
                                    rpc_err_out_t, NULL);

    rpc_decr_size_id = MARGO_REGISTER(mid,
        hg_tag::decr_size,
        rpc_trunc_in_t,
        rpc_err_out_t,
        NULL);

    rpc_update_metadentry_id = MARGO_REGISTER(mid, hg_tag::update_metadentry, rpc_update_metadentry_in_t,
                                              rpc_err_out_t, NULL);
    rpc_get_metadentry_size_id = MARGO_REGISTER(mid, hg_tag::get_metadentry_size, rpc_path_only_in_t,
                                                rpc_get_metadentry_size_out_t, NULL);
    rpc_update_metadentry_size_id = MARGO_REGISTER(mid, hg_tag::update_metadentry_size,
                                                   rpc_update_metadentry_size_in_t,
                                                   rpc_update_metadentry_size_out_t,
                                                   NULL);

#ifdef HAS_SYMLINKS
    rpc_mk_symlink_id = MARGO_REGISTER(mid,
         hg_tag::mk_symlink,
         rpc_mk_symlink_in_t,
         rpc_err_out_t,
         NULL);
#endif

    rpc_write_data_id = MARGO_REGISTER(mid, hg_tag::write_data, rpc_write_data_in_t, rpc_data_out_t,
                                       NULL);
    rpc_read_data_id = MARGO_REGISTER(mid, hg_tag::read_data, rpc_read_data_in_t, rpc_data_out_t,
                                      NULL);

    rpc_trunc_data_id = MARGO_REGISTER(mid,
         hg_tag::trunc_data,
         rpc_trunc_in_t,
         rpc_err_out_t,
         NULL);

    rpc_get_dirents_id = MARGO_REGISTER(mid, hg_tag::get_dirents, rpc_get_dirents_in_t, rpc_get_dirents_out_t,
                                      NULL);

    rpc_chunk_stat_id = MARGO_REGISTER(mid,
        hg_tag::chunk_stat,
        rpc_chunk_stat_in_t,
        rpc_chunk_stat_out_t,
        NULL);
}

/**
 * Initializes the Margo client for a given na_plugin
 * @param mode
 * @param na_plugin
 * @return
 */
bool init_margo_client(const std::string& na_plugin) {
    // IMPORTANT: this struct needs to be zeroed before use
    struct hg_init_info hg_options = {};
#if USE_SHM
    hg_options.auto_sm = HG_TRUE;
#else
    hg_options.auto_sm = HG_FALSE;
#endif
    hg_options.stats = HG_FALSE;
    hg_options.na_class = nullptr;

    ld_margo_rpc_id = margo_init_opt(na_plugin.c_str(),
                                     MARGO_CLIENT_MODE,
                                     &hg_options,
                                     HG_FALSE,
                                     1);
    if (ld_margo_rpc_id == MARGO_INSTANCE_NULL) {
        CTX->log()->error("{}() margo_init_pool failed to initialize the Margo client", __func__);
        return false;
    }
    register_client_rpcs(ld_margo_rpc_id);
    return true;
}

/**
 * This function is only called in the preload constructor and initializes Argobots and Margo clients
 */
void init_ld_environment_() {

    //use rpc_addresses here to avoid "static initialization order problem"
    if (!init_margo_client(RPC_PROTOCOL)) {
        exit_error_msg(EXIT_FAILURE, "Unable to initializa Margo RPC client");
    }

    try {
        load_hosts();
    } catch (const std::exception& e) {
        exit_error_msg(EXIT_FAILURE, "Failed to load hosts addresses: "s + e.what());
    }

    /* Setup distributor */
    auto simple_hash_dist = std::make_shared<SimpleHashDistributor>(CTX->local_host_id(), CTX->hosts().size());
    CTX->distributor(simple_hash_dist);

    if (!rpc_send::get_fs_config()) {
        exit_error_msg(EXIT_FAILURE, "Unable to fetch file system configurations from daemon process through RPC.");
    }

    CTX->log()->info("{}() Environment initialization successful.", __func__);
}

void init_ld_env_if_needed() {
    pthread_once(&init_env_thread, init_ld_environment_);
}

void init_logging() {
    std::string path;
    try {
        path = gkfs::get_env_own("PRELOAD_LOG_PATH");
    } catch (const std::exception& e) {
        path = DEFAULT_PRELOAD_LOG_PATH;
    }

    spdlog::level::level_enum level;
    try {
        level = get_spdlog_level(gkfs::get_env_own("LOG_LEVEL"));
    } catch (const std::exception& e) {
        level = get_spdlog_level(DEFAULT_DAEMON_LOG_LEVEL);
    }

    auto logger_names = std::vector<std::string> {"main"};

    setup_loggers(logger_names, level, path);

    CTX->log(spdlog::get(logger_names.at(0)));
}

void log_prog_name() {
    std::string line;
    std::ifstream cmdline("/proc/self/cmdline");
    if (!cmdline.is_open()) {
        CTX->log()->error("Unable to open cmdline file");
        throw std::runtime_error("Unable to open cmdline file");
    }
    if(!getline(cmdline, line)) {
        throw std::runtime_error("Unable to read cmdline file");
    }
    std::replace(line.begin(), line.end(), '\0', ' ');
    CTX->log()->info("Command to itercept: '{}'", line);
    cmdline.close();
}

/**
 * Called initially ONCE when preload library is used with the LD_PRELOAD environment variable
 */
void init_preload() {
    init_logging();
    CTX->log()->debug("Initialized logging subsystem");
    log_prog_name();
    init_cwd();
    CTX->log()->debug("Current working directory: '{}'", CTX->cwd());
    init_ld_env_if_needed();
    CTX->enable_interception();
    CTX->log()->debug("{}() exit", __func__);
    start_interception();
}

/**
 * Called last when preload library is used with the LD_PRELOAD environment variable
 */
void destroy_preload() {
    stop_interception();
    CTX->disable_interception();
    if (ld_margo_rpc_id == nullptr) {
        CTX->log()->debug("{}() No services in preload library used. Nothing to shut down.", __func__);
        return;
    }
    cleanup_addresses();
    CTX->log()->debug("{}() About to finalize the margo RPC client", __func__);
    // XXX Sometimes this hangs on the cluster. Investigate.
    margo_finalize(ld_margo_rpc_id);
    CTX->log()->debug("{}() Shut down Margo RPC client successful", __func__);
    CTX->log()->info("All services shut down. Client shutdown complete.");
}
