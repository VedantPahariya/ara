# Verilator Setup Fix Guide for Ara RISC-V Vector Processor

**Date:** September 7, 2025  
**Environment:** Ubuntu 22.04 (without sudo access)  
**Project:** Ara RISC-V Vector Processor Hardware Simulation

## Problem Summary

When attempting to run the Ara hardware simulation with the command `app=hello_world make simv`, the following error occurred:

```bash
bash: line 1: /ssd_scratch/vedant.pahariya/ara/install/verilator/bin/verilator: No such file or directory
make: *** [Makefile:188: build/verilator/Vara_tb_verilator] Error 127
```

**Root Cause:** Missing Verilator installation in the expected location.

## Solution Overview

The fix involved building Verilator from source and resolving all dependency issues manually, since we didn't have sudo access to install packages system-wide.

## Step-by-Step Fix Process

### Step 0: Setup Environment (Portable Configuration)

Before starting, set up the environment variable for your Ara installation path:

```bash
# Navigate to your Ara project directory
cd /path/to/your/ara  # Change this to your actual Ara location

# Set the Ara root directory environment variable
export ARA_ROOT=$(pwd)

# Verify the path is correct
echo "Ara root: $ARA_ROOT"
ls $ARA_ROOT  # Should show: apps/, hardware/, toolchain/, etc.

# Optional: Add to your shell profile for persistence
echo "export ARA_ROOT=$ARA_ROOT" >> ~/.bashrc  # or ~/.zshrc
```

**Note**: All commands in this guide use `${ARA_ROOT}` which will work on any system once this variable is set correctly.

### Step 1: Investigate the Missing Verilator Installation

First, we confirmed that Verilator was indeed missing:

```bash
ls -la ${ARA_ROOT}/install/verilator/
# Result: No such file or directory
```

We found that Verilator source code was available in the toolchain directory:

```bash
ls -la ${ARA_ROOT}/toolchain/verilator/
# Found Verilator 5.012 source code
```

### Step 2: Clean Up Previous Installation Attempts

Removed any dangling symbolic links or partial installations:

```bash
cd ${ARA_ROOT}
rm -f install/verilator  # Remove broken symlink if it exists
```

### Step 3: Build Verilator from Source

#### Initial Configuration Attempt

First attempt with default compilers:

```bash
cd ${ARA_ROOT}/toolchain/verilator
./configure --prefix=${ARA_ROOT}/install/verilator
```

This succeeded, but the subsequent make failed due to compiler issues.

#### Compiler Issue Resolution

The build failed with clang++ linking errors. We switched to GCC:

```bash
cd ${ARA_ROOT}/toolchain/verilator
make clean  # Clean previous build artifacts
CC=gcc CXX=g++ ./configure --prefix=${ARA_ROOT}/install/verilator
```
used "make verilator CLANG_CC=gcc CLANG_CXX=g++" while multicore ara setup

#### Successful Build and Installation

```bash
make -j$(nproc)  # Build with parallel jobs
make install     # Install to the prefix directory
```

**Build Time:** Approximately 5-10 minutes on the available hardware.

### Step 4: Verify Verilator Installation

After installation, we verified the basic functionality:

```bash
${ARA_ROOT}/install/verilator/bin/verilator --version
# Output: Verilator 5.012 2023-01-15 rev v5.012
```

Check installation structure:

```bash
ls -la ${ARA_ROOT}/install/verilator/
# bin/  include/  share/
```

### Step 5: Fix Missing verilator_bin Executable

During testing, we discovered that the Verilator wrapper script expected `verilator_bin` but only `verilator_bin_dbg` was present. We created the necessary symbolic links:

```bash
cd ${ARA_ROOT}/install/verilator/bin/
ln -sf verilator_bin_dbg verilator_bin
```

### Step 6: Resolve libelf Dependency Issues

#### First Compilation Attempt

When running the build, we encountered:

```bash
fatal error: libelf.h: No such file or directory
```

This indicated missing development headers for libelf.

#### Download and Extract libelf Development Headers

Since we couldn't use `sudo apt install`, we manually downloaded and extracted the package:

```bash
apt download libelf-dev
dpkg -x libelf-dev_0.186-1ubuntu0.1_amd64.deb /tmp/extracted/
```

#### Install Headers to DPI Module

We copied the headers to the DPI module directory where they were needed:

```bash
find /tmp/extracted -name "libelf.h"
# Found: /tmp/extracted/usr/include/libelf.h

cp /tmp/extracted/usr/include/libelf.h ${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/cpp/
cp -r /tmp/extracted/usr/include/elfutils/ ${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/cpp/
cp /tmp/extracted/usr/include/gelf.h ${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/cpp/
cp /tmp/extracted/usr/include/nlist.h ${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/cpp/
```

### Step 7: Resolve libelf Runtime Library Issues

#### Second Compilation Attempt

After resolving the header issue, the compilation succeeded but linking failed:

```bash
/usr/bin/ld: cannot find -lelf: No such file or directory
```

#### Download and Install libelf Runtime Library

Downloaded the runtime library:

```bash
apt download libelf1
dpkg -x libelf1_0.186-1ubuntu0.1_amd64.deb extracted_lib/
```

Created a local library directory and installed the library:

```bash
mkdir -p ${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib
cp extracted_lib/usr/lib/x86_64-linux-gnu/libelf-0.186.so ${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib/
cd ${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib/
ln -sf libelf-0.186.so libelf.so.1
ln -sf libelf-0.186.so libelf.so
```

### Step 8: Configure Library Paths and Final Build

Set the necessary environment variables for library discovery:

```bash
export LD_LIBRARY_PATH="${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:$LD_LIBRARY_PATH"
export LIBRARY_PATH="${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:$LIBRARY_PATH"
```

### Step 9: Successful Build and Test

Finally, run the complete build process:

```bash
cd ${ARA_ROOT}/hardware
app=hello_world make verilate  # Build Verilator simulation
app=hello_world make simv      # Run simulation
```

## Verification of Success

The successful simulation output confirmed the fix:

```
Simulation of Ara
=================

Simulation running, end by pressing CTRL-c.
Ariane says Hello!
[hw-cycles]:           0
[2716] -Info: ara_tb_verilator.sv:51: TOP.ara_tb_verilator: Core Test *** SUCCESS *** (tohost = 0)

Simulation statistics
=====================
Executed cycles:  54e
Wallclock time:   0.808 s
Simulation speed: 1680.69 cycles/s (1.68069 kHz)
```

## Key Files Created/Modified

### New Files Created:
- `${ARA_ROOT}/install/verilator/` - Complete Verilator installation
- `${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib/` - Local libelf library directory
- `${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/cpp/libelf.h` - libelf header file

### Symbolic Links Created:
- `verilator_bin -> verilator_bin_dbg`
- `libelf.so -> libelf-0.186.so`
- `libelf.so.1 -> libelf-0.186.so`

## Environment Variables Required

For future builds, ensure these environment variables are set:

```bash
export ARA_ROOT=$(pwd) # Set this to your actual Ara installation path
export LD_LIBRARY_PATH="${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:$LD_LIBRARY_PATH"
export LIBRARY_PATH="${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:$LIBRARY_PATH"
```

**Tip**: Add these to your `~/.bashrc` or `~/.zshrc` to make them permanent:

```bash
echo "export ARA_ROOT=/path/to/your/ara" >> ~/.bashrc
echo "export LD_LIBRARY_PATH=\"\${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:\$LD_LIBRARY_PATH\"" >> ~/.bashrc
echo "export LIBRARY_PATH=\"\${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:\$LIBRARY_PATH\"" >> ~/.bashrc
```

## Lessons Learned

1. **Compiler Selection Matters**: GCC worked better than Clang for this specific build environment
2. **Manual Dependency Management**: Without sudo access, manual package extraction is a viable solution
3. **Library Path Configuration**: Proper `LD_LIBRARY_PATH` and `LIBRARY_PATH` configuration is crucial for linking
4. **Systematic Debugging**: Breaking down the problem into compilation vs. linking phases helped identify specific issues

## Troubleshooting Tips

If you encounter similar issues:

1. **Check Verilator Installation**: Verify `verilator --version` works
2. **Verify Library Paths**: Ensure libelf can be found with `ldd` command on compiled binaries
3. **Environment Variables**: Always set library paths before building
4. **Clean Builds**: Use `make clean` when switching compilers or build configurations

## Performance Notes

- **Build Time**: Verilator compilation takes 5-10 minutes
- **Simulation Speed**: Achieved ~1.68 kHz simulation speed for the hello_world application
- **Memory Usage**: Peak memory usage during build was manageable within typical system limits

## Future Maintenance

To maintain this setup:

1. Keep the environment variables in your shell profile
2. Document any additional applications that require these library paths
3. Consider creating a setup script for other developers
4. Monitor for Verilator version updates that might require rebuilding

---

**Status**: ✅ **RESOLVED** - Ara Verilator simulation is fully functional  
**Next Steps**: Ready for hardware development and testing with Verilator simulation
