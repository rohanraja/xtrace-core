#!/bin/bash

set -e  # Exit on any error

# Setup directories
CHROMIUM_ROOT="/workspace/cr3"
ALL_STAGES="init,depot,fetch,deps,rbe,sync,build"
STAGES_TO_RUN=""

# Display help information
show_help() {
  echo "Chromium Setup Script"
  echo "Usage: $0 [options]"
  echo
  echo "Options:"
  echo "  -h, --help         Show this help message"
  echo "  -s, --stages       Specify stages to run (comma-separated)"
  echo "                     Available stages: init,depot,fetch,deps,rbe,sync,build"
  echo "                     Default: all stages"
  echo "  -v, --verbose      Enable verbose output (set -x)"
  echo "  -d, --directory    Set Chromium root directory (default: /workspace/cr3)"
  echo
  echo "Examples:"
  echo "  $0 --stages=init,depot,fetch  # Only initialize, clone depot_tools, and fetch source"
  echo "  $0 --stages=build             # Only build Chromium (assumes previous stages completed)"
}

# Parse command line arguments
parse_args() {
  while [[ $# -gt 0 ]]; do
    case $1 in
      -h|--help)
        show_help
        exit 0
        ;;
      -s=*|--stages=*)
        STAGES_TO_RUN="${1#*=}"
        ;;
      -s|--stages)
        STAGES_TO_RUN="$2"
        shift
        ;;
      -v|--verbose)
        set -x
        ;;
      -d=*|--directory=*)
        CHROMIUM_ROOT="${1#*=}"
        ;;
      -d|--directory)
        CHROMIUM_ROOT="$2"
        shift
        ;;
      *)
        echo "Unknown option: $1"
        show_help
        exit 1
        ;;
    esac
    shift
  done

  # If no stages specified, run all
  if [ -z "$STAGES_TO_RUN" ]; then
    STAGES_TO_RUN="$ALL_STAGES"
  fi
}

# Stage functions
init_stage() {
  echo "=== Stage: Initialization ==="
  echo "Setting up directories..."
  mkdir -p ${CHROMIUM_ROOT}
  
  # Install dependencies
  echo "Installing dependencies..."
  sudo apt install git -y
}

depot_tools_stage() {
  echo "=== Stage: Setting up depot_tools ==="
  cd $HOME
  if [ ! -d "$HOME/depot_tools" ]; then
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
  else
    rm -rf "$HOME/depot_tools" 
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
  fi

  export PATH="${HOME}/depot_tools:$PATH"
}

fetch_chromium_stage() {
  echo "=== Stage: Fetching Chromium source ==="
  cd ${CHROMIUM_ROOT}
  echo "Fetching Chromium source code (this may take a while)..."
  fetch --nohooks chromium
}

deps_stage() {
  echo "=== Stage: Installing dependencies and running hooks ==="
  cd ${CHROMIUM_ROOT}/src

  # Install build dependencies
  echo "Installing build dependencies..."
  ./build/install-build-deps.sh

  # Run hooks
  echo "Running hooks..."
  gclient runhooks
}

rbe_stage() {
  echo "=== Stage: Setting up RBE (Remote Build Execution) ==="
  cd ${CHROMIUM_ROOT}
  git clone https://microsoft.visualstudio.com/DefaultCollection/Edge/_git/chromium.depot_tools.cr-contrib

  # Update PATH for Microsoft depot_tools
#   export PATH="${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib:${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib/scripts:$PATH"

#   # Add the path to .bashrc for future sessions
#   echo "Adding Microsoft depot_tools to PATH in .bashrc..."
#   echo "export PATH=\"${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib:${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib/scripts:\$PATH\"" >> ${HOME}/.bashrc
}

sync_stage() {
  export PATH="${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib:${CHROMIUM_ROOT}/chromium.depot_tools.cr-contrib/scripts:$PATH"
  echo "=== Stage: Syncing dependencies ==="
  cd ${CHROMIUM_ROOT}/src
  gclient sync
}

build_stage() {
  echo "=== Stage: Building Chromium ==="
  cd ${CHROMIUM_ROOT}/src
  autogn x64 release
  autoninja -C out/release_x64 chrome
  
  echo "Chromium setup and build completed!"
  echo "You can run Chromium with: ${CHROMIUM_ROOT}/src/out/release_x64/chrome"
}

# Run specified stages
run_stages() {
  IFS=',' read -ra STAGE_ARRAY <<< "$STAGES_TO_RUN"
  
  for stage in "${STAGE_ARRAY[@]}"; do
    case "$stage" in
      init)
        init_stage
        ;;
      depot)
        depot_tools_stage
        ;;
      fetch)
        fetch_chromium_stage
        ;;
      deps)
        deps_stage
        ;;
      rbe)
        rbe_stage
        ;;
      sync)
        sync_stage
        ;;
      build)
        build_stage
        ;;
      *)
        echo "Unknown stage: $stage"
        show_help
        exit 1
        ;;
    esac
  done
}

# Main execution
echo "Starting Chromium setup process..."
echo "CHROMIUM_ROOT set to: ${CHROMIUM_ROOT}"

parse_args "$@"
run_stages

echo "Script execution completed!"