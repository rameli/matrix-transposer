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

# Step 3: Run the server in the background
echo "Starting the server..."
./transpose_server &

# Capture the server's process ID to manage it later
SERVER_PID=$!

# Allow the server a moment to start up (adjust if necessary)
sleep 2

# Step 4: Run the client
echo "Running the client..."
./transpose_client

# Step 5: Cleanup - Kill the server process after the client is done
echo "Stopping the server..."
kill $SERVER_PID

echo "Done."

