cmake_minimum_required(VERSION 3.11)

project(gkfs.io
    VERSION 0.1
    LANGUAGES CXX
)

add_executable(gkfs.io
    gkfs.io/main.cpp
    gkfs.io/commands.hpp
    gkfs.io/mkdir.cpp
    gkfs.io/open.cpp
    gkfs.io/opendir.cpp
    gkfs.io/read.cpp
    gkfs.io/readv.cpp
    gkfs.io/pread.cpp
    gkfs.io/preadv.cpp
    gkfs.io/readdir.cpp
    gkfs.io/reflection.hpp
    gkfs.io/rmdir.cpp
    gkfs.io/serialize.hpp
    gkfs.io/stat.cpp
    gkfs.io/write.cpp
    gkfs.io/pwrite.cpp
    gkfs.io/writev.cpp
    gkfs.io/pwritev.cpp
    gkfs.io/statx.cpp
    gkfs.io/lseek.cpp
    gkfs.io/write_validate.cpp
    gkfs.io/write_random.cpp
    gkfs.io/truncate.cpp
    gkfs.io/util/file_compare.cpp
)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG dd0d8e4fe729e5b1110232c7a5c9566dad884686 # v1.9.0
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG e7b3b40b5a95bc74b9a7f662830a27c49ffc01b4 # v3.7.3
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)

FetchContent_GetProperties(cli11)

if(NOT cli11_POPULATED)
    FetchContent_Populate(cli11)
    message(STATUS "[gkfs.io] CLI11 source dir: ${cli11_SOURCE_DIR}")
    message(STATUS "[gkfs.io] CLI11 binary dir: ${cli11_BINARY_DIR}")
    add_subdirectory(${cli11_SOURCE_DIR} ${cli11_BINARY_DIR})
endif()

FetchContent_GetProperties(nlohmann_json)

if(NOT nlohmann_json_POPULATED)
    FetchContent_Populate(nlohmann_json)
    message(STATUS "[gkfs.io] Nlohmann JSON source dir: ${nlohmann_json_SOURCE_DIR}")
    message(STATUS "[gkfs.io] Nlohmann JSON binary dir: ${nlohmann_json_BINARY_DIR}")

    # we don't really care so much about a third party library's tests to be
    # run from our own project's code
    set(JSON_BuildTests OFF CACHE INTERNAL "")

    # we also don't need to install it when our main project gets installed
    set(JSON_Install OFF CACHE INTERNAL "")

    add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR})
endif()

target_include_directories(gkfs.io PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/gkfs.io>
)

target_link_libraries(gkfs.io
    CLI11::CLI11
    nlohmann_json::nlohmann_json
    fmt::fmt
)

if(GKFS_INSTALL_TESTS)
    install(TARGETS gkfs.io
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
