cmake_minimum_required(VERSION 3.2)

# Set the project name
project(matrix_transposer)

# Enable C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)

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
add_library(stats INTERFACE)
add_library(mat-transpose SHARED
    lib/mat-transpose/TransposeNaive.cpp
    lib/mat-transpose/TransposeTiledMultiThreaded.cpp
    lib/mat-transpose/MatricesAreEqual.cpp
)

target_include_directories(futex PUBLIC lib/futex lib/mem-utils lib/shared-mem)
target_include_directories(matrix-buf PUBLIC lib/shared-mem)
target_include_directories(presentation INTERFACE lib/presentation)
target_include_directories(mem-utils INTERFACE lib/mem-utils)
target_include_directories(stats INTERFACE lib/stats)
target_include_directories(shared-mem PUBLIC lib/shared-mem)
target_include_directories(unix-socks INTERFACE lib/unix-socks)
target_include_directories(spsc-queue PUBLIC lib/mem-utils lib/shared-mem lib/futex)
target_include_directories(mat-transpose PUBLIC lib/mem-utils lib/shared-mem lib/futex)

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

target_link_libraries(transpose_server PRIVATE futex matrix-buf presentation mem-utils shared-mem unix-socks spsc-queue mat-transpose stats)
target_link_libraries(transpose_client PRIVATE futex matrix-buf presentation mem-utils shared-mem unix-socks spsc-queue mat-transpose stats)

# Add debug information flags for Debug builds
target_compile_options(transpose_server PRIVATE $<$<CONFIG:Debug>:-g>)
target_compile_options(transpose_client PRIVATE $<$<CONFIG:Debug>:-g>)

# -------------------------------------------------------
# Add GoogleTest for testing
# -------------------------------------------------------

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Add test source files
set(TEST_SOURCES
    tests/test_FutexSignaller.cpp
    tests/test_SharedMatrixBuffer.cpp
    tests/test_SpscQueue.cpp
    tests/test_MemoryUtils.cpp
    tests/test_mat-transpose.cpp
)

add_executable(run_tests ${TEST_SOURCES})

target_include_directories(run_tests PUBLIC lib)
target_link_libraries(run_tests PRIVATE gtest gtest_main futex matrix-buf shared-mem unix-socks spsc-queue mem-utils mat-transpose)

# Automatically discover tests
include(GoogleTest)
gtest_discover_tests(run_tests)

# -------------------------------------------------------
# Add Google Benchmark for benchmarking
# -------------------------------------------------------

set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)

set(BENCHMARK_SOURCES
    benchmarks/bechmark_main.cpp
    # benchmarks/bechmark_SpscQueueSingleThreaded.cpp
    # benchmarks/bechmark_SpscQueueMultiThreaded.cpp
    benchmarks/bechmark_SpscQueueMultiProcess.cpp
    # benchmarks/benchmark_mat-transpose_TransposeTiledMultiThreaded.cpp
)

add_executable(run_benchmarks ${BENCHMARK_SOURCES})

target_include_directories(run_benchmarks PUBLIC lib)
target_link_libraries(run_benchmarks PRIVATE benchmark::benchmark futex matrix-buf shared-mem unix-socks spsc-queue mem-utils mat-transpose)

# Set maximum optimization for the benchmark build
target_compile_options(run_benchmarks PRIVATE $<$<CONFIG:Release>:-O3>)

# Fetch Google Benchmark library
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.0
)
FetchContent_MakeAvailable(benchmark)
