# Ara RISC-V LLVM Toolchain Setup and Verification Guide

This document provides a complete step-by-step guide for building the RISC-V LLVM toolchain with newlib and verifying that the entire Ara setup is working correctly.

## Problem Context

After building the LLVM toolchain using `make toolchain-llvm`, applications failed to compile with the error:
```
hello_world/main.c:20:10: fatal error: 'string.h' file not found
   20 | #include <string.h>
      |          ^~~~~~~~~~
1 error generated.
```

This indicated that newlib (C standard library) was not properly built and installed.

## Root Cause Analysis

The issue was identified in the Makefile structure:

1. **Inefficient Build Logic**: The `toolchain-llvm-newlib` and `toolchain-llvm-rt` targets were unnecessarily rebuilding the entire LLVM toolchain due to `rm -rf build` commands.

2. **Missing Components**: The full LLVM toolchain requires three components:
   - `toolchain-llvm-main` (LLVM/Clang compiler) ✅ Built
   - `toolchain-llvm-newlib` (C standard library) ❌ Missing  
   - `toolchain-llvm-rt` (compiler runtime) ❌ Missing

## Step-by-Step Solution

### 0. Setup Environment (Portable Configuration)

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

### 1. Verify LLVM Installation

First, check what components are already installed:

```bash
# Check LLVM installation directory
ls -la ${ARA_ROOT}/install/riscv-llvm/

# Check LLVM binaries
ls ${ARA_ROOT}/install/riscv-llvm/bin/ | head -10

# Verify clang is working
${ARA_ROOT}/install/riscv-llvm/bin/clang --version
```

### 2. Check for Missing Headers

```bash
# Look for newlib headers (should be empty if not installed)
find ${ARA_ROOT}/install/riscv-llvm -name "string.h" -type f

# Check for RISC-V target directory (should be missing)
find ${ARA_ROOT}/install/riscv-llvm -name "riscv64-unknown-elf" -type d
```

# Check for RISC-V target directory (should be missing)
find /home/vedant/Desktop/ara/install/riscv-llvm -name "riscv64-unknown-elf" -type d


### 3. Manual Newlib Build (Avoiding Full LLVM Rebuild)

Since the Makefile inefficiently rebuilds LLVM, build newlib manually:

```bash
# Set the Ara root directory (adapt this path to your setup)
export ARA_ROOT=$(pwd)  # Assumes you're in the ara directory
# OR use absolute path: export ARA_ROOT=/path/to/your/ara

# Navigate to newlib directory
cd ${ARA_ROOT}/toolchain/newlib

# Remove old build directory and create new one
rm -rf build && mkdir -p build && cd build

# Configure newlib for RISC-V target (portable version)
../configure --prefix=${ARA_ROOT}/install/riscv-llvm \
  --target=riscv64-unknown-elf \
  CC_FOR_TARGET="${ARA_ROOT}/install/riscv-llvm/bin/clang -march=rv64gc -mabi=lp64d -mno-relax -mcmodel=medany -Wno-error-implicit-function-declaration -Wno-error=int-conversion" \
  AS_FOR_TARGET=${ARA_ROOT}/install/riscv-llvm/bin/llvm-as \
  AR_FOR_TARGET=${ARA_ROOT}/install/riscv-llvm/bin/llvm-ar \
  LD_FOR_TARGET=${ARA_ROOT}/install/riscv-llvm/bin/llvm-ld \
  RANLIB_FOR_TARGET=${ARA_ROOT}/install/riscv-llvm/bin/llvm-ranlib

# Build newlib
make -j4

# Install newlib
make install
```

Running  make -j4 sometimes give error like 

instead try make install-target-newlib 

**Build Command Options:**

- **`make -j4`**
   - **Builds ALL targets** in the current Makefile
   - **Parallel execution** with 4 jobs simultaneously
   - For newlib: builds both `newlib` AND `libgloss` components
   - Includes documentation (where texinfo error occurs)

- **`make all-target-newlib -j4`**
   - **Builds ONLY newlib** (the C standard library)
   - **Skips libgloss** (board support package)
   - Skips documentation
   - Parallel execution with 4 jobs
   - Faster and more targeted

- **`make install-target-newlib`**
   - Only installs already-built newlib components
   - Does NOT build - assumes `make all-target-newlib` was run first
   - No parallelization (installation is typically fast)
   - Copies files to `${ARA_ROOT}/install/riscv-llvm/`

**Recommended Approach:**
```bash
# Build newlib only (avoiding documentation issues)
make all-target-newlib -j4

# Install the built components
make install-target-newlib
```

### 4. Verify Newlib Installation

```bash
# Check if newlib headers are now installed
find ${ARA_ROOT}/install/riscv-llvm/riscv64-unknown-elf -name "string.h"

# Verify newlib libraries
ls ${ARA_ROOT}/install/riscv-llvm/riscv64-unknown-elf/lib/

# Should show: libc.a, libm.a, libg.a, crt0.o, etc.
```

### 5. Build Compiler-RT Runtime

The runtime library is needed for linking:

```bash
# Set the Ara root directory (adapt this path to your setup)
export ARA_ROOT=$(pwd)  # Assumes you're in the ara directory
# OR use absolute path: export ARA_ROOT=/path/to/your/ara

# Navigate to compiler-rt directory
cd ${ARA_ROOT}/toolchain/riscv-llvm/compiler-rt

# Remove old build and create new one
rm -rf build && mkdir -p build && cd build

# Configure compiler-rt (portable version)
cmake .. -G Ninja \
  -DCMAKE_INSTALL_PREFIX=${ARA_ROOT}/install/riscv-llvm \
  -DCMAKE_C_COMPILER_TARGET="riscv64-unknown-elf" \
  -DCMAKE_ASM_COMPILER_TARGET="riscv64-unknown-elf" \
  -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
  -DCOMPILER_RT_BAREMETAL_BUILD=ON \
  -DCOMPILER_RT_BUILD_BUILTINS=ON \
  -DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
  -DCOMPILER_RT_BUILD_MEMPROF=OFF \
  -DCOMPILER_RT_BUILD_PROFILE=OFF \
  -DCOMPILER_RT_BUILD_SANITIZERS=OFF \
  -DCOMPILER_RT_BUILD_XRAY=OFF \
  -DCMAKE_C_COMPILER_WORKS=1 \
  -DCMAKE_CXX_COMPILER_WORKS=1 \
  -DCMAKE_SIZEOF_VOID_P=4 \
  -DCMAKE_C_COMPILER="${ARA_ROOT}/install/riscv-llvm/bin/clang" \
  -DCMAKE_C_FLAGS="-march=rv64gc -mabi=lp64d -mno-relax -mcmodel=medany" \
  -DCMAKE_ASM_FLAGS="-march=rv64gc -mabi=lp64d -mno-relax -mcmodel=medany" \
  -DCMAKE_AR=${ARA_ROOT}/install/riscv-llvm/bin/llvm-ar \
  -DCMAKE_NM=${ARA_ROOT}/install/riscv-llvm/bin/llvm-nm \
  -DCMAKE_RANLIB=${ARA_ROOT}/install/riscv-llvm/bin/llvm-ranlib \
  -DLLVM_CONFIG_PATH=${ARA_ROOT}/install/riscv-llvm/bin/llvm-config

# Build only builtins (avoid sanitizer issues)
ninja builtins

# Install builtins
ninja install-builtins
```

### 6. Fix Runtime Library Path

The compiler expects the runtime library in a specific location:

```bash
# Create the correct directory structure
rm -f ${ARA_ROOT}/install/riscv-llvm/lib/clang/20/lib
mkdir -p ${ARA_ROOT}/install/riscv-llvm/lib/clang/20/lib/riscv64-unknown-unknown-elf

# Copy the runtime library to the expected location
cp ${ARA_ROOT}/install/riscv-llvm/lib/linux/libclang_rt.builtins-riscv64.a \
   ${ARA_ROOT}/install/riscv-llvm/lib/clang/20/lib/riscv64-unknown-unknown-elf/libclang_rt.builtins.a
```

### 7. Test Application Compilation

```bash
# Navigate to apps directory (from Ara root)
cd ${ARA_ROOT}/apps

# Test basic application compilation
make bin/hello_world
```

**Expected Output**: Should compile successfully with no errors about missing headers.

### 8. Test SPIKE Simulation

```bash
# Build for SPIKE simulation
make bin/hello_world.spike

# Run on SPIKE simulator
make spike-run-hello_world
```

**Expected Output**: 
```
Ariane says Hello!
*** FAILED *** (tohost = 1337)
```
The "FAILED" with tohost=1337 is normal and indicates successful execution.

## Complete Setup Verification

### 1. Verify All Toolchain Components

```bash
cd /home/vedant/Desktop/ara

# Check all installed components
ls install/

# Should show: riscv-llvm, riscv-isa-sim, riscv-isa-sim-mod, verilator
```

### 2. Test LLVM Toolchain Components

```bash
# Verify LLVM tools
ls install/riscv-llvm/bin/ | grep -E "(clang|llvm-)"

# Check newlib installation
ls install/riscv-llvm/riscv64-unknown-elf/include/ | head -10
ls install/riscv-llvm/riscv64-unknown-elf/lib/

# Verify compiler-rt
ls install/riscv-llvm/lib/clang/20/lib/riscv64-unknown-unknown-elf/
```

### 3. Test Multiple Applications

```bash
cd apps

# Test vector applications
make bin/fdotproduct
make bin/fmatmul

# Check built binaries
ls -la bin/
```

### 4. Verify Vector Instruction Generation

```bash
# Check that vector instructions are generated
grep -c "v[0-9]" bin/fmatmul.dump

# Look for specific vector instructions
grep "vle" bin/fmatmul.dump | head -3
```

**Expected Output**: Should show vector load instructions like `vle64.v v16, (s5)`

### 5. Test Verilator Installation

```bash
# Check Verilator binaries
ls install/verilator/bin/
```

### 6. Test Spike Simulators

```bash
# Check Spike installations
ls install/riscv-isa-sim/bin/
ls install/riscv-isa-sim-mod/bin/
```

## Issue of Missing Headers

I can see the issue! The compilation flags show that we're using LLVM but the issue is that clang isn't automatically finding the newlib headers. According to your guide, we need to update the Toolchain Setup Guide to include information about completing the installation correctly. Let me check the current status of the toolchain setup by looking at the actual compilation command that was executed:

The error shows that the compiler can't find string.h, which means we need to tell clang where to find the newlib headers. Let me check how the build system normally expects to work by creating a simple test:

```bash
${ARA_ROOT}/install/riscv-llvm/bin/clang --target=riscv64-unknown-elf -march=rv64gc -mabi=lp64d --sysroot=${ARA_ROOT}/install/riscv-llvm/riscv64-unknown-elf -E - <<< "#include <string.h>" > /dev/null && echo "Headers found!" || echo "Headers not found!"
```

The issue is that the build system needs to use proper compiler flags to tell clang where to find the newlib headers. According to your guide, this was a known issue. Let me check if the toolchain requires the --sysroot flag or similar to work properly.


## Application Build System Update Required

After successfully building newlib and compiler-rt, applications still fail to compile because the build system needs to be updated to include proper compiler flags. The issue is that clang needs to be told where to find the newlib headers and libraries.

### Required Fix for Application Compilation

The build system in `apps/common/runtime.mk` needs to be updated to include the correct flags for finding newlib. The LLVM_FLAGS should include:

```makefile
# Add sysroot and target flags for newlib
LLVM_FLAGS     ?= --target=riscv64-unknown-elf --sysroot=$(LLVM_INSTALL_DIR)/riscv64-unknown-elf -march=rv64gcv_zfh_zvfh -mabi=$(RISCV_ABI) -mno-relax -fuse-ld=lld
```

This tells clang:
- `--target=riscv64-unknown-elf`: Use the cross-compilation target
- `--sysroot=...`: Look for headers and libraries in the newlib installation directory

## Verification Results Summary

### ✅ Successfully Working Components:

1. **LLVM Toolchain**:
   - Clang compiler with RISC-V Vector Extension v1.0 support
   - Newlib C standard library with all headers (string.h, etc.)
   - Compiler-rt runtime libraries
   - All LLVM utilities (objdump, strip, etc.)

2. **Spike ISA Simulator**:
   - Standard and modified versions
   - Vector extension support
   - Successfully executes applications

3. **Verilator**:
   - Complete installation for RTL simulation

4. **Application Build System**:
   - Basic applications (`hello_world`)
   - Vector applications (`fdotproduct`, `fmatmul`)
   - SPIKE simulation workflow
   - Proper vector instruction generation

### ✅ Successfully Tested Workflows:

1. **Application Compilation**: `make bin/hello_world` ✓
2. **SPIKE Simulation**: `make bin/hello_world.spike` + `make spike-run-hello_world` ✓
3. **Vector Applications**: `make bin/fdotproduct` and `make bin/fmatmul` ✓
4. **Vector Instruction Verification**: Confirmed proper vector code generation ✓

### ⚠️ Known Limitations:

1. **GCC Toolchain**: Not built (required only for specific RISC-V tests)
2. **Hardware RTL Simulation**: Requires additional setup in `hardware/` directory

## Troubleshooting Common Issues

### Issue: "string.h not found"
**Solution**: Follow the newlib build steps above.

### Issue: "libclang_rt.builtins.a not found"
**Solution**: Build compiler-rt and copy to correct path as shown above.

### Issue: Makefile inefficiency
**Root Cause**: `rm -rf build` commands cause unnecessary full rebuilds.
**Workaround**: Use manual build steps to avoid rebuilding already-compiled components.

## Future Improvements

To fix the Makefile inefficiency, consider:

1. Adding dependency checks before rebuilding
2. Using separate build directories for different components  
3. Implementing proper incremental build logic

## Conclusion

The Ara RISC-V LLVM toolchain is now **completely functional** for:
- ✅ **Functional Verification**: Compiling C applications with RISC-V Vector Extension v1.0
- ✅ **Library Support**: Linking against newlib C standard library  
- ✅ **Code Generation**: Generating proper vector instructions
- ✅ **Spike Simulation**: Running functional simulations on Spike ISA simulator
- ✅ **Application Workflow**: Supporting the full software development workflow for vector processing

### For Performance Analysis:

**Spike Simulation** (Current Workflow):
- ✅ **Purpose**: Functional verification and correctness testing
- ❌ **Limitation**: No meaningful cycle counts (always returns 0)
- ✅ **Use Case**: Quick testing, debugging, algorithmic verification

**RTL Simulation** (For Performance Metrics):
- ✅ **Purpose**: Cycle-accurate performance analysis
- ✅ **Provides**: Real cycle counts, FLOP/cycle metrics, utilization percentages  
- ✅ **Tools**: Verilator-based hardware simulation
- ⚠️ **Requirement**: More complex setup, longer simulation time

### Summary:
- **For testing correctness**: Use Spike simulation (`make spike-run-fmatmul`) ✅
- **For performance analysis**: Use RTL simulation (`scripts/benchmark.sh fmatmul`) ✅

This setup enables both development/testing (Spike) and performance optimization (RTL) workflows for the Ara vector processor.

## Common Issues and Solutions

### Issue: Cycle Count Shows 0 in Spike Simulation

**Problem**: When running applications on Spike (e.g., `make spike-run-fmatmul`), you get:
```
The execution took 0 cycles.
The performance is %f FLOP/cycle (%f% utilization).
```

**Root Cause**: **Spike is a functional simulator, not a performance simulator**. It verifies correctness but doesn't provide meaningful cycle counts for performance analysis.

**Understanding the Issue**:
- When `SPIKE=1` is defined (which happens with `.spike` builds), the timer functions in `runtime.h` are disabled:
  - `start_timer()` → does nothing
  - `stop_timer()` → does nothing  
  - `get_timer()` → always returns 0
- This is **intentional design** - Spike focuses on functional verification

**Solution for Performance Analysis**: Use RTL (Hardware) Simulation:

```bash
# Method 1: Use the benchmark script for automated RTL simulation
cd ${ARA_ROOT}
scripts/benchmark.sh fmatmul

# Method 2: Manual RTL simulation steps
cd ${ARA_ROOT}

# 1. Generate benchmark data
python3 apps/fmatmul/script/gen_data.py 128 128 128 > apps/benchmarks/data/data.S

# 2. Compile for hardware simulation  
config=default ENV_DEFINES="-DFMATMUL=1" make -C apps/ bin/benchmarks

# 3. Run RTL simulation with Verilator
config=default make -C hardware/ simv app=benchmarks
```

**Expected RTL Output**: You should see real cycle counts like:
```
[hw-cycles] Runtime: 12543 cycles
The execution took 12543 cycles.
The performance is 2.15 FLOP/cycle (26.9% utilization).
```

**Note**: RTL simulation requires more setup and takes significantly longer than Spike functional simulation, but provides accurate performance metrics.

### Issue: DTC Error in Spike Simulation

**Problem**: When running spike simulations (e.g., `make spike-run-hello_world`), you encounter:
```
Failed to run dtc: No such file or directory
Child dtb process failed
```

**Root Cause**: The Device Tree Compiler (`dtc`) is required by Spike but is not in the system PATH.

**Solution**: The `dtc` binary is already built and installed with the Ara toolchain, but needs to be added to PATH:

```bash
# Add the riscv-isa-sim bin directory to your PATH
export PATH=${ARA_ROOT}/install/riscv-isa-sim/bin:$PATH

# Verify dtc is now accessible
which dtc
dtc --version  # Should show: Version: DTC 1.6.1

# Make this permanent by adding to your shell profile
echo "export PATH=${ARA_ROOT}/install/riscv-isa-sim/bin:\$PATH" >> ~/.bashrc
```

**Verification**: After adding to PATH, spike simulations should work correctly:
```bash
cd ${ARA_ROOT}/apps
make spike-run-hello_world
# Should output: "Ariane says Hello!" followed by "*** FAILED *** (tohost = 1337)"
# The "FAILED" message is expected - it indicates successful test completion
```

### Issue: RISC-V Tests Fail with "No such file or directory" for GCC

**Problem**: When running `make riscv_tests`, you encounter:
```
bash: line 1: /ssd_scratch/vedant.pahariya/ara/install/riscv-gcc/bin/riscv64-unknown-elf-gcc: No such file or directory
make: *** [common/crt0-gcc.S.o] Error 127
```

**Root Cause**: The RISC-V tests are hardcoded to use the GCC toolchain, but you only have the LLVM toolchain built.

**Solution 1: Build GCC Toolchain (Recommended for full compatibility)**
```bash
cd ${ARA_ROOT}
make toolchain-gcc
```

**Solution 2: Use LLVM for RISC-V Tests (Quick workaround)**
Override the GCC compiler settings to use LLVM:

```bash
cd ${ARA_ROOT}/apps
make riscv_tests \
    RISCV_CC_GCC="${ARA_ROOT}/install/riscv-llvm/bin/clang --target=riscv64-unknown-elf --sysroot=${ARA_ROOT}/install/riscv-llvm/riscv64-unknown-elf" \
    RISCV_LDFLAGS_GCC="-static -nostartfiles -lm -mcmodel=medany -march=rv64gcv -mabi=lp64d -I${ARA_ROOT}/apps/common -static -O3 -ffast-math -fno-common -fno-builtin-printf -DNR_LANES=4 -DVLEN=4096 -Wunused-variable -Wall -Wextra -Wno-unused-command-line-argument -std=gnu99 -T${ARA_ROOT}/apps/common/link.ld"
```

Or use the provided script:
```bash
${ARA_ROOT}/scripts/riscv_tests_llvm.sh
```

**Verification**: The tests should compile successfully. Some floating-point tests might have syntax errors in the test files themselves, but most integer and vector tests will work.

To install the Ara:
```bash
cd ${ARA_ROOT} && make toolchain-llvm-newlib
```

chmod +x /ssd_scratch/vedant.pahariya/ara/scripts/riscv_tests_llvm.sh

## Sequence of Problems
1. "string.h not found"

   The issue is that the build system needs to use proper compiler flags to tell clang where to find the newlib headers. Fixed by modifying the LLVM flag in runtime.mk file
2. "libclang_rt.builtins.a not found"

3. Makefile inefficiency
4. DTC Error in Spike Simulation

   Couldn't find the dtc so exported path for fixing it
5. RISC-V Tests Fail with "No such file or directory" for GCC