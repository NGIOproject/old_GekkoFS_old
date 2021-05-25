add_library(storage STATIC)

target_sources(storage
    PUBLIC
    ${INCLUDE_DIR}/daemon/backend/data/chunk_storage.hpp
    PRIVATE
    ${INCLUDE_DIR}/global/path_util.hpp
    ${INCLUDE_DIR}/global/global_defs.hpp
    ${INCLUDE_DIR}/daemon/backend/data/data_module.hpp
    ${INCLUDE_DIR}/daemon/backend/data/file_handle.hpp
    ${CMAKE_CURRENT_LIST_DIR}/chunk_storage.cpp
    ${CMAKE_CURRENT_LIST_DIR}/data_module.cpp
    )

target_link_libraries(storage
    PRIVATE
    spdlog
    Boost::filesystem
    -ldl
    )

#target_include_directories(storage
#    PRIVATE
#    )
