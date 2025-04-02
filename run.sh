#!/bin/bash
# This script is used to run the project

# Check if the executable file exists
if [ ! -f "./bin/fluid_sim" ]; then
    echo "Executable file not found!"
    exit 1
fi

# Run the executable file
./bin/fluid_sim
