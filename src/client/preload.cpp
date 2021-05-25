/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <client/preload.hpp>
#include <client/path.hpp>
#include <client/logging.hpp>
#include <client/rpc/forward_management.hpp>
#include <client/preload_util.hpp>
#include <client/intercept.hpp>

#include <global/rpc/distributor.hpp>
#include <global/global_defs.hpp>

#include <fstream>

#include <hermes.hpp>

extern "C" {
#include <sys/types.h>
}

using namespace std;

std::unique_ptr<hermes::async_engine> ld_network_service; // extern variable

namespace {
// make sure that things are only initialized once
pthread_once_t init_env_thread = PTHREAD_ONCE_INIT;

#ifdef GKFS_ENABLE_FORWARDING
pthread_t mapper;
bool forwarding_running;

pthread_mutex_t remap_mutex;
pthread_cond_t remap_signal;
#endif

inline void exit_error_msg(int errcode, const string& msg) {

    LOG_ERROR("{}", msg);
    gkfs::log::logger::log_message(stderr, "{}\n", msg);

    // if we don't disable interception before calling ::exit()
    // syscall hooks may find an inconsistent in shared state
    // (e.g. the logger) and thus, crash
    gkfs::preload::stop_interception();
    CTX->disable_interception();
    ::exit(errcode);
}

/**
 * Initializes the Hermes client for a given transport prefix
 * @return true if successfully initialized; false otherwise
 */
bool init_hermes_client() {

    try {

        hermes::engine_options opts{};

        if (CTX->auto_sm())
            opts |= hermes::use_auto_sm;
        if (gkfs::rpc::protocol::ofi_psm2 == CTX->rpc_protocol()) {
            opts |= hermes::force_no_block_progress;
        }

        ld_network_service =
                std::make_unique<hermes::async_engine>(
                        hermes::get_transport_type(CTX->rpc_protocol()), opts);
        ld_network_service->run();
    } catch (const std::exception& ex) {
        fmt::print(stderr, "Failed to initialize Hermes RPC client {}\n",
                   ex.what());
        return false;
    }
    return true;
}

/**
 * This function is only called in the preload constructor and initializes 
 * the file system client
 */
void init_ld_environment_() {

    vector<pair<string, string>> hosts{};
    try {
        LOG(INFO, "Loading peer addresses...");
        hosts = gkfs::util::read_hosts_file();
    } catch (const std::exception& e) {
        exit_error_msg(EXIT_FAILURE, "Failed to load hosts addresses: "s + e.what());
    }

    // initialize Hermes interface to Mercury
    LOG(INFO, "Initializing RPC subsystem...");

    if (!init_hermes_client()) {
        exit_error_msg(EXIT_FAILURE, "Unable to initialize RPC subsystem");
    }

    try {
        gkfs::util::connect_to_hosts(hosts);
    } catch (const std::exception& e) {
        exit_error_msg(EXIT_FAILURE, "Failed to connect to hosts: "s + e.what());
    }

    /* Setup distributor */
#ifdef GKFS_ENABLE_FORWARDING
    try {
        gkfs::util::load_forwarding_map();

        LOG(INFO, "{}() Forward to {}", __func__, CTX->fwd_host_id());
    } catch (std::exception& e){
        exit_error_msg(EXIT_FAILURE, fmt::format("Unable set the forwarding host '{}'", e.what()));
    }
    
    auto forwarder_dist = std::make_shared<gkfs::rpc::ForwarderDistributor>(CTX->fwd_host_id(), CTX->hosts().size());
    CTX->distributor(forwarder_dist);
#else
    auto simple_hash_dist = std::make_shared<gkfs::rpc::SimpleHashDistributor>(CTX->local_host_id(),
                                                                               CTX->hosts().size());
    CTX->distributor(simple_hash_dist);
#endif

    LOG(INFO, "Retrieving file system configuration...");

    if (!gkfs::rpc::forward_get_fs_config()) {
        exit_error_msg(EXIT_FAILURE, "Unable to fetch file system configurations from daemon process through RPC.");
    }

    LOG(INFO, "Environment initialization successful.");
}

#ifdef GKFS_ENABLE_FORWARDING
void *forwarding_mapper(void* p) {
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 10; // 10 seconds

    int previous = -1;

    while (forwarding_running) {
        try {
            gkfs::util::load_forwarding_map();

            if (previous != CTX->fwd_host_id()) {
                LOG(INFO, "{}() Forward to {}", __func__, CTX->fwd_host_id());

                previous = CTX->fwd_host_id();
            }
        } catch (std::exception& e) {
            exit_error_msg(EXIT_FAILURE, fmt::format("Unable set the forwarding host '{}'", e.what()));
        }

        pthread_mutex_lock(&remap_mutex);
        pthread_cond_timedwait(&remap_signal, &remap_mutex, &timeout);
        pthread_mutex_unlock(&remap_mutex);
    }

    return nullptr;
}
#endif

#ifdef GKFS_ENABLE_FORWARDING
void init_forwarding_mapper() {
    forwarding_running = true;

    pthread_create(&mapper, NULL, forwarding_mapper, NULL);
}
#endif

#ifdef GKFS_ENABLE_FORWARDING
void destroy_forwarding_mapper() {
    forwarding_running = false;

    pthread_cond_signal(&remap_signal);

    pthread_join(mapper, NULL);
}
#endif

void log_prog_name() {
    std::string line;
    std::ifstream cmdline("/proc/self/cmdline");
    if (!cmdline.is_open()) {
        LOG(ERROR, "Unable to open cmdline file");
        throw std::runtime_error("Unable to open cmdline file");
    }
    if (!getline(cmdline, line)) {
        throw std::runtime_error("Unable to read cmdline file");
    }
    std::replace(line.begin(), line.end(), '\0', ' ');
    line.erase(line.length() - 1, line.length());
    LOG(INFO, "Process cmdline: '{}'", line);
    cmdline.close();
}

} // namespace

namespace gkfs {
namespace preload {

void init_ld_env_if_needed() {
    pthread_once(&init_env_thread, init_ld_environment_);
}

} // namespace preload
} // namespace gkfs

/**
 * Called initially ONCE when preload library is used with the LD_PRELOAD environment variable
 */
void init_preload() {
    // The original errno value will be restored after initialization to not leak internal error codes
    auto oerrno = errno;

    CTX->enable_interception();
    gkfs::preload::start_self_interception();

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
    gkfs::path::init_cwd();

    LOG(DEBUG, "Current working directory: '{}'", CTX->cwd());
    gkfs::preload::init_ld_env_if_needed();
    CTX->enable_interception();

    CTX->unprotect_user_fds();

#ifdef GKFS_ENABLE_FORWARDING
    init_forwarding_mapper();
#endif

    gkfs::preload::start_interception();
    errno = oerrno;
}

/**
 * Called last when preload library is used with the LD_PRELOAD environment variable
 */
void destroy_preload() {
#ifdef GKFS_ENABLE_FORWARDING
    destroy_forwarding_mapper();
#endif

    CTX->clear_hosts();
    LOG(DEBUG, "Peer information deleted");

    ld_network_service.reset();
    LOG(DEBUG, "RPC subsystem shut down");

    gkfs::preload::stop_interception();
    CTX->disable_interception();
    LOG(DEBUG, "Syscall interception stopped");

    LOG(INFO, "All subsystems shut down. Client shutdown complete.");
}
