# Changelog
All notable changes to GekkoFS project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
First GekkoFS pubblic release

This version provides a client library that uses GLibC I/O function interception.

## [0.3.1] - 2018-03-04
### Changed
- Read-write process improved. @Marc vef
- Improved Filemap. @Marc Vef
