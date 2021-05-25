
| :warning: WARNING :warning:          |
|:-------------------------------------|
| *At the moment, this repository is only used for the purpose of publishing stable releases of GekkoFS. Please refer to the [official SSEC GitLab<sup>1</sup>](https://storage.bsc.es/gitlab/hpc/gekkofs) to report any issues, contribute PR or just to find the latest development version of GekkoFS.* |
|<sup>1</sup>Information on how to get an SSEC collaborator account can be found [here](https://storage.bsc.es/helpdesk/). |

# GekkoFS

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![pipeline status](https://storage.bsc.es/gitlab/hpc/gekkofs/badges/master/pipeline.svg)](https://storage.bsc.es/gitlab/hpc/gekkofs/commits/master)

GekkoFS is a file system capable of aggregating the local I/O capacity and performance of each compute node
in a HPC cluster to produce a high-performance storage space that can be accessed in a distributed manner.
This storage space allows HPC applications and simulations to run in isolation from each other with regards 
to I/O, which reduces interferences and improves performance.

# Dependencies

- Upgrade your gcc to version at least 4.8 to get C++11 support
- CMake >3.6 (>3.11 for GekkoFS testing)

## Debian/Ubuntu - Dependencies

- snappy: `sudo apt-get install libsnappy-dev`
- zlib: `sudo apt-get install zlib1g-dev`
- bzip2: `sudo apt-get install libbz2-dev`
- zstandard: `sudo apt-get install libzstd-dev`
- lz4: `sudo apt-get install liblz4-dev`
- uuid: `sudo apt-get install uuid-dev`
- capstone: `sudo apt-get install libcapstone-dev`

### CentOS/Red Hat - Dependencies


- snappy: `sudo yum install snappy snappy-devel`
- zlib: `sudo yum install zlib zlib-devel`
- bzip2: `sudo yum install bzip2 bzip2-devel`
- zstandard: 
```bash
   wget https://github.com/facebook/zstd/archive/v1.1.3.tar.gz
   mv v1.1.3.tar.gz zstd-1.1.3.tar.gz
   tar zxvf zstd-1.1.3.tar.gz
   cd zstd-1.1.3
   make && sudo make install
```
- lz4: `sudo yum install lz4 lz4-devel`
- uuid: `sudo yum install libuuid-devel`
- capstone: `sudo yum install capstone capstone-devel`


# Usage

## Clone and compile direct GekkoFS dependencies

- Go to the `scripts` folder and first clone all dependencies projects. You can choose the according network (na) plugin
(execute the script for help):

```bash
usage: dl_dep.sh [-h] [-l] [-n <NAPLUGIN>] [-c <CONFIG>] [-d <DEPENDENCY>]
                    source_path


This script gets all GekkoFS dependency sources (excluding the fs itself)

positional arguments:
        source_path              path where the dependency downloads are put


optional arguments:
        -h, --help              shows this help message and exits
        -l, --list-dependencies
                                list dependencies available for download with descriptions
        -n <NAPLUGIN>, --na <NAPLUGIN>
                                network layer that is used for communication. Valid: {bmi,ofi,all}
                                defaults to 'ofi'
        -c <CONFIG>, --config <CONFIG>
                                allows additional configurations, e.g., for specific clusters
                                supported values: {mogon2, mogon1, ngio, direct, all}
                                defaults to 'direct'
        -d <DEPENDENCY>, --dependency <DEPENDENCY>
                                download a specific dependency and ignore --config setting. If unspecified
                                all dependencies are built and installed based on set --config setting.
                                Multiple dependencies are supported: Pass a space-separated string (e.g., "ofi mercury"
        -v, --verbose           Increase download verbosity
```
- Now use the install script to compile them and install them to the desired directory. You can choose the according
(na) network plugin (execute the script for help):

```bash
usage: compile_dep.sh [-h] [-l] [-n <NAPLUGIN>] [-c <CONFIG>] [-d <DEPENDENCY>] [-j <COMPILE_CORES>]
                      source_path install_path


This script compiles all GekkoFS dependencies (excluding the fs itself)

positional arguments:
    source_path         path to the cloned dependencies path from clone_dep.sh
    install_path    path to the install path of the compiled dependencies


optional arguments:
    -h, --help  shows this help message and exits
    -l, --list-dependencies
                list dependencies available for building and installation
    -n <NAPLUGIN>, --na <NAPLUGIN>
                network layer that is used for communication. Valid: {bmi,ofi,all}
                defaults to 'all'
    -c <CONFIG>, --config <CONFIG>
                allows additional configurations, e.g., for specific clusters
                supported values: {mogon1, mogon2, ngio, direct, all}
                defaults to 'direct'
    -d <DEPENDENCY>, --dependency <DEPENDENCY>
                download a specific dependency and ignore --config setting. If unspecified
                all dependencies are built and installed based on set --config setting.
                Multiple dependencies are supported: Pass a space-separated string (e.g., "ofi mercury"
    -j <COMPILE_CORES>, --compilecores <COMPILE_CORES>
                number of cores that are used to compile the dependencies
                defaults to number of available cores
    -t, --test  Perform libraries tests.
```

## Compile GekkoFS

If above dependencies have been installed outside of the usual system paths, use CMake's `-DCMAKE_PREFIX_PATH` to
make this path known to CMake.

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

In order to build self-tests, the *optional* GKFS_BUILD_TESTS CMake option needs
to be enabled when building. Once that is done, tests can be run by running
`make test` in the `build` directory:

```bash
mkdir build && cd build
cmake -DGKFS_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
make
make test
make install
```

**IMPORTANT:** Please note that the testing framework requires Python 3.6 and >CMake 3.11 as
additional dependencies in order to run.

## Run GekkoFS

On each node a daemon (`gkfs_daemon` binary) has to be started. Other tools can be used to execute
the binary on many nodes, e.g., `srun`, `mpiexec/mpirun`, `pdsh`, or `pssh`.

You need to decide what Mercury NA plugin you want to use for network communication. `ofi+sockets` is the default.
The `-P` argument is used for setting another RPC protocol. See below.

 - `ofi+sockets` for using the libfabric plugin with TCP (stable)
 - `ofi+tcp` for using the libfabric plugin with TCP (slower than sockets)
 - `ofi+verbs` for using the libfabric plugin with Infiniband verbs (reasonably stable)
 - `ofi+psm2` for using the libfabric plugin with Intel Omni-Path (unstable)
 - `bmi+tcp` for using the bmi plugin with TCP (alternative to libfabric)

### Start and shut down daemon directly

`./build/src/daemon/gkfs_daemon -r <fs_data_path> -m <pseudo_mount_dir_path>`

Further options:
```bash
Allowed options:
  -h [ --help ]             Help message
  -m [ --mountdir ] arg     Virtual mounting directory where GekkoFS is 
                            available.
  -r [ --rootdir ] arg      Local data directory where GekkoFS data for this 
                            daemon is stored.
  -i [ --metadir ] arg      Metadata directory where GekkoFS RocksDB data
                            directory is located. If not set, rootdir is used.
  -l [ --listen ] arg       Address or interface to bind the daemon to. 
                            Default: local hostname.
                            When used with ofi+verbs the FI_VERBS_IFACE 
                            environment variable is set accordingly which 
                            associates the verbs device with the network 
                            interface. In case FI_VERBS_IFACE is already 
                            defined, the argument is ignored. Default 'ib'.
  -H [ --hosts-file ] arg   Shared file used by deamons to register their 
                            endpoints. (default './gkfs_hosts.txt')
  -P [ --rpc-protocol ] arg Used RPC protocol for inter-node communication.
                            Available: {ofi+sockets, ofi+verbs, ofi+psm2} for 
                            TCP, Infiniband, and Omni-Path, respectively. 
                            (Default ofi+sockets)
                            Libfabric must have enabled support verbs or psm2.
  --auto-sm                 Enables intra-node communication (IPCs) via the 
                            `na+sm` (shared memory) protocol, instead of using 
                            the RPC protocol. (Default off)
  --version                 Print version and exit.
```

Shut it down by gracefully killing the process (SIGTERM).

## Miscellaneous

Metadata and actual data will be stored at the `<fs_data_path>`. The path where the application works on is set with
`<pseudo_mount_dir_path>`.
 
Run the application with the preload library: `LD_PRELOAD=<path>/build/lib/libgkfs_intercept.so ./application`. In the case of
an MPI application use the `{mpirun, mpiexec} -x` argument.
 
### Logging
The following environment variables can be used to enable logging in the client
library: `LIBGKFS_LOG=<module>` and `LIBGKFS_LOG_OUTPUT=<path/to/file>` to
configure the output module and set the path to the log file of the client
library. If not path is specified in `LIBGKFS_LOG_OUTPUT`, the client library 
will send log messages to `/tmp/gkfs_client.log`.

The following modules are available:

 - `none`: don't print any messages
 - `syscalls`: Trace system calls: print the name of each system call, its
   arguments, and its return value. All system calls are printed after being
   executed save for those that may not return, such as `execve()`,
   `execve_at()`, `exit()`, and `exit_group()`. This module will only be
   available if the client library is built in `Debug` mode.
 - `syscalls_at_entry`: Trace system calls: print the name of each system call
   and its arguments. All system calls are printed before being executed and
   therefore their return values are not available in the log. This module will
   only be available if the client library is built in `Debug` mode.
 - `info`: Print information messages.
 - `critical`: Print critical errors.
 - `errors`: Print errors.
 - `warnings`: Print warnings.
 - `mercury`: Print Mercury messages.
 - `debug`: Print debug messages.  This module will only be available if the
   client library is built in `Debug` mode.
 - `most`: All previous options combined except `syscalls_at_entry`. This
   module will only be available if the client library is built in `Debug`
   mode.
 - `all`: All previous options combined.
 - `help`: Print a help message and exit.

When tracing sytem calls, specific syscalls can be removed from log messages by
setting the `LIBGKFS_LOG_SYSCALL_FILTER` environment variable. For instance,
setting it to `LIBGKFS_LOG_SYSCALL_FILTER=epoll_wait,epoll_create` will filter
out any log entries from the `epoll_wait()` and `epoll_create()` system calls.

Additionally, setting the `LIBGKFS_LOG_OUTPUT_TRUNC` environment variable with
a value different from `0` will instruct the logging subsystem to truncate 
the file used for logging, rather than append to it.

For the daemon, the `GKFS_DAEMON_LOG_PATH=<path/to/file>` environment variable 
can be provided to set the path to the log file, and the log module can be 
selected with the `GKFS_LOG_LEVEL={off,critical,err,warn,info,debug,trace}`
environment variable.


### Acknowledgment

This software was partially supported by the EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

This software was partially supported by the ADA-FS project under the SPPEXA project funded by the DFG.
