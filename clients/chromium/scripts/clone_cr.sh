#!/bin/bash
# filepath: /Users/rohanraja/TestSyncVault/setup_chromium.sh

set -e  # Exit on any error
set -x  # Print commands being executed

# Setup directories
CHROMIUM_ROOT=" /workspace/cr4"

mkdir -p ${CHROMIUM_ROOT}

echo "Starting Chromium setup and build process..."

# Login to Microsoft services
echo "Authenticating with Microsoft services..."
echo "Please follow the instructions to authenticate:"
# es login -f device-code

# Clone first since it may prompt
cd ${CHROMIUM_ROOT}
git clone https://microsoft.visualstudio.com/DefaultCollection/Edge/_git/chromium.depot_tools.cr-contrib

# Install dependencies
echo "Installing dependencies..."
sudo apt install git -y


## Clone
cd $HOME
if [ ! -d "$HOME/depot_tools" ]; then
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
else
    rm -rf "$HOME/depot_tools" 
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
fi

export PATH="${HOME}/depot_tools:$PATH"

# Change to chromium directory
cd ${CHROMIUM_ROOT}

# Fetch chromium source code
echo "Fetching Chromium source code (this may take a while)..."
fetch --nohooks chromium

# Change to src directory
cd src

# Install build dependencies
echo "Installing build dependencies..."
./build/install-build-deps.sh

# Run hooks
echo "Running hooks..."
gclient runhooks

# Set up RBE (Remote Build Execution)
echo "Setting up RBE..."

# Update PATH for Microsoft depot_tools
export PATH="${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib:${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib/scripts:$PATH"

# Add the path to .bashrc for future sessions
echo "Adding Microsoft depot_tools to PATH in .bashrc..."
echo "export PATH=\"${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib:${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib/scripts:\$PATH\"" >> ${HOME}/.bashrc


# Sync dependencies
echo "Syncing dependencies..."
gclient sync

# Build Chromium
echo "Building Chromium..."
cd ${CHROMIUM_ROOT}/src
autogn x64 release
autoninja -C out/release_x64 chrome

echo "Chromium setup and build completed!"
echo "You can run Chromium with: ${CHROMIUM_ROOT}/src/out/release_x64/chrome"