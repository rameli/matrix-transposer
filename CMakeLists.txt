# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(matrix-transposer)

# Enable C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the build type to Debug by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Add debug information flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# -------------------------------------------------------
# Shared libraries
# -------------------------------------------------------
add_library(ipc SHARED
    src/ipc/Futex.cpp
    src/ipc/MPSCQueue.cpp
    src/ipc/SharedMatrixBuffer.cpp
    src/ipc/UnixSocketClient.cpp
    src/ipc/UnixSocketServer.cpp
)

target_include_directories(ipc PUBLIC include/ipc)

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
target_link_libraries(transpose_server PRIVATE ipc)

add_executable(transpose_client ${CLIENT_SOURCES})
target_link_libraries(transpose_client PRIVATE ipc)

# -------------------------------------------------------
# Add GoogleTest for testing
# -------------------------------------------------------

enable_testing()

# Add test source files
set(TEST_SOURCES
    tests/test_Futex.cpp
    tests/test_SharedMatrixBuffer.cpp
    tests/test_UnixSockets.cpp
)

add_executable(run_tests ${TEST_SOURCES})

target_include_directories(run_tests PRIVATE include)
target_link_libraries(run_tests PRIVATE gtest gtest_main ipc)

# Automatically discover tests
include(GoogleTest)
gtest_discover_tests(run_tests)
