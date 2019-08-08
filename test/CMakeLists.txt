cmake_minimum_required(VERSION 3.6)
project(GekkoFS_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)


set(SOURCE_FILES main.cpp)
add_executable(gkfs_test ${SOURCE_FILES})

set(SOURCE_FILES_IO main_IO_testing.cpp)
add_executable(gkfs_test_IO ${SOURCE_FILES_IO})

set(SOURCE_FILES_TEMP main_temp.cpp)
add_executable(gkfs_test_temp ${SOURCE_FILES_TEMP})


add_executable(gkfs_test_wr wr_test.cpp)

add_executable(gkfs_test_dir dir_test.cpp)

add_executable(gkfs_test_truncate truncate.cpp)

add_executable(gkfs_test_lseek lseek.cpp)
add_executable(gkfs_test_symlink symlink_test.cpp)

add_executable(gkfs_test_path_resolution path_resolution.cpp)

find_package(MPI)
if(${MPI_FOUND})
    set(SOURCE_FILES_MPI main_MPI.cpp)
    add_executable(gkfs_test_MPI ${SOURCE_FILES_MPI})
    if(TARGET MPI::MPI_CXX)
        # should be defined for CMAKE > 3.9
        target_link_libraries(gkfs_test_MPI MPI::MPI_CXX ${MPI_LIBRARIES})
    else()
        target_link_libraries(gkfs_test_MPI ${MPI_CXX_LIBRARIES})
        target_include_directories(gkfs_test_MPI PUBLIC ${MPI_CXX_INCLUDE_PATH})
    endif()
endif()