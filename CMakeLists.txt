cmake_minimum_required(VERSION 3.6)

project(
    GekkoFS
    VERSION 0.6.0
)


if(NOT CMAKE_COMPILER_IS_GNUCC)
    message(FATAL_ERROR "The choosen C compiler is not gcc and is not supported")
endif()
if(NOT CMAKE_COMPILER_IS_GNUCXX)
    message(FATAL_ERROR "The choosen C++ compiler is not g++ and is not supported")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release
            CACHE STRING "Choose the type of build: Debug Release Memcheck" FORCE)
ENDIF (NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Compiler flags for various cmake build types
set(WARNINGS_FLAGS "-Wall -Wextra --pedantic -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${WARNINGS_FLAGS} -g -O0")
set(CMAKE_CXX_FLAGS_MEMCHECK "${WARNINGS_FLAGS} -g -O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_MAINTAINER "${WARNINGS_FLAGS} -g -O0 -pg -no-pie")
mark_as_advanced(CMAKE_CXX_FLAGS_MAINTAINER)

# Project version
set(GIT_VERSION_FOUND FALSE)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
    find_package(Git)
    if(Git_FOUND)
        execute_process(COMMAND /bin/bash -c "${GIT_EXECUTABLE} describe --tags --match='v*.*.*' --long --dirty"
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_RET
                        OUTPUT_VARIABLE GIT_VERSION
                        ERROR_VARIABLE GIT_ERR
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        )
        if("${GIT_RET}" STREQUAL "0")
            string(REGEX MATCH
                   "^v([0-9]+)\.([0-9]+)\.([0-9]+)-([0-9]+)-(.*)$"
                   DROP ${GIT_VERSION})
            set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
            set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
            set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})
            set(GKFS_COMMIT_GAP ${CMAKE_MATCH_4})
            set(GKFS_COMMIT_HASH ${CMAKE_MATCH_5})
            set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-snapshot+${GKFS_COMMIT_GAP}-${GKFS_COMMIT_HASH}")
            set(GIT_VERSION_FOUND TRUE)
        else()
            message(WARNING "Failed to run `git describe`: [${GIT_RET}] '${GIT_ERR}'")
        endif()
    endif()
endif()
message(STATUS "Project version: ${PROJECT_VERSION}")
set(GKFS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(GKFS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(GKFS_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(GKFS_VERSION_STRING ${PROJECT_VERSION})
configure_file(include/version.hpp.in include/version.hpp)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

set(CMAKE_EXPORT_COMPILE_COMMANDS 0)

# Rocksdb dependencies
find_package(LZ4 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Snappy REQUIRED)
find_package(ZStd REQUIRED)
find_package(JeMalloc) # required if rocksdb has been build with jemalloc
find_package(RocksDB REQUIRED)
# margo dependencies
find_package(Mercury REQUIRED)
find_package(Abt REQUIRED)
find_package(Margo REQUIRED)
find_package(Syscall_intercept REQUIRED)

# boost dependencies, system is required for filesystem
find_package(Boost 1.53 REQUIRED
    COMPONENTS
    filesystem
    program_options
    )

find_package(Threads REQUIRED)

set(RPC_PROTOCOL "ofi+tcp" CACHE STRING "Communication plugin used for RPCs")
set_property(CACHE RPC_PROTOCOL PROPERTY STRINGS
   "bmi+tcp"
   "ofi+tcp"
   "ofi+verbs"
   "ofi+psm2"
)
message(STATUS "RPC protocol: '${RPC_PROTOCOL}'")

option(USE_SHM "Use shared memory for intra-node communication" ON)
message(STATUS "Shared-memory communication: ${USE_SHM}")

option(SYMLINK_SUPPORT "Compile with support for symlinks" ON)
if(SYMLINK_SUPPORT)
    add_definitions(-DHAS_SYMLINKS)
endif()
message(STATUS "Symlink support: ${SYMLINK_SUPPORT}")

configure_file(include/global/configure.hpp.in include/global/configure.hpp)

# Imported target
add_library(RocksDB INTERFACE IMPORTED GLOBAL)
target_link_libraries(RocksDB
    INTERFACE
    ${ROCKSDB_LIBRARIES}
    # rocksdb libs
    ${Snappy_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${BZIP2_LIBRARIES}
    ${ZSTD_LIBRARIES}
    ${LZ4_LIBRARIES}
)

if(${JeMalloc_FOUND})
    target_link_libraries(RocksDB
        INTERFACE
        ${JEMALLOC_LIBRARIES}
    )
endif()
# we cannot use target_include_directories with CMake < 3.11
set_target_properties(RocksDB
    PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${ROCKSDB_INCLUDE_DIRS}
)

add_library(spdlog INTERFACE)
# we cannot use target_include_directories with CMake < 3.11
set_target_properties(spdlog
    PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external"
)

add_library(fmt INTERFACE)
# we cannot use target_include_directories with CMake < 3.11
set_target_properties(fmt
    PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/spdlog"
)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# define include directories that are relevant for all targets
include_directories(
    ${INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

include(GNUInstallDirs)

# Global components
add_subdirectory(src/global)
# Daemon
add_subdirectory(src/daemon)
# Client library
add_subdirectory(src/client)
