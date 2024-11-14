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
add_library(unix-socks INTERFACE)
add_library(spsc-queue SHARED lib/spsc-queue/SpscQueue.cpp)

target_include_directories(futex PUBLIC lib/futex lib/mem-utils lib/shared-mem)
target_include_directories(matrix-buf PUBLIC lib/shared-mem)
target_include_directories(presentation INTERFACE lib/presentation)
target_include_directories(mem-utils INTERFACE lib/mem-utils)
target_include_directories(shared-mem PUBLIC lib/shared-mem)
target_include_directories(unix-socks INTERFACE lib/unix-socks)
target_include_directories(spsc-queue PUBLIC lib/mem-utils lib/shared-mem lib/futex)

target_link_libraries(spsc-queue PRIVATE futex matrix-buf presentation mem-utils shared-mem unix-socks)

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

target_link_libraries(transpose_server PRIVATE futex matrix-buf presentation mem-utils shared-mem unix-socks spsc-queue)
target_link_libraries(transpose_client PRIVATE futex matrix-buf presentation mem-utils shared-mem unix-socks spsc-queue)

# Add debug information flags
target_compile_options(transpose_server PRIVATE $<$<CONFIG:Debug>:-g>)
target_compile_options(transpose_client PRIVATE $<$<CONFIG:Debug>:-g>)


# ################################################
set(A_SOURCES tests/a.cpp)
set(B_SOURCES tests/b.cpp)

add_executable(a ${A_SOURCES})
add_executable(b ${B_SOURCES})

target_include_directories(a PUBLIC lib)
target_include_directories(b PUBLIC lib)

target_link_libraries(a PUBLIC futex matrix-buf shared-mem unix-socks spsc-queue mem-utils)
target_link_libraries(b PUBLIC futex matrix-buf shared-mem unix-socks spsc-queue mem-utils)

# # # Add debug information flags
# # target_compile_options(a PRIVATE $<$<CONFIG:Debug>:-g>)
# # target_compile_options(b PRIVATE $<$<CONFIG:Debug>:-g>)

# ################################################

# -------------------------------------------------------
# Add GoogleTest for testing
# -------------------------------------------------------

enable_testing()

# Add test source files
set(TEST_SOURCES
    tests/test_FutexSignaller.cpp
    tests/test_SharedMatrixBuffer.cpp
    tests/test_SpscQueue.cpp
    tests/test_MemoryUtils.cpp
)

add_executable(run_tests ${TEST_SOURCES})

target_include_directories(run_tests PUBLIC lib)
target_link_libraries(run_tests PRIVATE gtest gtest_main futex matrix-buf shared-mem unix-socks spsc-queue mem-utils)

# Automatically discover tests
include(GoogleTest)
gtest_discover_tests(run_tests)
