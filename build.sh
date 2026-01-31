#!/bin/bash

echo "Building Console Game..."

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Run CMake
cmake -DCMAKE_BUILD_TYPE=Release ..
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build
cmake --build .
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo ""
echo "Build successful!"
echo "Executable: build/game"
echo ""
