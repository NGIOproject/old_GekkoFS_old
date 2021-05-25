add_subdirectory(backend/metadata)
add_subdirectory(backend/data)

set(DAEMON_SRC
    ../global/rpc/rpc_util.cpp
    ../global/path_util.cpp
    daemon.cpp
    util.cpp
    ops/metadentry.cpp
    ops/data.cpp
    classes/fs_data.cpp
    classes/rpc_data.cpp
    handler/srv_data.cpp
    handler/srv_metadata.cpp
    handler/srv_management.cpp
    )
set(DAEMON_HEADERS
    ../../include/config.hpp
    ../../include/version.hpp
    ../../include/global/cmake_configure.hpp
    ../../include/global/global_defs.hpp
    ../../include/global/rpc/rpc_types.hpp
    ../../include/global/rpc/rpc_util.hpp
    ../../include/global/path_util.hpp
    ../../include/daemon/daemon.hpp
    ../../include/daemon/util.hpp
    ../../include/daemon/ops/data.hpp
    ../../include/daemon/ops/metadentry.hpp
    ../../include/daemon/classes/fs_data.hpp
    ../../include/daemon/classes/rpc_data.hpp
    ../../include/daemon/handler/rpc_defs.hpp
    ../../include/daemon/handler/rpc_util.hpp
    )
set(DAEMON_LINK_LIBRARIES
    # internal libs
    metadata
    metadata_db
    storage
    distributor
    log_util
    env_util
    spdlog
    fmt::fmt
    # margo libs
    ${ABT_LIBRARIES}
    mercury
    ${MARGO_LIBRARIES}
    # others
    Boost::boost
    Boost::program_options
    Boost::filesystem
    Threads::Threads
    )
set(DAEMON_INCLUDE_DIRS
    ${ABT_INCLUDE_DIRS}
    ${MARGO_INCLUDE_DIRS}
    )

add_executable(gkfs_daemon ${DAEMON_SRC} ${DAEMON_HEADERS})
target_link_libraries(gkfs_daemon ${DAEMON_LINK_LIBRARIES})

target_include_directories(gkfs_daemon PRIVATE ${DAEMON_INCLUDE_DIRS})

install(TARGETS gkfs_daemon
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
################### Forwarding daemon ###################
if (GKFS_ENABLE_FORWARDING)
    set(FWD_DAEMON_SRC
        ${DAEMON_SRC}
        scheduler/agios.cpp
        )
    set(FWD_DAEMON_HEADERS
        ${DAEMON_HEADERS}
        ../../include/daemon/scheduler/agios.hpp
        )
    add_executable(gkfwd_daemon ${FWD_DAEMON_SRC} ${FWD_DAEMON_HEADERS})

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

    target_link_libraries(gkfwd_daemon
        ${DAEMON_LINK_LIBRARIES}
        ${AGIOS_LIBRARIES}
        )

    target_include_directories(gkfwd_daemon
        PRIVATE
        ${DAEMON_INCLUDE_DIRS}
        ${AGIOS_INCLUDE_DIRS}
        )

    install(TARGETS gkfwd_daemon
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
endif ()
