#!/bin/bash

# GitHub Codespace Setup Script for Ara RISC-V LLVM Toolchain
# This script sets up the complete toolchain including newlib in a GitHub Codespace

set -e  # Exit on any error

echo "=== Ara RISC-V LLVM Toolchain Setup for GitHub Codespace ==="

# Get the current directory (should be /workspaces/ara)
ARA_ROOT=$(pwd)
echo "Ara root directory: $ARA_ROOT"

# Export for use in subprocesses
export ARA_ROOT=$ARA_ROOT

# Check if we're in the right directory
if [[ ! -f "Makefile" ]] || [[ ! -d "apps" ]]; then
    echo "ERROR: This script must be run from the Ara root directory"
    echo "Expected to find Makefile and apps/ directory"
    exit 1
fi

# Install required system dependencies
echo "=== Installing system dependencies ==="
sudo apt-get update
sudo apt-get install -y \
    libmpc-dev \
    ninja-build \
    flex \
    libfl-dev \
    help2man \
    texinfo \
    build-essential \
    python3 \
    python3-pip

# Initialize submodules if not already done
echo "=== Initializing git submodules ==="
git submodule update --init --recursive --checkout -- toolchain/riscv-llvm
git submodule update --init --recursive --checkout -- toolchain/newlib

# Check if toolchain already exists
if [[ -d "install/riscv-llvm/bin" ]] && [[ -f "install/riscv-llvm/bin/clang" ]]; then
    echo "=== LLVM toolchain already exists ==="
    
    # Check if newlib is installed
    if find install/riscv-llvm/riscv64-unknown-elf -name "string.h" 2>/dev/null | grep -q string.h; then
        echo "=== Newlib already installed ==="
        echo "=== Toolchain setup complete! ==="
        exit 0
    else
        echo "=== LLVM found but newlib missing, building newlib... ==="
        # Skip to newlib build
        BUILD_MAIN=false
    fi
else
    echo "=== Building complete LLVM toolchain ==="
    BUILD_MAIN=true
fi

# Build LLVM main toolchain if needed
if [[ "$BUILD_MAIN" == "true" ]]; then
    echo "=== Building LLVM main toolchain (this may take 30-60 minutes) ==="
    CC=gcc CXX=g++ make toolchain-llvm-main
    echo "=== LLVM main toolchain build complete ==="
fi

# Build newlib
echo "=== Building newlib (this may take 10-20 minutes) ==="
CC=gcc CXX=g++ make toolchain-llvm-newlib

# Build compiler runtime
echo "=== Building LLVM compiler runtime ==="
CC=gcc CXX=g++ make toolchain-llvm-rt

echo "=== Verifying installation ==="

# Verify clang exists
if [[ ! -f "install/riscv-llvm/bin/clang" ]]; then
    echo "ERROR: clang not found at install/riscv-llvm/bin/clang"
    exit 1
fi

# Verify newlib headers exist
if ! find install/riscv-llvm/riscv64-unknown-elf -name "string.h" 2>/dev/null | grep -q string.h; then
    echo "ERROR: newlib headers not found (string.h missing)"
    exit 1
fi

echo "=== Success! Toolchain verification ==="
echo "✓ Clang: $(install/riscv-llvm/bin/clang --version | head -n1)"
echo "✓ Newlib headers found"

# Test a simple build
echo "=== Testing with hello_world app ==="
cd apps
if make bin/hello_world; then
    echo "✓ Successfully built hello_world application"
else
    echo "WARNING: Failed to build hello_world application"
    echo "This might indicate an issue with the toolchain setup"
fi

echo ""
echo "=== Setup Complete! ==="
echo "The RISC-V LLVM toolchain with newlib is now installed."
echo "You can now build applications using:"
echo "  cd apps"
echo "  make bin/<app_name>"
echo ""
echo "Available applications:"
ls -1 apps/ | grep -v -E '\.(log|o|mk)$|Makefile|README|^bin$|^common$|^benchmarks$|^script$|spike_runs$' | head -10
echo "..."
