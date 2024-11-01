#!/bin/bash

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
  echo "CMake is not installed. Please install CMake and try again."
  exit 1
fi

# Navigate to the project directory (modify if script is placed outside)
PROJECT_DIR=$(dirname "$0")
cd "$PROJECT_DIR"

# Step 1: Create and enter the build directory
rm -rf build
mkdir -p build
cd build

# Step 2: Run CMake to configure the project and build the binaries
echo "Configuring and building the project..."
cmake .. && make

# Check if build was successful
if [[ $? -ne 0 ]]; then
  echo "Build failed. Exiting."
  exit 1
fi

# # Run the server in the background
# echo "Starting the server..."
# ./transpose_server &
# SERVER_PID=$!

# # Allow the server a moment to start up (adjust if necessary)
# sleep 1

# # Run the client
# echo "Running the client..."
# ./transpose_client

