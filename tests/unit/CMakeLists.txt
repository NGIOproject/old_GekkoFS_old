include(FetchContent)

# get Catch2
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG 255aa5f2afe1a622c97422f65ace6ca915be0d8d # v2.11.3
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)

FetchContent_GetProperties(catch2)

if(NOT catch2_POPULATED)
    FetchContent_Populate(catch2)
    message(STATUS "[gkfs] Catch2 source dir: ${catch2_SOURCE_DIR}")
    message(STATUS "[gkfs] Catch2 binary dir: ${catch2_BINARY_DIR}")
    set(CATCH_BUILD_TESTING OFF CACHE INTERNAL "")
    add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR})
endif()

# create a convenience library with Catch2's main 
# to speed up test compilation
add_library(catch2_main
    STATIC catch_main.cpp
)

target_link_libraries(catch2_main
    Catch2::Catch2
)

# define executables for tests and make them depend on the convenience 
# library (and Catch2 transitively) and fmt
add_executable(tests
    test_example_00.cpp
    test_example_01.cpp
)

target_link_libraries(tests
    catch2_main
    fmt::fmt
)

# Catch2's contrib folder includes some helper functions
# to auto-discover Catch tests and register them in CTest
set(CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/contrib" ${CMAKE_MODULE_PATH})
include(Catch)
catch_discover_tests(tests)

if(GKFS_INSTALL_TESTS)
    install(TARGETS tests
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
