# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(matrix_transposer)

# Enable C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the build type to Debug by default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# -------------------------------------------------------
# Libraries
# -------------------------------------------------------
add_library(ipc_signal SHARED lib/futex/Futex.cpp)
add_library(ipc_shared_mem SHARED lib/shared-mem/SharedMatrixBuffer.cpp)
add_library(ipc_unix_sock INTERFACE)

target_include_directories(ipc_signal PUBLIC lib/futex)
target_include_directories(ipc_shared_mem PUBLIC lib/shared-mem)
target_include_directories(ipc_unix_sock INTERFACE lib/unix-socks)

# -------------------------------------------------------
# Executables
# -------------------------------------------------------

set(SERVER_SOURCES
    transposer_demo/transpose_server/transpose_server.cpp
    transposer_demo/transpose_server/ServerWorkspace.h
)

set(CLIENT_SOURCES
    transposer_demo/transpose_client/transpose_client.cpp
    transposer_demo/transpose_client/ClientWorkspace.h
)

add_executable(transpose_server ${SERVER_SOURCES})
add_executable(transpose_client ${CLIENT_SOURCES})

target_include_directories(transpose_server PUBLIC lib transposer_demo/common)
target_include_directories(transpose_client PUBLIC lib transposer_demo/common)

target_link_libraries(transpose_server PRIVATE ipc_signal ipc_shared_mem ipc_unix_sock)
target_link_libraries(transpose_client PRIVATE ipc_signal ipc_shared_mem ipc_unix_sock)

# Add debug information flags
target_compile_options(transpose_server PRIVATE $<$<CONFIG:Debug>:-g>)
target_compile_options(transpose_client PRIVATE $<$<CONFIG:Debug>:-g>)

# -------------------------------------------------------
# Add GoogleTest for testing
# -------------------------------------------------------

# enable_testing()

# # Add test source files
# set(TEST_SOURCES
#     tests/test_Futex.cpp
#     # tests/test_SharedMatrixBuffer.cpp
#     # tests/test_UnixSockets.cpp
# )

# add_executable(run_tests ${TEST_SOURCES})

# target_include_directories(run_tests PRIVATE include)
# target_link_libraries(run_tests PRIVATE gtest gtest_main ipc_signal ipc_shared_mem)

# # Automatically discover tests
# include(GoogleTest)
# gtest_discover_tests(run_tests)
