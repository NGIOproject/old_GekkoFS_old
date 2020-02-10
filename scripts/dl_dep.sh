#!/bin/bash

COMMON_CURL_FLAGS="--silent --fail --show-error --location -O"
COMMON_GIT_FLAGS="--quiet --single-branch"
PATCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PATCH_DIR="${PATCH_DIR}/patches"
DEPENDENCY=""
NA_LAYER=""
DEP_CONFIG=""
VERBOSE=false

VALID_DEP_OPTIONS="mogon2 direct all"

MOGON2_DEPS=(
    "zstd" "lz4" "snappy" "capstone" "ofi" "mercury" "argobots" "margo" "rocksdb"
    "syscall_intercept" "date"
)

DIRECT_DEPS=(
  "ofi" "mercury" "argobots" "margo" "rocksdb" "syscall_intercept" "date"
)

ALL_DEPS=(
    "zstd" "lz4" "snappy" "capstone" "bmi" "ofi" "mercury" "argobots" "margo" "rocksdb"
     "syscall_intercept" "date"
)

# Stop all backround jobs on interruption.
# "kill -- -$$" sends a SIGTERM to the whole process group,
# thus killing also descendants.
# Use single quotes, otherwise this expands now rather than when signalled.
# See shellcheck SC2064.
trap 'trap - SIGTERM && kill -- -$$' SIGINT SIGTERM

exit_child() {
    if [ ! $? -eq 0 ]; then
        # notify the parent
        kill -s SIGTERM -- -$$
    fi
}

error_exit() {
    echo "$1" >&2  ## Send message to stderr. Exclude >&2 if you don't want it that way.
    exit "${2:-1}" ## Return a code specified by $2 or 1 by default.
}

list_dependencies() {

    echo "Available dependencies: "

    echo -n "  Mogon 2: "
    for d in "${MOGON2_DEPS[@]}"; do
        echo -n "$d "
    done
    echo -n "  Direct GekkoFS dependencies: "
    for d in "${DIRECT_DEPS[@]}"; do
        echo -n "$d "
    done
    echo -n "  All: "
    for d in "${ALL_DEPS[@]}"; do
        echo -n "$d "
    done
    echo ""
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
#      [[ "${DEPENDENCY}" == "${DEP}" ]] && return
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

clonedeps() {
    if [[ "$VERBOSE" == true ]]; then
        set -ex
    else
        set -e
    fi
    trap exit_child EXIT

    local FOLDER=$1
    local REPO=$2
    local COMMIT=$3
    local GIT_FLAGS=$4
    local PATCH=$5

    local ACTION

    if [[ -d "${SOURCE}/${FOLDER}/.git" ]]; then
        cd "${SOURCE}/${FOLDER}" && git fetch -q
        ACTION="Pulled"
    else
        git clone ${COMMON_GIT_FLAGS} ${GIT_FLAGS} -- "${REPO}" "${SOURCE}/${FOLDER}"
        ACTION="Cloned"
    fi
    # fix the version
    cd "${SOURCE}/${FOLDER}" && git checkout -qf ${COMMIT}
    echo "${ACTION} ${FOLDER} [$COMMIT]"

    # apply patch if provided
    if [[ -n "${PATCH}" ]]; then
        git apply --verbose "${PATCH_DIR}/${PATCH}"
    fi
}

wgetdeps() {
    if [[ "$VERBOSE" == true ]]; then
        set -ex
    else
        set -e
    fi
    trap exit_child EXIT

    FOLDER=$1
    URL=$2
    if [[ -d "${SOURCE}/${FOLDER}" ]]; then
        # SC2115 Use "${var:?}" to ensure this never expands to /* .
        rm -rf "${SOURCE:?}/${FOLDER:?}"
    fi
    mkdir -p "${SOURCE}/${FOLDER}"
    cd "${SOURCE}"
    FILENAME=$(basename $URL)
    if [[ -f "${SOURCE}/$FILENAME" ]]; then
        rm -f "${SOURCE}/$FILENAME"
    fi
    curl ${COMMON_CURL_FLAGS} "$URL" || error_exit "Failed to download ${URL}" $?
    tar -xf "$FILENAME" --directory "${SOURCE}/${FOLDER}" --strip-components=1
    rm -f "$FILENAME"
    echo "Downloaded ${FOLDER}"
}

usage_short() {
	echo "
usage: dl_dep.sh [-h] [-l] [-n <NAPLUGIN>] [-c <CONFIG>] [-d <DEPENDENCY>]
                    source_path
	"
}

help_msg() {

    usage_short
    echo "
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
                                supported values: {mogon2, direct, all}
                                defaults to 'direct'
        -d <DEPENDENCY>, --dependency <DEPENDENCY>
                                download a specific dependency and ignore --config setting. If unspecified
                                all dependencies are built and installed based on set --config setting.
                                Multiple dependencies are supported: Pass a space-separated string (e.g., \"ofi mercury\"
        -v, --verbose           Increase download verbosity
        "
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
            exit 1
        fi
        DEPENDENCY="$2"
        shift # past argument
        shift # past value
        ;;
    -l | --list-dependencies)
        list_dependencies
        exit
        ;;
    -h | --help)
        help_msg
        exit
        ;;
    -v | --verbose)
        VERBOSE=true
        shift # past argument
        ;;
    *) # unknown option
        POSITIONAL+=("$1") # save it in an array for later
        shift              # past argument
        ;;
    esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

# positional arguments
if [[ -z ${1+x} ]]; then
    echo "ERROR: Positional arguments missing."
    usage_short
    exit 1
fi
SOURCE="$(readlink -mn "${1}")"

# optional arguments
if [[ "${NA_LAYER}" == "" ]]; then
    echo "Defaulting NAPLUGIN to 'ofi'"
    NA_LAYER="ofi"
fi

# enable predefined dependency template
case ${TMP_DEP_CONF} in
mogon2)
  DEP_CONFIG=("${MOGON2_DEPS[@]}")
  [[ -z "${DEPENDENCY}" ]] && echo "'Mogon2' dependencies are downloaded"
  ;;
all)
  DEP_CONFIG=("${ALL_DEPS[@]}")
  [[ -z "${DEPENDENCY}" ]] && echo "'All' dependencies are downloaded"
  ;;
direct | *)
  DEP_CONFIG=("${DIRECT_DEPS[@]}")
  [[ -z "${DEPENDENCY}" ]] && echo "'Direct' GekkoFS dependencies are downloaded (default)"
  ;;
esac

# sanity checks
if [[ "${NA_LAYER}" == "bmi" || "${NA_LAYER}" == "ofi" || "${NA_LAYER}" == "all" ]]; then
    echo NAPLUGIN = "${NA_LAYER}"
else
    echo "ERROR: No valid plugin selected"
    usage_short
    exit 1
fi

echo "Source path is set to  \"${SOURCE}\""
echo "------------------------------------"

mkdir -p "${SOURCE}"

## Third party dependencies

# get zstd for fast compression in rocksdb
if check_dependency "zstd" "${DEP_CONFIG[@]}"; then
    wgetdeps "zstd" "https://github.com/facebook/zstd/archive/v1.3.2.tar.gz" &
fi

# get zlib for rocksdb
if check_dependency "lz4" "${DEP_CONFIG[@]}"; then
    wgetdeps "lz4" "https://github.com/lz4/lz4/archive/v1.8.0.tar.gz" &
fi

# get snappy for rocksdb
if check_dependency "snappy" "${DEP_CONFIG[@]}"; then
    wgetdeps "snappy" "https://github.com/google/snappy/archive/1.1.7.tar.gz" &
fi

# get capstone for syscall-intercept
if check_dependency "capstone" "${DEP_CONFIG[@]}"; then
    wgetdeps "capstone" "https://github.com/aquynh/capstone/archive/4.0.1.tar.gz" &
fi

## Direct GekkoFS dependencies

# get BMI
if check_dependency "bmi" "${DEP_CONFIG[@]}"; then
    if [ "${NA_LAYER}" == "bmi" ] || [ "${NA_LAYER}" == "all" ]; then
        clonedeps "bmi" "https://xgitlab.cels.anl.gov/sds/bmi.git" "81ad0575fc57a69269a16208417cbcbefa51f9ea" &
    fi
fi

# get libfabric
if check_dependency "ofi" "${DEP_CONFIG[@]}"; then
    if [ "${NA_LAYER}" == "ofi" ] || [ "${NA_LAYER}" == "all" ]; then
        wgetdeps "libfabric" "https://github.com/ofiwg/libfabric/releases/download/v1.8.1/libfabric-1.8.1.tar.bz2" &
    fi
fi

# get Mercury
if check_dependency "mercury" "${DEP_CONFIG[@]}"; then
    clonedeps "mercury" "https://github.com/mercury-hpc/mercury" "fd410dfb9852b2b98d21113531f3058f45bfcd64"  "--recurse-submodules" &
fi

# get Argobots
if check_dependency "argobots" "${DEP_CONFIG[@]}"; then
    wgetdeps "argobots" "https://github.com/pmodels/argobots/archive/v1.0rc1.tar.gz" &
fi

# get Margo
if check_dependency "margo" "${DEP_CONFIG[@]}"; then
    clonedeps "margo" "https://xgitlab.cels.anl.gov/sds/margo.git" "016dbdce22da3fe4f97b46c20a53bced9370a217" &
fi

# get rocksdb
if check_dependency "rocksdb" "${DEP_CONFIG[@]}"; then
    wgetdeps "rocksdb" "https://github.com/facebook/rocksdb/archive/v6.2.2.tar.gz" &
fi

# get syscall_intercept
if check_dependency "syscall_intercept" "${DEP_CONFIG[@]}"; then
    clonedeps "syscall_intercept" "https://github.com/pmem/syscall_intercept.git" "cc3412a2ad39f2e26cc307d5b155232811d7408e" "" "syscall_intercept.patch" &
fi

# get date
if check_dependency "date" "${DEP_CONFIG[@]}"; then
    clonedeps "date" "https://github.com/HowardHinnant/date.git" "e7e1482087f58913b80a20b04d5c58d9d6d90155" &
fi
# Wait for all download to be completed
wait
echo "Done"