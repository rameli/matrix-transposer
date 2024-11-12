#!/bin/bash

rm -rf /dev/shm/*

# Configurable range for each random number argument
MIN1=4
MAX1=14
MIN2=4
MAX2=14
MIN3=1
MAX3=20

# Configurable range for the number of repetitions
MIN_REPS=10
MAX_REPS=32

# Configurable range for the delay (in milliseconds)
MIN_DELAY_MS=100  # Minimum delay in milliseconds
MAX_DELAY_MS=3000 # Maximum delay in milliseconds

# Function to check if MAX >= MIN
validate_range() {
  local MIN=$1
  local MAX=$2
  if (( MAX < MIN )); then
    echo "Error: MAX ($MAX) cannot be less than MIN ($MIN)."
    exit 1
  fi
}

# Validate all ranges
validate_range $MIN1 $MAX1
validate_range $MIN2 $MAX2
validate_range $MIN3 $MAX3
validate_range $MIN_REPS $MAX_REPS
validate_range $MIN_DELAY_MS $MAX_DELAY_MS

# Function to generate a random number within a given range
generate_random() {
  local MIN=$1
  local MAX=$2
  echo $(( RANDOM % (MAX - MIN + 1) + MIN ))
}

# Generate a random number of repetitions within the specified range
REPETITIONS=$(generate_random $MIN_REPS $MAX_REPS)

# Loop the specified number of repetitions
for (( i=0; i<REPETITIONS; i++ )); do
  # Generate a random delay in milliseconds and convert to seconds for sleep
  DELAY_MS=$(generate_random $MIN_DELAY_MS $MAX_DELAY_MS)
  DELAY_SEC=$(echo "scale=3; $DELAY_MS / 1000" | bc)

  # Generate three random numbers within their respective ranges
  RANDOM1=$(generate_random $MIN1 $MAX1)
  RANDOM2=$(generate_random $MIN2 $MAX2)
  RANDOM3=$(generate_random $MIN3 $MAX3)

  # Start a background process that waits for the delay, then executes the command
  (
    sleep $DELAY_SEC
    ./transpose_client $RANDOM1 $RANDOM2 $RANDOM3
  ) &

done

# Wait for all background processes to complete
wait
