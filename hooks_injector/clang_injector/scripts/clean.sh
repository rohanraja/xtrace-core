#!/bin/bash

# Clean build artifacts

set -e

echo "Cleaning build artifacts..."

# Remove build directory
if [ -d "build" ]; then
    rm -rf build
    echo "✓ Removed build directory"
else
    echo "No build directory found"
fi

# Remove any temporary files
find . -name "*.tmp" -delete 2>/dev/null || true
find . -name "*.log" -delete 2>/dev/null || true

echo "Clean complete!"
