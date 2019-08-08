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
usage: dl_dep.sh [-h] [-n <NAPLUGIN>] [-c <CLUSTER>]
                    source_path
 	

This script gets all GekkoFS dependency sources (excluding the fs itself)
 
positional arguments:
        source_path              path where the dependency downloads are put
 
 
optional arguments:
        -h, --help              shows this help message and exits
        -n <NAPLUGIN>, --na <NAPLUGIN>
                                network layer that is used for communication. Valid: {bmi,ofi,all}
                                defaults to 'all'
        -c <CLUSTER>, --cluster <CLUSTER>
                                additional configurations for specific compute clusters
                                supported clusters: {mogon1,fh2}
```
- Now use the install script to compile them and install them to the desired directory. You can choose the according
na_plugin (execute the script for help):

```bash
usage: compile_dep.sh [-h] [-n <NAPLUGIN>] [-c <CLUSTER>] [-j <COMPILE_CORES>]
                      source_path install_path
	
 
This script compiles all GekkoFS dependencies (excluding the fs itself)
 
positional arguments:
    source_path 	path to the cloned dependencies path from clone_dep.sh
    install_path    path to the install path of the compiled dependencies
 
 
optional arguments:
    -h, --help      shows this help message and exits
    -n <NAPLUGIN>, --na <NAPLUGIN>
                network layer that is used for communication. Valid: {bmi,ofi,all}
                defaults to 'all'
    -c <CLUSTER>, --cluster <CLUSTER>
                additional configurations for specific compute clusters
                supported clusters: {mogon1,mogon2,fh2}
    -j <COMPILE_CORES>, --compilecores <COMPILE_CORES>
                number of cores that are used to compile the depdencies
                defaults to number of available cores
```

## Compile GekkoFS
You need to decide what Mercury NA plugin you want to use. The following NA plugins are available, although only BMI is considered stable at the moment.
 - `ofi+tcp` for using the libfabric plugin with TCP
 - `ofi+verbs` for using the libfabric plugin with Infiniband verbs (not threadsafe. Do not use.)
 - `ofi+psm2` for using the libfabric plugin with Intel Omni-Path
 - `bmi+tcp` for using the bmi plugin with the tcp protocol 

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Run GekkoFS

First on each node a daemon has to be started. This can be done in two ways using the `gkfs_daemon` binary directly or
the corresponding startup and shutdown scripts. The latter is recommended for cluster usage. It requires pssh (or
parallel-ssh) with python2.

### Start and shut down daemon directly

`./build/bin/gkfs_daemon -r <fs_data_path> -m <pseudo_mount_dir_path> --hosts <hosts_comma_separated>`
 
Shut it down by gracefully killing the process.
 
### Startup and shutdown scripts

The scripts are located in `scripts/{startup_gkfs.py, shutdown_gkfs.py}`. Use the -h argument for their usage.

## Miscellaneous

Metadata and actual data will be stored at the `<fs_data_path>`. The path where the application works on is set with
`<pseudo_mount_dir_path>`
 
Run the application with the preload library: `LD_PRELOAD=<path>/build/lib/libiointer.so ./application`. In the case of
an MPI application use the `{mpirun, mpiexec} -x` argument.
 
### Logging
To enable logging the following environment variables are used:
GKFS_PRELOAD_LOG_PATH="<path/to/file>" to set the path to the logging file of the client library.
GKFS_DAEMON_LOG_PATH="<path/to/file>" to set the path to the logging file of the daemon.
GKFS_LOG_LEVEL={off,critical,err,warn,info,debug,trace} to set the trace level verbosity.
Numbers from 0-6 may also be used where as 0 is off and 6 represents trace.


### Acknowledgment

This software was partially supported by the EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

This software was partially supported by the ADA-FS project under the SPPEXA project funded by the DFG.