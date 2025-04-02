#!/bin/bash
# This script is used to build the project
# It will create a build directory, copy the necessary files, and run cmake and make commands

# Check if the CMakeLists.txt file exists
if [ ! -f "CMakeLists.txt" ]; then
    echo "CMakeLists.txt not found!"
    exit 1
fi

# Check if the build directory exists, if not, create it
if [ ! -d "build" ]; then
    mkdir build
fi
# Change to the build directory
cd build

# Run cmake command to generate the Makefile
cmake ..

# Run make command to build the project
make