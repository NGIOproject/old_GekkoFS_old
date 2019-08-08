set(PRELOAD_SRC
    preload_context.cpp
    adafs_functions.cpp
    intercept.cpp
    hooks.cpp
    open_file_map.cpp
    open_dir.cpp
    preload.cpp
    resolve.cpp
    preload_util.cpp
    rpc/ld_rpc_management.cpp
    rpc/ld_rpc_data_ws.cpp
    rpc/ld_rpc_metadentry.cpp
    ../global/rpc/rpc_utils.cpp
    ../global/path_util.cpp
    )
set(PRELOAD_HEADERS
    ../../include/global/configure.hpp
    ../../include/global/global_defs.hpp
    ../../include/global/rpc/rpc_types.hpp
    ../../include/global/rpc/rpc_utils.hpp
    ../../include/global/path_util.hpp
    ../../include/global/chunk_calc_util.hpp
    ../../include/client/preload_context.hpp
    ../../include/client/adafs_functions.hpp
    ../../include/client/intercept.hpp
    ../../include/client/syscall_names.hpp
    ../../include/client/hooks.hpp
    ../../include/client/open_file_map.hpp
    ../../include/client/open_dir.hpp
    ../../include/client/preload.hpp
    ../../include/client/resolve.hpp
    ../../include/client/preload_util.hpp
    ../../include/client/rpc/ld_rpc_management.hpp
    ../../include/client/rpc/ld_rpc_data_ws.hpp
    ../../include/client/rpc/ld_rpc_metadentry.hpp
    )

add_library(gkfs_intercept SHARED ${PRELOAD_SRC} ${PRELOAD_HEADERS})

target_link_libraries(gkfs_intercept
    # internal
    metadata
    distributor
    log_util
    env_util
    # external
    Syscall_intercept::Syscall_intercept
    dl
    ${ABT_LIBRARIES}
    mercury
    ${MARGO_LIBRARIES}
    Boost::boost # needed for tokenizer header
    Threads::Threads
)

target_include_directories(gkfs_intercept
    PRIVATE
    ${ABT_INCLUDE_DIRS}
    ${MARGO_INCLUDE_DIRS}
    )

install(TARGETS gkfs_intercept
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gkfs
)

