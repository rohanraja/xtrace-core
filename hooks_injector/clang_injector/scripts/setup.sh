#!/bin/bash

# Setup script - installs dependencies and configures the project

set -e

echo "Setting up xTrace Clang Injector..."
echo "=================================="

# Install dependencies
echo "Step 1: Installing dependencies..."
npm run install-deps

echo ""
echo "Step 2: Configuring build..."
npm run configure

echo ""
echo "Step 3: Building project..."
npm run build

echo ""
echo "✓ Setup complete!"
echo ""
echo "The xTrace Clang Injector is now ready to use."
echo "Executable location: $(pwd)/build/xtrace-clang-injector"
echo ""
echo "Examples:"
echo "  # Process a C++ file"
echo "  ./build/xtrace-clang-injector example.cpp -- -std=c++17"
echo ""
echo "  # Process from stdin"
echo "  echo 'int main() { return 0; }' | ./build/xtrace-clang-injector /dev/stdin --"
