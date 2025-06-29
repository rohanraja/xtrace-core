#!/bin/bash

# Configure the build using CMake

set -e

echo "Configuring xTrace Clang Injector build..."

# Create build directory
mkdir -p build
cd build

# Find LLVM installation
LLVM_CONFIG=$(which llvm-config 2>/dev/null || echo "")
if [ -z "$LLVM_CONFIG" ]; then
    # Try common locations
    for version in 15 14 13 12 11; do
        if command -v "llvm-config-$version" &> /dev/null; then
            LLVM_CONFIG="llvm-config-$version"
            break
        fi
    done
fi

if [ -z "$LLVM_CONFIG" ]; then
    echo "Error: llvm-config not found. Please install LLVM development libraries."
    echo "Run: npm run install-deps"
    exit 1
fi

echo "Using LLVM config: $LLVM_CONFIG"

# Get LLVM configuration
LLVM_VERSION=$($LLVM_CONFIG --version)
LLVM_PREFIX=$($LLVM_CONFIG --prefix)

echo "LLVM Version: $LLVM_VERSION"
echo "LLVM Prefix: $LLVM_PREFIX"

# Configure with CMake
CMAKE_ARGS=""

# Set LLVM paths if not automatically detected
if [ ! -z "$LLVM_PREFIX" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DLLVM_DIR=$LLVM_PREFIX/lib/cmake/llvm"
    CMAKE_ARGS="$CMAKE_ARGS -DClang_DIR=$LLVM_PREFIX/lib/cmake/clang"
fi

# Configure build type
CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release"

# Configure C++ standard
CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_STANDARD=17"

echo "CMake arguments: $CMAKE_ARGS"
echo "Configuring..."

cmake .. $CMAKE_ARGS

echo "Configuration complete!"
echo "Next step: npm run build"
