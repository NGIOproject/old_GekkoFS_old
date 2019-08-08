#!/bin/bash

usage_short() {
	echo "
usage: compile_dep.sh [-h] [-n <NAPLUGIN>] [-c <CLUSTER>] [-j <COMPILE_CORES>]
                      source_path install_path
	"
}

help_msg() {

	usage_short
    echo "
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
    -t, --test  Perform libraries tests.
"
}

prepare_build_dir() {
    if [ ! -d "$1/build" ]; then
        mkdir $1/build
    fi
    rm -rf $1/build/*
}

find_cmake() {
    local CMAKE=`command -v cmake3 || command -v cmake`
    if [ $? -ne 0 ]; then
        >&2 echo "ERROR: could not find cmake"
        exit 1
    fi
    echo ${CMAKE}
}

PATCH_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PATCH_DIR="${PATCH_DIR}/patches"
CLUSTER=""
NA_LAYER=""
CORES=""
SOURCE=""
INSTALL=""

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case ${key} in
    -n|--na)
    NA_LAYER="$2"
    shift # past argument
    shift # past value
    ;;
	-c|--cluster)
    CLUSTER="$2"
    shift # past argument
    shift # past value
    ;;
	-j|--compilecores)
    CORES="$2"
    shift # past argument
    shift # past value
    ;;
    -t|--test)
    PERFORM_TEST=true
    shift
    ;;
    -h|--help)
    help_msg
	exit
    #shift # past argument
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

# deal with positional arguments
if [[ ( -z ${1+x} ) || ( -z ${2+x} ) ]]; then
    echo "Positional arguments missing."
    usage_short
    exit 1
fi
SOURCE="$( readlink -mn "${1}" )"
INSTALL="$( readlink -mn "${2}" )"

# deal with optional arguments
if [ "${NA_LAYER}" == "" ]; then
	echo "Defaulting NAPLUGIN to 'all'"
	NA_LAYER="all"
fi
if [ "${CORES}" == "" ]; then
	CORES=$(grep -c ^processor /proc/cpuinfo)
	echo "CORES = ${CORES} (default)"
else
	if [ ! "${CORES}" -gt "0" ]; then
		echo "CORES set to ${CORES} which is invalid.
Input must be numeric and greater than 0."
		usage_short
		exit
	else
		echo CORES    = "${CORES}"
	fi
fi
if [ "${NA_LAYER}" == "bmi" ] || [ "${NA_LAYER}" == "ofi" ] || [ "${NA_LAYER}" == "all" ]; then
	echo NAPLUGIN = "${NA_LAYER}"
else
    echo "No valid plugin selected"
    usage_short
    exit
fi
if [[ "${CLUSTER}" != "" ]]; then
	if [[ ( "${CLUSTER}" == "mogon1" ) || ( "${CLUSTER}" == "fh2" ) || ( "${CLUSTER}" == "mogon2" ) ]]; then
		echo CLUSTER  = "${CLUSTER}"
    else
        echo "${CLUSTER} cluster configuration is invalid. Exiting ..."
        usage_short
        exit
    fi
else
    echo "No cluster configuration set."
fi

USE_BMI="-DNA_USE_BMI:BOOL=OFF"
USE_OFI="-DNA_USE_OFI:BOOL=OFF"

CMAKE=`find_cmake`
CMAKE="${CMAKE} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"

echo "Source path = ${SOURCE}";
echo "Install path = ${INSTALL}";

mkdir -p ${SOURCE}

######### From now on exits on any error ########
set -e

export CPATH="${CPATH}:${INSTALL}/include"
export LIBRARY_PATH="${LIBRARY_PATH}:${INSTALL}/lib:${INSTALL}/lib64"

# Set cluster dependencies first
if [[ ( "${CLUSTER}" == "mogon1" ) || ( "${CLUSTER}" == "fh2" ) || ( "${CLUSTER}" == "mogon2" ) ]]; then
    # compile zstd
    echo "############################################################ Installing:  zstd"
    CURR=${SOURCE}/zstd/build/cmake
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    $CMAKE -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release ..
    make -j${CORES}
    make install
    echo "############################################################ Installing:  lz4"
    CURR=${SOURCE}/lz4
	cd ${CURR}
    make -j${CORES}
    make DESTDIR=${INSTALL} PREFIX="" install
    echo "############################################################ Installing:  snappy"
    CURR=${SOURCE}/snappy
    prepare_build_dir ${CURR}
    cd ${CURR}/build
    $CMAKE -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release ..
    make -j${CORES}
    make install
fi

if [ "$NA_LAYER" == "bmi" ] || [ "$NA_LAYER" == "all" ]; then
    USE_BMI="-DNA_USE_BMI:BOOL=ON"
    echo "############################################################ Installing:  BMI"
    # BMI
    CURR=${SOURCE}/bmi
    prepare_build_dir ${CURR}
    cd ${CURR}
    ./prepare
    cd ${CURR}/build
    CFLAGS="${CFLAGS} -w" ../configure --prefix=${INSTALL} --enable-shared --disable-static --disable-karma --enable-bmi-only --enable-fast --disable-strict
    make -j${CORES}
    make install
fi

if [ "$NA_LAYER" == "ofi" ] || [ "$NA_LAYER" == "all" ]; then
    USE_OFI="-DNA_USE_OFI:BOOL=ON"
    # Mogon2 already has libfabric installed in a version that Mercury supports.
    if [[ ("${CLUSTER}" != "mogon2") ]]; then
        echo "############################################################ Installing:  LibFabric"
        #libfabric
        CURR=${SOURCE}/libfabric
        prepare_build_dir ${CURR}
        cd ${CURR}/build
        ../configure --prefix=${INSTALL} --enable-tcp=yes
        make -j${CORES}
        make install
        [ "${PERFORM_TEST}" ] && make check
    fi
fi

echo "############################################################ Installing:  Mercury"

# Mercury
CURR=${SOURCE}/mercury
prepare_build_dir ${CURR}
cd ${CURR}/build
$CMAKE \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DBUILD_TESTING:BOOL=ON \
    -DMERCURY_USE_SM_ROUTING:BOOL=ON \
    -DMERCURY_USE_SELF_FORWARD:BOOL=ON \
    -DMERCURY_USE_CHECKSUMS:BOOL=OFF \
    -DMERCURY_USE_BOOST_PP:BOOL=ON \
    -DMERCURY_USE_EAGER_BULK:BOOL=ON \
    -DBUILD_SHARED_LIBS:BOOL=ON \
    -DCMAKE_INSTALL_PREFIX=${INSTALL} \
    ${USE_BMI} ${USE_OFI} \
    ..
make -j${CORES}
make install

echo "############################################################ Installing:  Argobots"

# Argobots
CURR=${SOURCE}/argobots
prepare_build_dir ${CURR}
cd ${CURR}
./autogen.sh
cd ${CURR}/build
../configure --prefix=${INSTALL} --enable-perf-opt --disable-checks
make -j${CORES}
make install
[ "${PERFORM_TEST}" ] && make check

echo "############################################################ Installing:  Margo"
# Margo
CURR=${SOURCE}/margo
prepare_build_dir ${CURR}
cd ${CURR}
./prepare.sh
cd ${CURR}/build
../configure --prefix=${INSTALL} PKG_CONFIG_PATH=${INSTALL}/lib/pkgconfig CFLAGS="${CFLAGS} -Wall -O3"
make -j${CORES}
make install
[ "${PERFORM_TEST}" ] && make check

echo "############################################################ Installing:  Rocksdb"
# Rocksdb
CURR=${SOURCE}/rocksdb
cd ${CURR}
make clean
USE_RTTI=1 make -j${CORES} static_lib
INSTALL_PATH=${INSTALL} make install


echo "############################################################ Installing:  Capstone (syscall intercept dependency)"
CURR=${SOURCE}/capstone
prepare_build_dir ${CURR}
cd ${CURR}/build
$CMAKE -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release ..
make -j${CORES} install

echo "############################################################ Installing:  Syscall_intercept"
CURR=${SOURCE}/syscall_intercept
prepare_build_dir ${CURR}
cd ${CURR}/build
PKG_CONFIG_PATH="/home/nx01/shared/GekkoFS-BSC/0.6slurm/lib/pkgconfig" $CMAKE -DCMAKE_INSTALL_PREFIX=${INSTALL} -DCMAKE_BUILD_TYPE:STRING=Release -DBUILD_EXAMPLES:BOOL=OFF -DBUILD_TESTS:BOOK=OFF ..
make install

echo "Done"
