add_library(distributor STATIC)
set_property(TARGET distributor PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(distributor
    PUBLIC
    ${INCLUDE_DIR}/global/rpc/distributor.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/rpc/distributor.cpp
    )

add_library(log_util STATIC)
set_property(TARGET log_util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(log_util
    PUBLIC
    ${INCLUDE_DIR}/global/log_util.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/log_util.cpp
    )
target_link_libraries(log_util
    spdlog
    )

add_library(env_util STATIC)
set_property(TARGET env_util PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(env_util
    PUBLIC
    ${INCLUDE_DIR}/global/env_util.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/env_util.cpp
    )

add_library(metadata STATIC)
set_property(TARGET metadata PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(metadata
    PUBLIC
    ${INCLUDE_DIR}/global/metadata.hpp
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/metadata.cpp
    )
target_link_libraries(metadata
    fmt::fmt
    )
