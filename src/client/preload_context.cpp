/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/

#include <hermes.hpp>
#include <client/preload_context.hpp>

#include <libsyscall_intercept_hook_point.h>
#include <syscall.h>

#include <client/env.hpp>
#include <global/env_util.hpp>
#include <client/logging.hpp>
#include <client/open_file_map.hpp>
#include <client/open_dir.hpp>
#include <client/resolve.hpp>

#include <global/path_util.hpp>
#include <cassert>
#include <cstdio>

decltype(PreloadContext::MIN_INTERNAL_FD) constexpr 
PreloadContext::MIN_INTERNAL_FD;
decltype(PreloadContext::MAX_USER_FDS) constexpr 
PreloadContext::MAX_USER_FDS;

PreloadContext::PreloadContext():
    ofm_(std::make_shared<OpenFileMap>()),
    fs_conf_(std::make_shared<FsConfig>()) {

    internal_fds_.set();
    internal_fds_must_relocate_ = true;
}

void 
PreloadContext::init_logging() {

    const std::string log_opts = 
        gkfs::env::get_var(gkfs::env::LOG, DEFAULT_CLIENT_LOG_LEVEL);

    const std::string log_output = 
        gkfs::env::get_var(gkfs::env::LOG_OUTPUT, DEFAULT_CLIENT_LOG_PATH);

#ifdef GKFS_DEBUG_BUILD
    // atoi returns 0 if no int conversion can be performed, which works
    // for us since if the user provides a non-numeric value we can just treat
    // it as zero
    const int log_verbosity = std::atoi(
            gkfs::env::get_var(gkfs::env::LOG_DEBUG_VERBOSITY, "0").c_str());

    const std::string log_filter =
        gkfs::env::get_var(gkfs::env::LOG_SYSCALL_FILTER, "");
#endif

    const std::string trunc_val = 
        gkfs::env::get_var(gkfs::env::LOG_OUTPUT_TRUNC);

    const bool log_trunc = !(!trunc_val.empty() && trunc_val[0] == 0);

    gkfs::log::create_global_logger(log_opts, log_output, log_trunc
#ifdef GKFS_DEBUG_BUILD
                                    , log_filter, log_verbosity
#endif
            );
}

void PreloadContext::mountdir(const std::string& path) {
    assert(is_absolute_path(path));
    assert(!has_trailing_slash(path));
    mountdir_components_ = split_path(path);
    mountdir_ = path;
}

const std::string& PreloadContext::mountdir() const {
    return mountdir_;
}

const std::vector<std::string>& PreloadContext::mountdir_components() const {
    return mountdir_components_;
}

void PreloadContext::cwd(const std::string& path) {
    cwd_ = path;
}

const std::string& PreloadContext::cwd() const {
    return cwd_;
}

const std::vector<hermes::endpoint>& PreloadContext::hosts() const {
    return hosts_;
}

void PreloadContext::hosts(const std::vector<hermes::endpoint>& endpoints) {
    hosts_ = endpoints;
}

void PreloadContext::clear_hosts() {
    hosts_.clear();
}

uint64_t PreloadContext::local_host_id() const {
    return local_host_id_;
}

void PreloadContext::local_host_id(uint64_t id) {
    local_host_id_ = id;
}

RelativizeStatus PreloadContext::relativize_fd_path(int dirfd,
                                                    const char * raw_path,
                                                    std::string& relative_path,
                                                    bool resolve_last_link) const {

    // Relativize path should be called only after the library constructor has been executed
    assert(interception_enabled_);
    // If we run the constructor we also already setup the mountdir
    assert(!mountdir_.empty());

    // We assume raw path is valid
    assert(raw_path != nullptr);

    std::string path;

    if (raw_path[0] != PSP) {
        // path is relative
        if (dirfd == AT_FDCWD) {
            // path is relative to cwd
            path = prepend_path(cwd_, raw_path);
        } else {
            if (!ofm_->exist(dirfd)) {
                return RelativizeStatus::fd_unknown;
            }
            // path is relative to fd
            auto dir = ofm_->get_dir(dirfd);
            if (dir == nullptr) {
                return RelativizeStatus::fd_not_a_dir;
            }
            path = mountdir_;
            path.append(dir->path());
            path.push_back(PSP);
            path.append(raw_path);
        }
    } else {
        path = raw_path;
    }

    if (resolve_path(path, relative_path, resolve_last_link)) {
        return RelativizeStatus::internal;
    }
    return RelativizeStatus::external;
}

bool PreloadContext::relativize_path(const char * raw_path, std::string& relative_path, bool resolve_last_link) const {
    // Relativize path should be called only after the library constructor has been executed
    assert(interception_enabled_);
    // If we run the constructor we also already setup the mountdir
    assert(!mountdir_.empty());

    // We assume raw path is valid
    assert(raw_path != nullptr);

    std::string path;

    if(raw_path[0] != PSP) {
        /* Path is not absolute, we need to prepend CWD;
         * First reserve enough space to minimize memory copy
         */
        path = prepend_path(cwd_, raw_path);
    } else {
        path = raw_path;
    }
    return resolve_path(path, relative_path, resolve_last_link);
}

const std::shared_ptr<OpenFileMap>& PreloadContext::file_map() const {
    return ofm_;
}

void PreloadContext::distributor(std::shared_ptr<Distributor> d) {
    distributor_ = d;
}

std::shared_ptr<Distributor> PreloadContext::distributor() const {
    return distributor_;
}

const std::shared_ptr<FsConfig>& PreloadContext::fs_conf() const {
    return fs_conf_;
}

void PreloadContext::enable_interception() {
    interception_enabled_ = true;
}

void PreloadContext::disable_interception() {
    interception_enabled_ = false;
}

bool PreloadContext::interception_enabled() const {
    return interception_enabled_;
}

int PreloadContext::register_internal_fd(int fd) {

    assert(fd >= 0);

    if(!internal_fds_must_relocate_) {
        LOG(DEBUG, "registering fd {} as internal (no relocation needed)", fd);
        assert(fd >= MIN_INTERNAL_FD);
        internal_fds_.reset(fd - MIN_INTERNAL_FD);
        return fd;
    }

    LOG(DEBUG, "registering fd {} as internal (needs relocation)", fd);

    std::lock_guard<std::mutex> lock(internal_fds_mutex_);
    const int pos = internal_fds_._Find_first();

    if(static_cast<std::size_t>(pos) == internal_fds_.size()) {
        throw std::runtime_error(
"Internal GekkoFS file descriptors exhausted, increase MAX_INTERNAL_FDS in "
"CMake, rebuild GekkoFS and try again.");
    }
    internal_fds_.reset(pos);


#if defined(GKFS_ENABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
    long args[gkfs::syscall::MAX_ARGS]{fd, pos + MIN_INTERNAL_FD, O_CLOEXEC};
#endif

    LOG(SYSCALL, 
        gkfs::syscall::from_internal_code | 
        gkfs::syscall::to_kernel |
        gkfs::syscall::not_executed, 
        SYS_dup3, args);

    const int ifd = 
        ::syscall_no_intercept(SYS_dup3, fd, pos + MIN_INTERNAL_FD, O_CLOEXEC);

    LOG(SYSCALL, 
        gkfs::syscall::from_internal_code | 
        gkfs::syscall::to_kernel |
        gkfs::syscall::executed, 
        SYS_dup3, args, ifd);

    assert(::syscall_error_code(ifd) == 0);

#if defined(GKFS_ENABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
    long args2[gkfs::syscall::MAX_ARGS]{fd};
#endif

    LOG(SYSCALL, 
        gkfs::syscall::from_internal_code | 
        gkfs::syscall::to_kernel |
        gkfs::syscall::not_executed, 
        SYS_close, args2);

#if defined(GKFS_ENABLE_LOGGING) && defined(GKFS_DEBUG_BUILD)
    int rv = ::syscall_no_intercept(SYS_close, fd);
#else
    ::syscall_no_intercept(SYS_close, fd);
#endif

    LOG(SYSCALL, 
        gkfs::syscall::from_internal_code | 
        gkfs::syscall::to_kernel |
        gkfs::syscall::executed, 
        SYS_close, args2, rv);

    LOG(DEBUG, "    (fd {} relocated to ifd {})", fd, ifd);

    return ifd;
}

void PreloadContext::unregister_internal_fd(int fd) {

    LOG(DEBUG, "unregistering internal fd {}", fd);

    assert(fd >= MIN_INTERNAL_FD);

    const auto pos = fd - MIN_INTERNAL_FD;

    std::lock_guard<std::mutex> lock(internal_fds_mutex_);
    internal_fds_.set(pos);
}

bool PreloadContext::is_internal_fd(int fd) const {

    if(fd < MIN_INTERNAL_FD) {
        return false;
    }

    const auto pos = fd - MIN_INTERNAL_FD;

    std::lock_guard<std::mutex> lock(internal_fds_mutex_);
    return !internal_fds_.test(pos);
}

void
PreloadContext::protect_user_fds() {

    LOG(DEBUG, "Protecting application fds [{}, {}]", 0, MAX_USER_FDS - 1);

    const int nullfd = ::syscall_no_intercept(SYS_open, "/dev/null", O_RDONLY);
    assert(::syscall_error_code(nullfd) == 0);
    protected_fds_.set(nullfd);

    const auto fd_is_open = [](int fd) -> bool {
        const int ret = ::syscall_no_intercept(SYS_fcntl, fd, F_GETFD);
        return ::syscall_error_code(ret) == 0 || 
               ::syscall_error_code(ret) != EBADF;
    };

    for(int fd = 0; fd < MAX_USER_FDS; ++fd) {
        if(fd_is_open(fd)) {
            LOG(DEBUG, "  fd {} was already in use, skipping", fd);
            continue;
        }

        const int ret = ::syscall_no_intercept(SYS_dup3, nullfd, fd, O_CLOEXEC);
        assert(::syscall_error_code(ret) == 0);
        protected_fds_.set(fd);
    }

    internal_fds_must_relocate_ = false;
}

void
PreloadContext::unprotect_user_fds() {

    for(std::size_t fd = 0; fd < protected_fds_.size(); ++fd) {
        if(!protected_fds_[fd]) {
            continue;
        }

        const int ret = 
            ::syscall_error_code(::syscall_no_intercept(SYS_close, fd));

        if(ret != 0) {
            LOG(ERROR, "Failed to unprotect fd")
        }
    }

    internal_fds_must_relocate_ = true;
}
