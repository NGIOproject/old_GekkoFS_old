# GekkoFS
This is a file system.

# Dependencies

## Rocksdb

### Debian/Ubuntu - Dependencies

- Upgrade your gcc to version at least 4.8 to get C++11 support.
- Install snappy. This is usually as easy as: `sudo apt-get install libsnappy-dev`
- Install zlib. Try: `sudo apt-get install zlib1g-dev`
- Install bzip2: `sudo apt-get install libbz2-dev`
- Install zstandard: `sudo apt-get install libzstd-dev`
- Install lz4 `sudo apt-get install liblz4-dev`

### CentOS/Red Hat - Dependencies
- Upgrade your gcc to version at least 4.8 to get C++11 support: yum install gcc48-c++
- Install snappy:
    `sudo yum install snappy snappy-devel`
- Install zlib:
    `sudo yum install zlib zlib-devel`
- Install bzip2:
    `sudo yum install bzip2 bzip2-devel`
- Install ASAN (optional for debugging):
    `sudo yum install libasan`
- Install zstandard:

```bash
   wget https://github.com/facebook/zstd/archive/v1.1.3.tar.gz
   mv v1.1.3.tar.gz zstd-1.1.3.tar.gz
   tar zxvf zstd-1.1.3.tar.gz
   cd zstd-1.1.3
   make && sudo make install
```

# Usage

## Clone and compile direct GekkoFS dependencies

- Go to the `scripts` folder and first clone all dependencies projects. You can choose the according na_plugin
(execute the script for help):

```bash
usage: dl_dep.sh [-h] [-l] [-n <NAPLUGIN>] [-c <CLUSTER>] [-d <DEPENDENCY>] 
                    source_path
	

This script gets all GekkoFS dependency sources (excluding the fs itself)

positional arguments:
        source_path              path where the dependency downloads are put


optional arguments:
        -h, --help              shows this help message and exits
        -l, --list-dependencies
                                list dependencies available for download
        -n <NAPLUGIN>, --na <NAPLUGIN>
                                network layer that is used for communication. Valid: {bmi,ofi,all}
                                defaults to 'all'
        -c <CLUSTER>, --cluster <CLUSTER>
                                additional configurations for specific compute clusters
                                supported clusters: {mogon1,mogon2,fh2}
        -d <DEPENDENCY>, --dependency <DEPENDENCY>
                                download a specific dependency. If unspecified 
                                all dependencies are built and installed.
```
- Now use the install script to compile them and install them to the desired directory. You can choose the according
na_plugin (execute the script for help):

```bash
usage: compile_dep.sh [-h] [-l] [-n <NAPLUGIN>] [-c <CLUSTER>] [-d <DEPENDENCY>] [-j <COMPILE_CORES>]
                      source_path install_path
	

This script compiles all GekkoFS dependencies (excluding the fs itself)

positional arguments:
    source_path 	path to the cloned dependencies path from clone_dep.sh
    install_path    path to the install path of the compiled dependencies


optional arguments:
    -h, --help  shows this help message and exits
    -l, --list-dependencies
                list dependencies available for building and installation
    -n <NAPLUGIN>, --na <NAPLUGIN>
                network layer that is used for communication. Valid: {bmi,ofi,all}
                defaults to 'all'
    -c <CLUSTER>, --cluster <CLUSTER>
                additional configurations for specific compute clusters
                supported clusters: {mogon1,mogon2,fh2}
    -d <DEPENDENCY>, --dependency <DEPENDENCY>
                build and install a specific dependency. If unspecified 
                all dependencies are built and installed.
    -j <COMPILE_CORES>, --compilecores <COMPILE_CORES>
                number of cores that are used to compile the dependencies
                defaults to number of available cores
    -t, --test  Perform libraries tests.
```

## Compile GekkoFS
You need to decide what Mercury NA plugin you want to use. The following NA plugins are available, although only BMI is considered stable at the moment.
 - `ofi+sockets` for using the libfabric plugin with TCP
 - `ofi+tcp` for using the libfabric plugin with TCP (new version)
 - `ofi+verbs` for using the libfabric plugin with Infiniband verbs (not threadsafe. Do not use.)
 - `ofi+psm2` for using the libfabric plugin with Intel Omni-Path
 - `bmi+tcp` for using the bmi plugin with the tcp protocol 

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DRPC_PROTOCOL='ofi+sockets' ..
make
make install
```

## Run GekkoFS

First on each node a daemon has to be started. This can be done in two ways using the `gkfs_daemon` binary directly or
the corresponding startup and shutdown scripts. The latter is recommended for cluster usage. It requires pssh (or
parallel-ssh) with python2.

### Start and shut down daemon directly

`./build/bin/gkfs_daemon -r <fs_data_path> -m <pseudo_mount_dir_path>`
 
Shut it down by gracefully killing the process.
 
### Startup and shutdown scripts

The scripts are located in `scripts/{startup_gkfs.py, shutdown_gkfs.py}`. Use the -h argument for their usage.

## Miscellaneous

Metadata and actual data will be stored at the `<fs_data_path>`. The path where the application works on is set with
`<pseudo_mount_dir_path>`
 
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
