# GitHub Codespace Setup Guide for Ara

## Problem

When running `make bin/fmatmul` (or any application) in GitHub Codespace, you get the error:
```
bash: line 1: /workspaces/ara/install/riscv-llvm/bin/clang: No such file or directory
```

This happens because the RISC-V LLVM toolchain with newlib is not installed in the Codespace environment.

## Solution

### Quick Setup (Recommended)

1. **Open terminal in your GitHub Codespace**

2. **Navigate to the ara directory**:
   ```bash
   cd /workspaces/ara
   ```

3. **Run the automated setup script**:
   ```bash
   ./setup_codespace.sh
   ```

This script will:
- Install system dependencies
- Initialize git submodules  
- Build the complete RISC-V LLVM toolchain with newlib
- Verify the installation
- Test with hello_world application

**Expected time**: 45-90 minutes (mostly automated)

### Manual Setup (Alternative)

If you prefer to run the steps manually:

```bash
# 1. Install dependencies
sudo apt-get update
sudo apt-get install -y libmpc-dev ninja-build flex libfl-dev help2man texinfo build-essential python3 python3-pip

# 2. Initialize submodules
git submodule update --init --recursive --checkout -- toolchain/riscv-llvm
git submodule update --init --recursive --checkout -- toolchain/newlib

# 3. Build complete toolchain (this takes time!)
CC=gcc CXX=g++ make toolchain-llvm
```

## Verification

After setup, verify everything works:

```bash
# Check clang exists
ls -la install/riscv-llvm/bin/clang

# Check newlib headers
find install/riscv-llvm/riscv64-unknown-elf -name "string.h"

# Test building an application
cd apps
make bin/hello_world
```

## Why This Happens

The GitHub Codespace environment starts fresh and only contains the source code. Unlike your local environment where you've built the toolchain, the Codespace needs to build:

1. **LLVM/Clang compiler** (`toolchain-llvm-main`)
2. **Newlib C library** (`toolchain-llvm-newlib`) 
3. **Compiler runtime** (`toolchain-llvm-rt`)

The CI workflow in `.github/workflows/ci.yml` builds these components but doesn't persist them for interactive use.

## Optimization for Future Use

Consider creating a Docker container or using GitHub Codespace's prebuild feature to avoid rebuilding the toolchain every time:

### Option 1: Prebuild Configuration
Create `.devcontainer/devcontainer.json`:
```json
{
  "name": "Ara Development",
  "image": "ubuntu:22.04",
  "postCreateCommand": "./setup_codespace.sh",
  "features": {
    "ghcr.io/devcontainers/features/common-utils:2": {},
    "ghcr.io/devcontainers/features/git:1": {}
  }
}
```

### Option 2: Cache Toolchain
The toolchain files can be cached between sessions by committing them to a separate branch:
```bash
# After successful build
git checkout -b toolchain-cache
git add install/
git commit -m "Add built toolchain for Codespace use"
git push origin toolchain-cache
```

Then modify the setup script to check for cached toolchain first.

## Troubleshooting

### Out of Space Error
Codespaces have limited disk space. If you encounter space issues:
```bash
# Clean build artifacts
make clean-all
# Or just clean LLVM build
rm -rf toolchain/riscv-llvm/build
```

### Build Fails
If build fails partway through:
```bash
# Check what failed
make toolchain-llvm-main  # Build just LLVM
make toolchain-llvm-newlib  # Build just newlib
make toolchain-llvm-rt    # Build just runtime
```

### Still Missing Headers
If string.h is still missing after build:
```bash
# Manually check newlib build
cd toolchain/newlib
make install-target-newlib
```

## Performance Notes

- **LLVM main build**: ~30-60 minutes
- **Newlib build**: ~10-20 minutes  
- **Runtime build**: ~5-10 minutes
- **Total**: ~45-90 minutes

The setup script runs these in sequence and provides progress updates.
