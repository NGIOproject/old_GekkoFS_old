# Changelog
All notable changes to GekkoFS project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.8.0] - 2020-09-15
## New
- Both client library and daemon have been extended to support the ofi+verbs
  protocol.
- A new Python testing harness has been implemented to support integration
  tests. The end goal is to increase the robustness of the code in the mid- to
  long-term.
- The RPC protocol and the usage of shared memory for intra-node communication
  no longer need to be activated on compile time. New arguments
  `-P|--rpc-protocol` and `--auto-sm` have been added to the daemon to this
  effect. This configuration options are propagated to clients when they
  initialize and contact daemons.
- Native support for the Omni-Path network protocol by choosing the `ofi+psm2`
  RPC protocol. Note that this requires `libfabric`'s version to be greater
  than `1.8` as well as `psm2` to be installed in the system. Clients must set
  `FI_PSM2_DISCONNECT=1` to be able to reconnect once the client is shut down
  once.
  *Known limitations:* Client reconnect doesn't always work. Apparently, if
  clients reconnect too fast the servers won't accept the connections. Also,
  currently more than 16 clients per node are not supported.
- A new execution mode called `GekkoFWD` that allows GekkoFS to run as
  a user-level I/O forwarding infrastructure for applications. In this mode,
  I/O operations from an application are intercepted and forwarded to a single
  GekkoFS daemon that is chosen according to a pre-defined distribution. In the
  daemons, the requests are scheduled using the AGIOS scheduling library before
  they are dispatched to the shared backend parallel file system.
- The `fsync()` system call is now fully supported.
## Improved
- Argobots tasks in the daemon are now wrapped in a dedicated class,
  effectively removing the dependency. This lays ground work for future
  non-Argobots I/O implementations.
- The `readdir()` implementation has been refactored and improved.
- Improvements on how to the installation scripts manage dependencies.
## Fixed
- The server sometimes crashed due to uncaught system errors in the storage
  backend. This has now been fixed.
- Fixed a bug that broke `ls` on some architectures.
- Fixed a bug that leaked internal errors from the interception library to
  client applications via `errno` propagation.

## [0.7.0] - 2020-02-05
## Added
 - Adedd support for `eventfd()`and `eventfd2()` system calls.
## Changed
 - Replaced Margo with Mercury in the client library in order to increase
   application compatibility: the Argobots ULTs used by Margo to send and
   process RPCs clashed at times with applications using pthreads.
 - Renamed environment variables to better distinguish which variables affect
   the client library (`LIBGKFS_*`) and which affect the daemon
   (`GKFS_DAEMON_*`).
 - Replaced spdlog in the client with a bespoke logging infrastructure:
   spdlog's internal threads and exception management often had issues with the
   system call interception infrastructure. The current logging infrastructure
   is designed around the syscall interception mechanism, and is therefore more
   stable.
 - Due to the new logging infrastructure, there have been significant changes
   to the environment variables controlling logging output. The desired log
   module is now set with `LIBGKFS_LOG`, while the desired output channel is
   controlled with `LIBGKFS_LOG_OUTPUT`. Additional options such as
   `LIBGKFS_LOG_OUTPUT_TRUNC`, `LOG_SYSCALL_FILTER` and `LOG_DEBUG_VERBOSITY`
   can be used to further control messages. Run the client with
   `LIBGKFS_LOG=help` for more details.
 - Improved dependency management in CMake.
## Fixed
 - Relocate internal file descriptors to a private range to avoid interfering
   with client application file descriptors.
 - Handle internal file descriptors created by `fcntl()`.
 - Handle internal file descriptors passed to processes using `CMSG_DATA` in
   `recvmsg()`.

## [0.6.2] - 2019-10-07
## Added
 - Paths inside kernel pseudo filesystems (`/sys`, `/proc`) are forwarded directly to the kernel and internal path resolution will be skipped. Be aware that also paths  like  `/sys/../tmp/gkfs_mountpoint/asd` will be forwarded to the kernel
 - Added new Cmake flag `CREATE_CHECK_PARENTS` to controls if the existance of the parent node needs to be checked during the creation of a child node.
## Changed
 - Daemon logs for RPC handlers have been polished
 - Updated Margo, Mercury and Libfabric dependencies
## Fixed
 - mk_node RPC wasn't propagating errors correctly from daemons
 - README has been improoved and got some minor fixes
 - fix wrong path in log call for mk_symlink function

## [0.6.1] - 2019-09-17
## Added
 - Added new Cmake flag `LOG_SYSCALLS` to enable/disable syscall logging.
 - Intercept the 64 bit version of `getdents`.
 - Added debian-based docker image.
## Changed
 - Disable syscalls logging by default
 - Update Mercury, RocksDB and Libfabric dependencies
## Fixed
 - Fix read at the end of file.
 - Don't create log file when using `--version`/`--help` cli flags.
 - On some systems LD_PRELOAD used on /bin/bash binary was not working.
 - Missing definition of `loff_t` on new version of GCC.

## [0.6.0] - 2019-07-26
## Added
- Add compile time option to disable shared memory communication `-DUSE_SHM:BOOL=OFF`
## Changed
- Deamons does not store anymore information about the others deamons.
- Improoved error handling on deamon initialization
- Decreased RPC timeout 3min -> 3sec
- Update 3rd party dependencies
## Removed
- PID file is not used anymore, we use only the new `hosts file` for out of bound communication
- Dropped CCI plugin support
- Dropped hostname-suffix cli option
- Dropped port cli option (use `--listen` instead)
- It is not needed anymore to pass hosts information to deamons, thus the `--hosts` cli have been removed
## Fixed
- Errors on get_dirents RPC are now reported back to clients
- Write errors happenig on deamons are now reported back to clients
- number overflow on lseek didn't allow to use seek on huge files

## [0.5.0] - 2019-04-29
## Changed
- Intercept I/O syscalls instead of GlibC function using [syscall intercept library](https://github.com/pmem/syscall_intercept)

## [0.4.0] - 2019-04-18
First GekkoFS public release

This version provides a client library that uses GLibC I/O function interception.

## [0.3.1] - 2018-03-04
### Changed
- Read-write process improved. @Marc vef
- Improved Filemap. @Marc Vef
