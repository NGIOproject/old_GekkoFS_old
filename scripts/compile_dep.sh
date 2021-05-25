#!/bin/bash

PATCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PATCH_DIR="${PATCH_DIR}/patches"
DEPENDENCY=""
NA_LAYER=""
CORES=""
SOURCE=""
INSTALL=""
DEP_CONFIG=""

VALID_DEP_OPTIONS="mogon2 mogon1 ngio direct all"

MOGON1_DEPS=(
    "zstd" "lz4" "snappy" "capstone" "ofi" "mercury" "argobots" "margo" "rocksdb"
    "syscall_intercept" "date" "verbs"
)

MOGON2_DEPS=(
    "bzip2" "zstd" "lz4" "snappy" "capstone" "ofi" "mercury" "argobots" "margo" "rocksdb"
    "syscall_intercept" "date" "psm2"
)

NGIO_DEPS=(
    "zstd" "lz4" "snappy" "capstone" "ofi" "mercury" "argobots" "margo" "rocksdb"
    "syscall_intercept" "date" "agios" "psm2"
)

DIRECT_DEPS=(
  "ofi" "mercury" "argobots" "margo" "rocksdb" "syscall_intercept" "date"
)

ALL_DEPS=(
    "bzip2" "zstd" "lz4" "snappy" "capstone" "bmi" "ofi" "mercury" "argobots" "margo" "rocksdb"
     "syscall_intercept" "date" "agios"
)

usage_short() {
	echo "
usage: compile_dep.sh [-h] [-l] [-n <NAPLUGIN>] [-c <CONFIG>] [-d <DEPENDENCY>] [-j <COMPILE_CORES>]
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
                Multiple dependencies are supported: Pass a space-separated string (e.g., \"ofi mercury\"
    -j <COMPILE_CORES>, --compilecores <COMPILE_CORES>
                number of cores that are used to compile the dependencies
                defaults to number of available cores
    -t, --test  Perform libraries tests.
"
}

list_dependencies() {

    echo "Available dependencies: "

    echo -n "  Mogon 1: "
    for d in "${MOGON1_DEPS[@]}"; do
        echo -n "$d "
    done
	echo
    echo -n "  Mogon 2: "
    for d in "${MOGON2_DEPS[@]}"; do
        echo -n "$d "
    done
	echo
    echo -n "  NGIO: "
    for d in "${NGIO_DEPS[@]}"; do
        echo -n "$d "
    done
	echo
    echo -n "  Direct GekkoFS dependencies: "
    for d in "${DIRECT_DEPS[@]}"; do
        echo -n "$d "
    done
	echo
    echo -n "  All: "
    for d in "${ALL_DEPS[@]}"; do
        echo -n "$d "
    done
    echo
}

check_dependency() {
  local DEP=$1
  shift
  local DEP_CONFIG=("$@")
  # ignore template when specific dependency is set
  if [[ -n "${DEPENDENCY}" ]]; then
      # check if specific dependency was set and return from function
      if echo "${DEPENDENCY}" | grep -q "${DEP}"; then
        return
      fi
  else
      # if not check if dependency is part of dependency config
      for e in "${DEP_CONFIG[@]}"; do
        if [[ "${DEP}" == "${e}" ]]; then
          return
        fi
      done
  fi
  false
}

prepare_build_dir() {
    if [ ! -d "$1/build" ]; then
        mkdir "$1"/build
    fi
    rm -rf "$1"/build/*
}

find_cmake() {
    local CMAKE
    CMAKE=$(command -v cmake3 || command -v cmake)
    if [ $? -ne 0 ]; then
        echo >&2 "ERROR: could not find cmake"
        exit 1
    fi
    echo "${CMAKE}"
}

POSITIONAL=()
while [[ $# -gt 0 ]]; do
    key="$1"

    case ${key} in
    -n | --na)
        NA_LAYER="$2"
        shift # past argument
        shift # past value
        ;;
    -c | --config)
        if [[ -z "$2" ]]; then
            echo "ERROR: Missing argument for -c/--config option"
            exit 1
        fi
        if ! echo "$VALID_DEP_OPTIONS" | grep -q "$2"; then
            echo "ERROR: Invalid argument for -c/--config option"
            exit 1
        fi
        TMP_DEP_CONF="$2"
        shift # past argument
        shift # past value
        ;;
    -d | --dependency)
        if [[ -z "$2" ]]; then
            echo "ERROR: Missing argument for -d/--dependency option"
            exit
        fi
        DEPENDENCY="$2"
        shift # past argument
        shift # past value
        ;;
    -j | --compilecores)
        CORES="$2"
        shift # past argument
        shift # past value
        ;;
    -t | --test)
        PERFORM_TEST=true
        shift
        ;;
    -l | --list-dependencies)
        list_dependencies
        exit
        ;;
    -h | --help)
        help_msg
        exit
        #shift # past argument
        ;;
    *) # unknown option
        POSITIONAL+=("$1") # save it in an array for later
        shift              # past argument
        ;;
    esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

# deal with positional arguments
if [[ (-z ${1+x}) || (-z ${2+x}) ]]; then
    echo "ERROR: Positional arguments missing."
    usage_short
    exit 1
fi
SOURCE="$(readlink -mn "${1}")"
INSTALL="$(readlink -mn "${2}")"

# deal with optional arguments
if [[ "${NA_LAYER}" == "" ]]; then
    echo "Defaulting NAPLUGIN to 'ofi'"
    NA_LAYER="ofi"
fi
if [[ "${CORES}" == "" ]]; then
    CORES=$(grep -c ^processor /proc/cpuinfo)
    echo "CORES = ${CORES} (default)"
else
    if [[ ! "${CORES}" -gt "0" ]]; then
        echo "ERROR: CORES set to ${CORES} which is invalid.
Input must be numeric and greater than 0."
        usage_short
        exit
    else
        echo CORES = "${CORES}"
    fi
fi
if [[ "${NA_LAYER}" == "bmi" || "${NA_LAYER}" == "ofi" || "${NA_LAYER}" == "all" ]]; then
    echo NAPLUGIN = "${NA_LAYER}"
else
    echo "ERROR: No valid NA plugin selected"
    usage_short
    exit
fi
# enable predefined dependency template
case ${TMP_DEP_CONF} in
mogon1)
  DEP_CONFIG=("${MOGON1_DEPS[@]}")
  echo "'Mogon1' dependencies are compiled"
  ;;
mogon2)
  DEP_CONFIG=("${MOGON2_DEPS[@]}")
  echo "'Mogon2' dependencies are compiled"
  ;;
ngio)
  DEP_CONFIG=("${NGIO_DEPS[@]}")
  echo "'NGIO' dependencies are compiled"
  ;;
all)
  DEP_CONFIG=("${ALL_DEPS[@]}")
  echo "'All' dependencies are compiled"
  ;;
direct | *)
  DEP_CONFIG=("${DIRECT_DEPS[@]}")
  echo "'Direct' GekkoFS dependencies are compiled (default)"
  ;;
esac

USE_BMI="-DNA_USE_BMI:BOOL=OFF"
USE_OFI="-DNA_USE_OFI:BOOL=OFF"

CMAKE=$(find_cmake)
CMAKE="${CMAKE} -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"

echo "Source path = ${SOURCE}"
echo "Install path = ${INSTALL}"
echo "------------------------------------"

mkdir -p "${SOURCE}"

######### From now on exits on any error ########
set -e

export CPATH="${CPATH}:${INSTALL}/include"
export LIBRARY_PATH="${LIBRARY_PATH}:${INSTALL}/lib:${INSTALL}/lib64"
export PKG_CONFIG_PATH="${INSTALL}/lib/pkgconfig:${PKG_CONFIG_PATH}"

## Third party dependencies

# Set cluster dependencies first

# build zstd for fast compression in rocksdb
if check_dependency "zstd" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  zstd"
    CURR=${SOURCE}/zstd/build/cmake
    prepare_build_dir "${CURR}"
    cd "${CURR}"/build
    $CMAKE -DCMAKE_INSTALL_PREFIX="${INSTALL}" -DCMAKE_BUILD_TYPE:STRING=Release ..
    make -j"${CORES}"
    make install
fi

# build zlib for rocksdb
if check_dependency "lz4" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  lz4"
    CURR=${SOURCE}/lz4
    cd "${CURR}"
    make -j"${CORES}"
    make DESTDIR="${INSTALL}" PREFIX="" install
fi

# build snappy for rocksdb
if check_dependency "snappy" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  snappy"
    CURR=${SOURCE}/snappy
    prepare_build_dir "${CURR}"
    cd "${CURR}"/build
    $CMAKE -DCMAKE_INSTALL_PREFIX="${INSTALL}" -DCMAKE_BUILD_TYPE:STRING=Release ..
    make -j"${CORES}"
    make install
fi

# build bzip2 for rocksdb
if check_dependency "bzip2" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  bzip2"
    CURR=${SOURCE}/bzip2
    cd "${CURR}"
    make install PREFIX="${INSTALL}"
fi

# build capstone for syscall-intercept
if check_dependency "capstone" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  capstone"
    CURR=${SOURCE}/capstone
    prepare_build_dir "${CURR}"
    cd "${CURR}"/build
    $CMAKE -DCMAKE_INSTALL_PREFIX="${INSTALL}" -DCMAKE_BUILD_TYPE:STRING=Release ..
    make -j"${CORES}" install
fi

# build BMI
if check_dependency "bmi" "${DEP_CONFIG[@]}"; then
    if [[ "${NA_LAYER}" == "bmi" || "${NA_LAYER}" == "all" ]]; then
        USE_BMI="-DNA_USE_BMI:BOOL=ON"
        echo "############################################################ Installing:  BMI"
        # BMI
        CURR=${SOURCE}/bmi
        prepare_build_dir "${CURR}"
        cd "${CURR}"
        ./prepare
        cd "${CURR}"/build
        CFLAGS="${CFLAGS} -w" ../configure --prefix="${INSTALL}" --enable-shared --disable-static --disable-karma --enable-bmi-only --enable-fast --disable-strict
        make -j"${CORES}"
        make install
    fi
fi

# build ofi
if check_dependency "ofi" "${DEP_CONFIG[@]}"; then
    if [[ "${NA_LAYER}" == "ofi" || "${NA_LAYER}" == "all" ]]; then
        USE_OFI="-DNA_USE_OFI:BOOL=ON"
        echo "############################################################ Installing:  LibFabric"
        #libfabric
        CURR=${SOURCE}/libfabric
        prepare_build_dir ${CURR}
        cd ${CURR}
        ./autogen.sh
        cd ${CURR}/build
        OFI_CONFIG="../configure --prefix=${INSTALL} --enable-tcp=yes"
        if check_dependency "verbs" "${DEP_CONFIG[@]}"; then
            OFI_CONFIG="${OFI_CONFIG} --enable-verbs=yes"
        elif check_dependency "psm2" "${DEP_CONFIG[@]}"; then
            OFI_CONFIG="${OFI_CONFIG} --enable-psm2=yes --with-psm2-src=${SOURCE}/psm2"
        elif check_dependency "psm2-system" "${DEP_CONFIG[@]}"; then
            OFI_CONFIG="${OFI_CONFIG} --enable-psm2=yes"
        fi
         ${OFI_CONFIG}
        make -j${CORES}
        make install
        [ "${PERFORM_TEST}" ] && make check
    fi
fi

# AGIOS
if check_dependency "agios" "${DEP_CONFIG[@]}"; then
	echo "############################################################ Installing:  AGIOS"
	CURR=${SOURCE}/agios
	prepare_build_dir "${CURR}"
	cd "${CURR}"/build
	$CMAKE -DCMAKE_INSTALL_PREFIX="${INSTALL}" ..
	make install
fi

# Mercury
if check_dependency "mercury" "${DEP_CONFIG[@]}"; then

    if [[ "${NA_LAYER}" == "bmi" || "${NA_LAYER}" == "all" ]]; then
        USE_BMI="-DNA_USE_BMI:BOOL=ON"
    fi

    if [[ "${NA_LAYER}" == "ofi" || "${NA_LAYER}" == "all" ]]; then
        USE_OFI="-DNA_USE_OFI:BOOL=ON"
    fi

    echo "############################################################ Installing:  Mercury"
    CURR=${SOURCE}/mercury
    prepare_build_dir "${CURR}"
    cd "${CURR}"/build
    PKG_CONFIG_PATH=${INSTALL}/lib/pkgconfig $CMAKE \
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
    make -j"${CORES}"
    make install
fi

# Argobots
if check_dependency "argobots" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  Argobots"
    CURR=${SOURCE}/argobots
    prepare_build_dir "${CURR}"
    cd "${CURR}"
    ./autogen.sh
    cd "${CURR}"/build
    ../configure --prefix="${INSTALL}" --enable-perf-opt --disable-checks
    make -j"${CORES}"
    make install
    [ "${PERFORM_TEST}" ] && make check
fi

# Margo
if check_dependency "margo" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  Margo"
    CURR=${SOURCE}/margo
    prepare_build_dir "${CURR}"
    cd "${CURR}"
    ./prepare.sh
    cd "${CURR}"/build
    ../configure --prefix="${INSTALL}" PKG_CONFIG_PATH="${INSTALL}"/lib/pkgconfig CFLAGS="${CFLAGS} -Wall -O3"
    make -j"${CORES}"
    make install
    [ "${PERFORM_TEST}" ] && make check
fi

# Rocksdb
if check_dependency "rocksdb" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  Rocksdb"
    CURR=${SOURCE}/rocksdb
    cd "${CURR}"
    make clean
    USE_RTTI=1 make -j"${CORES}" static_lib
    INSTALL_PATH="${INSTALL}" make install
fi

# syscall_intercept
if check_dependency "syscall_intercept" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  Syscall_intercept"
    CURR=${SOURCE}/syscall_intercept
    prepare_build_dir "${CURR}"
    cd "${CURR}"/build
    $CMAKE -DCMAKE_PREFIX_PATH="${INSTALL}" -DCMAKE_INSTALL_PREFIX="${INSTALL}" -DCMAKE_BUILD_TYPE:STRING=Debug -DBUILD_EXAMPLES:BOOL=OFF -DBUILD_TESTS:BOOK=OFF ..
    make install
fi

# date
if check_dependency "date" "${DEP_CONFIG[@]}"; then
    echo "############################################################ Installing:  date"
    CURR=${SOURCE}/date
    prepare_build_dir "${CURR}"
    cd "${CURR}"/build
    $CMAKE -DCMAKE_INSTALL_PREFIX="${INSTALL}" -DCMAKE_CXX_STANDARD:STRING=14 -DUSE_SYSTEM_TZ_DB:BOOL=ON -DBUILD_SHARED_LIBS:BOOL=ON ..
    make install
fi

echo "Done"
