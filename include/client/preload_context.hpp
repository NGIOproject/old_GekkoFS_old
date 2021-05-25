/*
  Copyright 2018-2020, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2020, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#ifndef GEKKOFS_PRELOAD_CTX_HPP
#define GEKKOFS_PRELOAD_CTX_HPP

#include <hermes.hpp>
#include <map>
#include <mercury.h>
#include <memory>
#include <vector>
#include <string>

#include <bitset>

/* Forward declarations */
namespace gkfs {
namespace filemap {
class OpenFileMap;
}
namespace rpc {
class Distributor;
}
namespace log {
struct logger;
}

namespace preload {
/*
 * Client file system config
 */
struct FsConfig {
    // configurable metadata
    bool atime_state;
    bool mtime_state;
    bool ctime_state;
    bool link_cnt_state;
    bool blocks_state;

    uid_t uid;
    gid_t gid;

    std::string rootdir;

};

enum class RelativizeStatus {
    internal,
    external,
    fd_unknown,
    fd_not_a_dir
};

/**
 * Singleton class of the client context with all relevant global data
 */
class PreloadContext {

    static auto constexpr MIN_INTERNAL_FD = MAX_OPEN_FDS - MAX_INTERNAL_FDS;
    static auto constexpr MAX_USER_FDS = MIN_INTERNAL_FD;

private:
    PreloadContext();

    std::shared_ptr<gkfs::filemap::OpenFileMap> ofm_;
    std::shared_ptr<gkfs::rpc::Distributor> distributor_;
    std::shared_ptr<FsConfig> fs_conf_;

    std::string cwd_;
    std::vector<std::string> mountdir_components_;
    std::string mountdir_;

    std::vector<hermes::endpoint> hosts_;
    uint64_t local_host_id_;
    uint64_t fwd_host_id_;
    std::string rpc_protocol_;
    bool auto_sm_{false};

    bool interception_enabled_;

    std::bitset<MAX_INTERNAL_FDS> internal_fds_;
    mutable std::mutex internal_fds_mutex_;
    bool internal_fds_must_relocate_;
    std::bitset<MAX_USER_FDS> protected_fds_;

public:
    static PreloadContext* getInstance() {
        static PreloadContext instance;
        return &instance;
    }

    PreloadContext(PreloadContext const&) = delete;

    void operator=(PreloadContext const&) = delete;

    void init_logging();

    void mountdir(const std::string& path);

    const std::string& mountdir() const;

    const std::vector<std::string>& mountdir_components() const;

    void cwd(const std::string& path);

    const std::string& cwd() const;

    const std::vector<hermes::endpoint>& hosts() const;

    void hosts(const std::vector<hermes::endpoint>& addrs);

    void clear_hosts();

    uint64_t local_host_id() const;

    void local_host_id(uint64_t id);

    uint64_t fwd_host_id() const;

    void fwd_host_id(uint64_t id);

    const std::string& rpc_protocol() const;

    void rpc_protocol(const std::string& rpc_protocol);

    bool auto_sm() const;

    void auto_sm(bool auto_sm);

    RelativizeStatus relativize_fd_path(int dirfd,
                                        const char* raw_path,
                                        std::string& relative_path,
                                        bool resolve_last_link = true) const;

    bool relativize_path(const char* raw_path, std::string& relative_path, bool resolve_last_link = true) const;

    const std::shared_ptr<gkfs::filemap::OpenFileMap>& file_map() const;

    void distributor(std::shared_ptr<gkfs::rpc::Distributor> distributor);

    std::shared_ptr<gkfs::rpc::Distributor> distributor() const;

    const std::shared_ptr<FsConfig>& fs_conf() const;

    void enable_interception();

    void disable_interception();

    bool interception_enabled() const;

    int register_internal_fd(int fd);

    void unregister_internal_fd(int fd);

    bool is_internal_fd(int fd) const;

    void protect_user_fds();

    void unprotect_user_fds();
};

} // namespace preload
} // namespace gkfs


#endif //GEKKOFS_PRELOAD_CTX_HPP

