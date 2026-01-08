#!/bin/bash

# Quick build script for DiskNuke

echo "Building DiskNuke..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Run CMake
cmake ..

# Compile
make -j$(nproc)

echo ""
echo "Build complete!"
echo "Run './disknuke -h' for usage information"
