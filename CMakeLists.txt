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
add_library(futex SHARED lib/futex/FutexSignaller.cpp)
add_library(matrix-buf SHARED lib/matrix-buf/SharedMatrixBuffer.cpp)
add_library(presentation INTERFACE)
add_library(shared-mem SHARED lib/shared-mem/SharedMemory.cpp)
add_library(mem-utils INTERFACE)
add_library(spsc-queue SHARED lib/spsc-queue/SpscQueue.cpp)
add_library(unix-socks INTERFACE)

target_include_directories(futex PUBLIC lib/futex lib/mem-utils lib/shared-mem)
target_include_directories(matrix-buf PUBLIC lib/shared-mem)
target_include_directories(presentation INTERFACE lib/presentation)
target_include_directories(shared-mem PUBLIC lib/shared-mem)
target_include_directories(spsc-queue PUBLIC lib/mem-utils lib/shared-mem lib/futex)
target_include_directories(unix-socks INTERFACE lib/unix-socks)

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

target_link_libraries(transpose_server PRIVATE futex matrix-buf presentation mem-utils shared-mem unix-socks)
target_link_libraries(transpose_client PRIVATE futex matrix-buf presentation mem-utils shared-mem unix-socks)

# Add debug information flags
target_compile_options(transpose_server PRIVATE $<$<CONFIG:Debug>:-g>)
target_compile_options(transpose_client PRIVATE $<$<CONFIG:Debug>:-g>)

# -------------------------------------------------------
# Add GoogleTest for testing
# -------------------------------------------------------

enable_testing()

# Add test source files
set(TEST_SOURCES
    tests/test_FutexSignaller.cpp
    tests/test_SharedMatrixBuffer.cpp
    # tests/test_UnixSockets.cpp
)

add_executable(run_tests ${TEST_SOURCES})

target_include_directories(run_tests PUBLIC lib)
target_link_libraries(run_tests PRIVATE gtest gtest_main futex matrix-buf shared-mem unix-socks)

# Automatically discover tests
include(GoogleTest)
gtest_discover_tests(run_tests)
