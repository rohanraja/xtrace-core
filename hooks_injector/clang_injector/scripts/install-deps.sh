#!/bin/bash

# Script to install Clang/LLVM dependencies
# This script handles installation on Ubuntu/Debian systems

set -e

echo "Installing Clang/LLVM dependencies..."

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Ubuntu/Debian
    if command -v apt-get &> /dev/null; then
        echo "Detected Ubuntu/Debian system"
        
        # Install LLVM/Clang development libraries
        sudo apt-get update
        sudo apt-get install -y \
            clang-15 \
            llvm-15-dev \
            libclang-15-dev \
            clang-tools-15 \
            libllvm15 \
            llvm-15-runtime \
            cmake \
            build-essential \
            pkg-config
        
        # Create symlinks if needed
        if ! command -v clang++ &> /dev/null; then
            sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-15 100
            sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-15 100
        fi
        
        if ! command -v llvm-config &> /dev/null; then
            sudo update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-15 100
        fi
        
    # Red Hat/CentOS/Fedora
    elif command -v yum &> /dev/null || command -v dnf &> /dev/null; then
        echo "Detected Red Hat/CentOS/Fedora system"
        
        if command -v dnf &> /dev/null; then
            PKG_MANAGER="dnf"
        else
            PKG_MANAGER="yum"
        fi
        
        sudo $PKG_MANAGER install -y \
            clang \
            llvm-devel \
            clang-devel \
            cmake \
            gcc-c++ \
            pkgconfig
    fi
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    echo "Detected macOS system"
    
    if command -v brew &> /dev/null; then
        brew install llvm cmake
        
        # Add LLVM to PATH
        export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
        echo 'export PATH="/opt/homebrew/opt/llvm/bin:$PATH"' >> ~/.zshrc
        
    else
        echo "Please install Homebrew first: https://brew.sh/"
        exit 1
    fi
    
else
    echo "Unsupported operating system: $OSTYPE"
    echo "Please install LLVM/Clang development libraries manually"
    exit 1
fi

# Verify installation
echo "Verifying installation..."

if command -v clang++ &> /dev/null; then
    echo "✓ clang++ found: $(clang++ --version | head -n 1)"
else
    echo "✗ clang++ not found"
    exit 1
fi

if command -v llvm-config &> /dev/null; then
    echo "✓ llvm-config found: LLVM version $(llvm-config --version)"
else
    echo "✗ llvm-config not found"
    exit 1
fi

if command -v cmake &> /dev/null; then
    echo "✓ cmake found: $(cmake --version | head -n 1)"
else
    echo "✗ cmake not found"
    exit 1
fi

echo "Dependencies installed successfully!"
echo ""
echo "Next steps:"
echo "  npm run configure  # Configure the build"
echo "  npm run build      # Build the project"
