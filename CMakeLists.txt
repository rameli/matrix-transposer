# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(matrix-transposer)

# Enable C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Specify source files for each executable
set(SERVER_SOURCES
    src/transpose_server.cpp
    src/ipc_utils.cpp
    src/MatrixBuffer.cpp
    src/UnixSocketServer.cpp
)

set(CLIENT_SOURCES
    src/transpose_client.cpp
    src/ipc_utils.cpp
    src/MatrixBuffer.cpp
    src/UnixSocketClient.cpp
)

# Create the transpose_server executable
add_executable(transpose_server ${SERVER_SOURCES})

# Create the transpose_client executable
add_executable(transpose_client ${CLIENT_SOURCES})

# Add additional libraries or link dependencies if necessary
# For example:
# target_link_libraries(transpose_server <dependencies>)
# target_link_libraries(transpose_client <dependencies>)

# Include directories if your source files are organized in different folders
target_include_directories(transpose_server PRIVATE include)
target_include_directories(transpose_client PRIVATE include)

