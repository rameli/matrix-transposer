# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(matrix-transposer)

# Enable C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# -------------------------------------------------------
# Shared libraries
# -------------------------------------------------------
add_library(IpcUtils SHARED
    src/ipc_utils.cpp
    src/UnixSocketServer.cpp
    src/UnixSocketClient.cpp
)

add_library(MatrixBuffer SHARED
    src/MatrixBuffer.cpp
)

target_include_directories(IpcUtils PUBLIC include)
target_include_directories(MatrixBuffer PUBLIC include)

# -------------------------------------------------------
# Executables
# -------------------------------------------------------

set(SERVER_SOURCES
    src/transpose_server.cpp
)

set(CLIENT_SOURCES
    src/transpose_client.cpp
)

add_executable(transpose_server ${SERVER_SOURCES})
target_link_libraries(transpose_server PRIVATE MatrixBuffer IpcUtils)

add_executable(transpose_client ${CLIENT_SOURCES})
target_link_libraries(transpose_client PRIVATE MatrixBuffer IpcUtils)

# -------------------------------------------------------
# Add GoogleTest for testing
# -------------------------------------------------------

enable_testing()

# Add test source files
set(TEST_SOURCES
    tests/test_MatrixBuffer.cpp
    tests/test_UnixSockets.cpp
)

add_executable(run_tests ${TEST_SOURCES})

target_include_directories(run_tests PRIVATE include)
target_link_libraries(run_tests PRIVATE gtest gtest_main MatrixBuffer IpcUtils)

# Automatically discover tests
include(GoogleTest)
gtest_discover_tests(run_tests)
