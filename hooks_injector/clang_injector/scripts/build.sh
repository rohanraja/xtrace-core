#!/bin/bash

# Build the project

set -e

echo "Building xTrace Clang Injector..."

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Build directory not found. Running configure first..."
    npm run configure
fi

cd build

# Build the project
echo "Compiling..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Check if build was successful
if [ -f "xtrace-clang-injector" ]; then
    echo "✓ Build successful!"
    echo "Executable: $(pwd)/xtrace-clang-injector"
    
    # Show basic info about the binary
    echo ""
    echo "Binary info:"
    ls -lh xtrace-clang-injector
    echo ""
    echo "Usage:"
    echo "  ./xtrace-clang-injector <input.cpp> -- [clang-args]"
    echo "  echo 'int main() { return 0; }' | ./xtrace-clang-injector /dev/stdin --"
else
    echo "✗ Build failed!"
    exit 1
fi
