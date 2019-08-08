add_library(storage STATIC)

target_sources(storage
    PUBLIC
    ${INCLUDE_DIR}/daemon/backend/data/chunk_storage.hpp
    PRIVATE
    ${INCLUDE_DIR}/global/path_util.hpp
    ${CMAKE_CURRENT_LIST_DIR}/chunk_storage.cpp
    )

target_link_libraries(storage
    PRIVATE
    spdlog
    Boost::filesystem
    ${ABT_LIBRARIES}
    )

target_include_directories(storage
    PRIVATE
    ${ABT_INCLUDE_DIRS}
    )
