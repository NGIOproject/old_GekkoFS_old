set(PRELOAD_SRC
    gkfs_functions.cpp
    hooks.cpp
    intercept.cpp
    logging.cpp
    open_file_map.cpp
    open_dir.cpp
    path.cpp
    preload.cpp
    preload_context.cpp
    preload_util.cpp
    ../global/path_util.cpp
    ../global/rpc/rpc_util.cpp
    rpc/rpc_types.cpp
    rpc/forward_data.cpp
    rpc/forward_management.cpp
    rpc/forward_metadata.cpp
    syscalls/detail/syscall_info.c
    )
set(PRELOAD_HEADERS
    ../../include/client/gkfs_functions.hpp
    ../../include/config.hpp
    ../../include/client/env.hpp
    ../../include/client/hooks.hpp
    ../../include/client/intercept.hpp
    ../../include/client/logging.hpp
    ../../include/client/make_array.hpp
    ../../include/client/open_file_map.hpp
    ../../include/client/open_dir.hpp
    ../../include/client/path.hpp
    ../../include/client/preload.hpp
    ../../include/client/preload_context.hpp
    ../../include/client/preload_util.hpp
    ../../include/client/rpc/rpc_types.hpp
    ../../include/client/rpc/forward_management.hpp
    ../../include/client/rpc/forward_metadata.hpp
    ../../include/client/rpc/forward_data.hpp
    ../../include/client/syscalls/args.hpp
    ../../include/client/syscalls/decoder.hpp
    ../../include/client/syscalls/errno.hpp
    ../../include/client/syscalls/rets.hpp
    ../../include/client/syscalls/syscall.hpp
    ../../include/client/syscalls/detail/syscall_info.h
    ../../include/global/cmake_configure.hpp
    ../../include/global/chunk_calc_util.hpp
    ../../include/global/global_defs.hpp
    ../../include/global/path_util.hpp
    ../../include/global/rpc/rpc_types.hpp
    ../../include/global/rpc/rpc_util.hpp
    )
set(PRELOAD_LINK_LIBRARIES
    # internal
    metadata
    distributor
    env_util
    # external
    Syscall_intercept::Syscall_intercept
    dl
    mercury
    hermes
    fmt::fmt
    Boost::boost
    Threads::Threads
    Date::TZ
    )
set(PRELOAD_INCLUDE_DIRS
    ${ABT_INCLUDE_DIRS}
    ${MARGO_INCLUDE_DIRS}
    )

add_library(gkfs_intercept SHARED ${PRELOAD_SRC} ${PRELOAD_HEADERS})

target_link_libraries(gkfs_intercept ${PRELOAD_LINK_LIBRARIES})

target_include_directories(gkfs_intercept PRIVATE ${PRELOAD_INCLUDE_DIRS})

install(TARGETS gkfs_intercept
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gkfs
    )
################### Forwarding client ###################
if (GKFS_ENABLE_FORWARDING)
    set(FWD_PRELOAD_SRC
        ${PRELOAD_SRC}
        )
    set(FWD_PRELOAD_HEADERS
        ${PRELOAD_HEADERS}
        )

    add_library(gkfwd_intercept SHARED ${FWD_PRELOAD_SRC} ${FWD_PRELOAD_HEADERS})

    if (GKFS_ENABLE_AGIOS)
        target_compile_definitions(gkfwd_daemon
            PUBLIC
            GKFS_ENABLE_FORWARDING
            DGKFS_ENABLE_AGIOS
            )
    else ()
        target_compile_definitions(gkfwd_daemon
            PUBLIC
            GKFS_ENABLE_FORWARDING
            )
    endif ()

    message(STATUS "[gekkofs] Forwarding mode: ${GKFS_ENABLE_FORWARDING}")
    message(STATUS "[gekkofs] AGIOS scheduling: ${GKFS_ENABLE_AGIOS}")

    target_link_libraries(gkfwd_intercept ${PRELOAD_LINK_LIBRARIES})

    target_include_directories(gkfwd_intercept PRIVATE ${PRELOAD_INCLUDE_DIRS})

    install(TARGETS gkfwd_intercept
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gkfs
        )
endif ()