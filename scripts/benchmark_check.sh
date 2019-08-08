#!/usr/bin/env bash

usage_short() {
	echo "
usage: benchmark_check.sh [-h] [-d <TESTPATH>]
                      mdtest_bin_path ior_bin_path test_dir
	"
}

help_msg() {

	usage_short
    echo "
This script executes a number of benchmark experiments to catch most default use cases. For now, only local

positional arguments:
    mdtest_bin_path 	path to the mdtest binary
    ior_bin_path        path to the ior binary
    test_dir            path to directory where the tests will be run


optional arguments:
    -h, --help      shows this help message and exits
    --preload       libs that are added to LD_PRELOAD. E.g., for GekkoFS client lib (defaults to nothing)
    --mdtestonly    executes only mdtest tests (default both are run)
    --ioronly       executes only ior tests (default both are run)
    -v              verbose output
    --hostfile      hostfile to give to MPI (mutually exclusive with --host)
    --host          comma separated list of hosts to give to MPI (mutually exclusive with --hostfile)
    -n              mdtest+ior: number of processes used with mpiexec ${HOSTS} --map-by node (defaults to 1)
    -i              mdtest+iornumber of iterations in each experiment (defaults to 3)
    -I              mdtest: number of items created per process (defaults to 1000)
    -e              mdtest: amount of data to read and write in bytes (defaults to 4 KiB)
    -N              mdtest: number of stride (defaults to 2)
    -b              ior: amount of data to read and write in total per process (defaults to 64m) (k,m,g allowed as unit)
    -c              ior: amount of data to read and write per I/O operation equal to chunk size (defaults to 512k)
                    this is only set for tests that equal to chunk size. Other tests are not affected by this param
    --ior_no_sequential  ior: disables sequential tests
    --ior_no_random      ior: disables random tests
    --ior_no_stride      ior: disables strided tests
    --ior_no_fpp         ior: disables file per process tests
    --ior_no_shared      ior: disables shared file tests
"
}

PROC=1
ITER=3
ITEMS=1000
MDTEST_IO=4096
MDTEST_STRIDE=2
RUN_MDTEST=true
RUN_IOR=true
PRELOAD=""
VERBOSE=false
IO_SIZE="64m"
CHUNKSIZE="512k"
IOR_NO_SEQUENTIAL=false
IOR_NO_RANDOM=false
IOR_NO_STRIDE=false
IOR_NO_FPP=false
IOR_NO_SHARED=false
HOSTS=""

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case ${key} in
    --preload)
    PRELOAD="$2"
    shift # past argument
    shift # past value
    ;;
    -n)
    PROC="$2"
    shift # past argument
    shift # past value
    ;;
	-i)
    ITER="$2"
    shift # past argument
    shift # past value
    ;;
	-I)
    ITEMS="$2"
    shift # past argument
    shift # past value
    ;;
    -e)
    MDTEST_IO="$2"
    shift # past argument
    shift # past value
    ;;
    -N)
    MDTEST_STRIDE="$2"
    shift # past argument
    shift # past value
    ;;
    -b)
    IO_SIZE="$2"
    shift # past argument
    shift # past value
    ;;
    -c)
    CHUNKSIZE="$2"
    shift # past argument
    shift # past value
    ;;
    --hostfile)
    HOSTS="--hostfile $2"
    shift # past argument
    shift # past value
    ;;
    --host)
    HOSTS="--host $2"
    shift # past argument
    shift # past value
    ;;
    --mdtestonly)
    RUN_IOR=false
    shift # past argument
    ;;
    --ioronly)
    RUN_MDTEST=false
    shift # past argument
    ;;
    --ior_no_sequential)
    IOR_NO_SEQUENTIAL=true
    shift # past argument
    ;;
    --ior_no_random)
    IOR_NO_RANDOM=true
    shift # past argument
    ;;
    --ior_no_stride)
    IOR_NO_STRIDE=true
    shift # past argument
    ;;
    --ior_no_fpp)
    IOR_NO_FPP=true
    shift # past argument
    ;;
    --ior_no_shared)
    IOR_NO_SHARED=true
    shift # past argument
    ;;
    -v)
    VERBOSE=true
    shift # past argument
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
if [[ ( -z ${1+x} ) || ( -z ${2+x} ) || ( -z ${3+x} ) ]]; then
    echo "Positional arguments missing."
    usage_short
    exit
fi


######### From now on exits on any error ########
set -e
MDTEST_PATH="$( readlink -mn "${1}" )"
IOR_PATH="$( readlink -mn "${2}" )"
TEST_PATH="${3}"
# just check that paths exist
if [ ! -f ${1} ]; then
    echo "Mdtest path ${1} not found. exit."
    exit
fi
if [ ! -f ${2} ]; then
    echo "ior path ${2} not found. exit."
    exit
fi
if [ ! -d ${3} ]; then
    echo "test path ${3} does not exist. exit."
    exit
fi

if [ "${VERBOSE}" == true ]; then
echo "Following params are set:"
echo "--POSITIONAL params:"
echo "MDTEST_PATH: ${MDTEST_PATH}"
echo "IOR_PATH: ${IOR_PATH}"
echo "TEST_PATH: ${TEST_PATH}"
echo "--OPTIONAL params:"
echo "PROC: ${PROC}"
echo "ITER: ${ITER}"
echo "ITEMS: ${ITEMS}"
echo "MDTEST_IO: ${MDTEST_IO}"
echo "MDTEST_STRIDE: ${MDTEST_STRIDE}"
echo "RUN_MDTEST: ${RUN_MDTEST}"
echo "RUN_IOR: ${RUN_IOR}"
echo "PRELOAD: ${PRELOAD}"
echo "IO_SIZE: ${IO_SIZE}"
echo "CHUNKSIZE: ${CHUNKSIZE}"
echo "IOR_NO_SEQUENTIAL: ${IOR_NO_SEQUENTIAL}"
echo "IOR_NO_RANDOM: ${IOR_NO_RANDOM}"
echo "IOR_NO_STRIDE: ${IOR_NO_STRIDE}"
echo "IOR_NO_FPP: ${IOR_NO_FPP}"
echo "IOR_NO_SHARED: ${IOR_NO_SHARED}"
echo "HOSTS: ${HOSTS}"

fi #VERBOSE

# EXEC
MDTEST_COUNT=0
IOR_COUNT=0

if [ "${RUN_MDTEST}" == true ]; then

echo "Running MDTest: 6 experiments ..."
echo "#################################################################################"
echo "## 1. Files in single directory ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${MDTEST_PATH} -z 0 -b 1 -d ${TEST_PATH} -i ${ITER} -I ${ITEMS} -F
MDTEST_COUNT=$((MDTEST_COUNT+1))
echo "#################################################################################"
echo "## 2. Files in isolated process directory ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${MDTEST_PATH} -z 0 -b 1 -d ${TEST_PATH} -i ${ITER} -I ${ITEMS} -F -u
MDTEST_COUNT=$((MDTEST_COUNT+1))
echo "#################################################################################"
echo "## 3. Files and directories in single directory ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${MDTEST_PATH} -z 0 -b 1 -d ${TEST_PATH} -i ${ITER} -I ${ITEMS}
MDTEST_COUNT=$((MDTEST_COUNT+1))
echo "#################################################################################"
echo "## 4. Files and directories in isolated process directory ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${MDTEST_PATH} -z 0 -b 1 -d ${TEST_PATH} -i ${ITER} -I ${ITEMS} -u
MDTEST_COUNT=$((MDTEST_COUNT+1))
echo "#################################################################################"
echo "## 5. Files in single directory and write and read ${MDTEST_IO} bytes ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${MDTEST_PATH} -z 0 -b 1 -d ${TEST_PATH} -i ${ITER} -I ${ITEMS} -F -w=${MDTEST_IO} -e=${MDTEST_IO}
MDTEST_COUNT=$((MDTEST_COUNT+1))
echo "#################################################################################"
echo "## 6. Files in single directory and write and read ${MDTEST_IO} bytes ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${MDTEST_PATH} -z 0 -b 1 -d ${TEST_PATH} -i ${ITER} -I ${ITEMS} -F -w=${MDTEST_IO} -e=${MDTEST_IO} -N ${MDTEST_STRIDE}
MDTEST_COUNT=$((MDTEST_COUNT+1))
echo "#################################################################################"
echo "## MDTest finished ##"
echo "#################################################################################"
echo
echo
fi # end RUN_MDTEST

if [ "${RUN_IOR}" == true ]; then
echo "Running IOR: 40 experiments ..."
if [ "${IOR_NO_FPP}" == false ]; then
# File per process
echo "#################################################################################"
echo "## 1 - 20 file per process: 1 - 5 Sequential, 6 - 10 Random, 11 - 15 Sequential strided, 16 - 20 Random strided ##"
echo "#################################################################################"
if [ "${IOR_NO_SEQUENTIAL}" == false ]; then
# Sequential
echo "#################################################################################"
echo "## 1. FPP: Sequential I/O transfer size == chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t ${CHUNKSIZE} -x -w -r -F
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 2. FPP: Sequential I/O transfer size (4m) > chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 4m -x -w -r -F
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 3. FPP: Sequential I/O transfer size (128k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 128k -x -w -r -F
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 4. FPP: Sequential I/O transfer size (4k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 4k -x -w -r -F
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 5. FPP: Sequential I/O transfer size (1k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 1k -x -w -r -F
IOR_COUNT=$((IOR_COUNT+1))
fi # no sequential
if [ "${IOR_NO_RANDOM}" == false ]; then
# Random
echo "#################################################################################"
echo "## 6. FPP: Random I/O transfer size == chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t ${CHUNKSIZE} -x -w -r -F -z
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 7. FPP: Random I/O transfer size (4m) > chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 4m -x -w -r -F -z
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 8. FPP: Random I/O transfer size (128k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 128k -x -w -r -F -z
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 9. FPP: Random I/O transfer size (4k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 4k -x -w -r -F -z
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 10. FPP: Random I/O transfer size (1k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 1k -x -w -r -F -z
IOR_COUNT=$((IOR_COUNT+1))
fi # no random
if [ "${IOR_NO_STRIDE}" == false ]; then
if [ "${IOR_NO_SEQUENTIAL}" == false ]; then
# Sequential strided
echo "#################################################################################"
echo "## 11. FPP: Sequential strided I/O transfer size == chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t ${CHUNKSIZE} -x -w -r -F -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 12. FPP: Sequential strided I/O transfer size (4m) > chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 4m -x -w -r -F -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 13. FPP: Sequential strided I/O transfer size (128k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 128k -x -w -r -F -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 14. FPP: Sequential strided I/O transfer size (4k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 4k -x -w -r -F -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 15. FPP: Sequential strided I/O transfer size (1k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 1k -x -w -r -F -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
fi # no sequential
if [ "${IOR_NO_RANDOM}" == false ]; then
# Random strided
echo "#################################################################################"
echo "## 16. FPP: Random strided I/O transfer size == chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t ${CHUNKSIZE} -x -w -r -F -z -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 17. FPP: Random strided I/O transfer size (4m) > chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 4m -x -w -r -F -z -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 18. FPP: Random strided I/O transfer size (128k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 128k -x -w -r -F -z -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 19. FPP: Random strided I/O transfer size (4k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 4k -x -w -r -F -z -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 20. FPP: Random strided I/O transfer size (1k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 1k -x -w -r -F -z -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
fi # no random
fi # no stride
fi # no fpp
if [ "${IOR_NO_SHARED}" == false ]; then
# Shared file
echo "#################################################################################"
echo "## 21 - 35 Shared file: 21 - 25 Sequential, 26 - 30 Random, 31 - 35 Sequential strided ##"
echo "#################################################################################"
if [ "${IOR_NO_SEQUENTIAL}" == false ]; then
# Sequential
echo "#################################################################################"
echo "## 21. Shared File: Sequential I/O transfer size == chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t ${CHUNKSIZE} -x -w -r
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 22. Shared File: Sequential I/O transfer size (4m) > chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 4m -x -w -r
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 23. Shared File: Sequential I/O transfer size (128k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 128k -x -w -r
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 24. Shared File: Sequential I/O transfer size (4k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 4k -x -w -r
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 25. Shared File: Sequential I/O transfer size (1k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 1k -x -w -r
IOR_COUNT=$((IOR_COUNT+1))
fi # no sequential
if [ "${IOR_NO_RANDOM}" == false ]; then
# Random
echo "#################################################################################"
echo "## 26. Shared File: Random I/O transfer size == chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t ${CHUNKSIZE} -x -w -r -z
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 27. Shared File: Random I/O transfer size (4m) > chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 4m -x -w -r -z
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 28. Shared File: Random I/O transfer size (128k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 128k -x -w -r -z
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 29. Shared File: Random I/O transfer size (4k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 4k -x -w -r -z
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 30. Shared File: Random I/O transfer size (1k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 1k -x -w -r -z
IOR_COUNT=$((IOR_COUNT+1))
fi # no random
if [ "${IOR_NO_STRIDE}" == false ]; then
if [ "${IOR_NO_SEQUENTIAL}" == false ]; then
# Sequential strided
echo "#################################################################################"
echo "## 31. Shared File: Sequential strided I/O transfer size == chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t ${CHUNKSIZE} -x -w -r -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 32. Shared File: Sequential strided I/O transfer size (4m) > chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 4m -x -w -r -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 33. Shared File: Sequential strided I/O transfer size (128k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b ${IO_SIZE} -t 128k -x -w -r -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 34. Shared File: Sequential strided I/O transfer size (4k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 4k -x -w -r -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
echo "#################################################################################"
echo "## 35. Shared File: Sequential strided I/O transfer size (1k) < chunksize ##"
echo "#################################################################################"
mpiexec ${HOSTS} --map-by node -n ${PROC} -x LD_PRELOAD=${PRELOAD} ${IOR_PATH} -a POSIX -i ${ITER} -o ${TEST_PATH}/iortest -b 4m -t 1k -x -w -r -Z -X 42
IOR_COUNT=$((IOR_COUNT+1))
# shared file Random strided doesnt exist in ior
fi # no sequential
fi # no stride
fi # no shared
fi # end RUN_IOR
echo
echo "mdtest: ${MDTEST_COUNT} tests successfully executed"
echo "ior: ${IOR_COUNT} tests successfully executed"
echo
echo 'Nothing left to do; exiting. :)'
