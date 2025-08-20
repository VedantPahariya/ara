# Ara RISC-V Cycle Counting Fix and Benchmarking Guide

This document provides step-by-step instructions to fix cycle counting issues in Ara RISC-V applications and perform accurate benchmarking.

## Problem Description

When running `make bin/fmatmul`, `make bin/fmatmul.spike`, or `make spike-run-fmatmul`, the cycle counts were showing as 0 or displaying literal "%f" instead of performance metrics.

## Root Causes Identified

1. **Timer Functions Disabled in SPIKE**: The `runtime.h` file had SPIKE timer functions that always returned 0
2. **Printf Formatting Issues**: SPIKE's limited libc support doesn't handle `%f` format specifiers properly

## Step-by-Step Fix Instructions

### Step 1: Fix Timer Functions in runtime.h

Navigate to the runtime header file and enable SPIKE timer functions:

```bash
cd /path/to/ara/apps/common/
```

Edit `runtime.h` to enable timer functions for SPIKE builds:

**Original Code (lines 39-45):**
```cpp
#else
#define HW_CNT_READY ;
#define HW_CNT_NOT_READY ;
// Start and stop the counter - DISABLED for SPIKE
inline void start_timer() { }
inline void stop_timer() { }
// Get the value of the timer - DISABLED for SPIKE
inline int64_t get_timer() { return 0; }
```

**Fixed Code:**
```cpp
#else
#define HW_CNT_READY ;
#define HW_CNT_NOT_READY ;
// Start and stop the counter - Enable timing for SPIKE
inline void start_timer() { timer = -get_cycle_count(); }
inline void stop_timer() { timer += get_cycle_count(); }

// Get the value of the timer
inline int64_t get_timer() { return timer; }
```

### Step 2: Fix Printf Formatting in fmatmul/main.c

Navigate to the fmatmul application:

```bash
cd /path/to/ara/apps/fmatmul/
```

Edit `main.c` to add SPIKE-compatible printf formatting:

**Add after existing includes (around line 30):**
```c
// SPIKE-compatible printf formatting
#ifdef SPIKE
#define PRINTF_FLOAT_AS_INT 1
#endif
```

**Replace performance printing section (around lines 90-95):**

**Original Code:**
```c
printf("The execution took %d cycles.\n", runtime);
printf("The performance is %f FLOP/cycle (%f%% utilization).\n",
       performance, utilization);
```

**Fixed Code:**
```c
printf("The execution took %ld cycles.\n", runtime);
#ifdef SPIKE
// SPIKE doesn't support %f, so convert to integer representation
int perf_int = (int)(performance * 1000); // 3 decimal places
int util_int = (int)(utilization * 1000);
printf("The performance is %d.%03d FLOP/cycle (%d.%03d%% utilization).\n",
       perf_int/1000, perf_int%1000, util_int/1000, util_int%1000);
#else
printf("The performance is %f FLOP/cycle (%f%% utilization).\n",
       performance, utilization);
#endif
```

### Step 3: Fix Additional Printf Format Warnings

Fix remaining format specifier warnings in `main.c`:

**Fix format specifiers (various lines):**
```c
// Change %d to %ld for int64_t variables
printf("The execution took %ld cycles.\n", runtime);

// Change %d to %f for double variables  
printf("The performance is %f FLOP/cycle (%f%% utilization).\n",
       performance, utilization);
```

## Step-by-Step Benchmarking Instructions

### Method 1: SPIKE Simulation (Fast, Functional Verification)

1. **Clean and build the applications:**
```bash
cd /path/to/ara/apps
make clean
```

2. **Build the regular version:**
```bash
make bin/fmatmul
```

3. **Build the SPIKE version:**
```bash
make bin/fmatmul.spike
```

4. **Run SPIKE simulation:**
```bash
make spike-run-fmatmul
```

**Expected Output:**
```
=============
=  FMATMUL  =
=============

------------------------------------------------------------
Calculating a (4 x 4) x (4 x 4) matrix multiplication...
------------------------------------------------------------

Calculating fmatmul...
The execution took 196 cycles.
The performance is 0.653 FLOP/cycle (8.163% utilization).

[... continues for different matrix sizes ...]

------------------------------------------------------------
Calculating a (128 x 128) x (128 x 128) matrix multiplication...
------------------------------------------------------------

Calculating fmatmul...
The execution took 49949 cycles.
The performance is 83.971 FLOP/cycle (1049.646% utilization).
Verifying result...
Passed.
```

### Method 2: RTL Hardware Simulation (Cycle-Accurate)

1. **Navigate to Ara root directory:**
```bash
cd /path/to/ara
```

2. **Run hardware benchmark script:**
```bash
scripts/benchmark.sh fmatmul
```

3. **Alternative manual RTL simulation:**
```bash
# Generate benchmark data
python3 apps/fmatmul/script/gen_data.py 128 128 128 > apps/benchmarks/data/data.S

# Compile for hardware simulation  
config=default ENV_DEFINES="-DFMATMUL=1" make -C apps/ bin/benchmarks

# Run RTL simulation with Verilator
config=default make -C hardware/ simv app=benchmarks
```

## Verification and Testing Instructions

### Test 1: Basic Functionality Test

Create and run a simple cycle counter test:

```bash
cd /path/to/ara/apps

# Create test directory
mkdir -p cycle_accuracy_test

# Create test file (content provided in previous steps)
# ... [test file creation] ...

# Build and run test
make bin/cycle_accuracy_test.spike
/path/to/ara/install/riscv-isa-sim/bin/spike --isa=rv64gcv_zfh --varch="vlen:4096,elen:64" bin/cycle_accuracy_test.spike
```

### Test 2: Consistency Verification

Run the same benchmark multiple times to verify consistency:

```bash
for i in {1..3}; do 
    echo "Run $i:"
    make spike-run-fmatmul | grep "execution took"
done
```

### Test 3: Cross-Platform Comparison

Compare SPIKE vs RTL results:

```bash
# SPIKE results
make spike-run-fmatmul > spike_results.txt

# RTL results (if available)
scripts/benchmark.sh fmatmul > rtl_results.txt

# Compare relative performance ratios between different matrix sizes
```

## Build System Verification

### Check Build Targets

Verify all necessary binaries are built:

```bash
ls -la bin/fmatmul*
# Should show:
# bin/fmatmul          (regular version)
# bin/fmatmul.spike    (SPIKE version)  
# bin/fmatmul.dump     (disassembly)
# bin/fmatmul.spike.dump (SPIKE disassembly)
```

### Clean Build Process

For clean rebuilds:

```bash
cd /path/to/ara/apps
make clean
make bin/fmatmul
make bin/fmatmul.spike
```

## Expected Results and Validation

### SPIKE Simulation Results

**Performance scaling for fmatmul:**
- 4×4 matrix: ~196 cycles, 0.653 FLOP/cycle
- 8×8 matrix: ~311 cycles, 3.292 FLOP/cycle  
- 16×16 matrix: ~876 cycles, 9.351 FLOP/cycle
- 32×32 matrix: ~3083 cycles, 21.257 FLOP/cycle
- 64×64 matrix: ~11721 cycles, 44.730 FLOP/cycle
- 128×128 matrix: ~49949 cycles, 83.971 FLOP/cycle

### Validation Criteria

✅ **Success Indicators:**
- Cycle counts are non-zero positive integers
- Performance metrics show floating-point values (not "%f")  
- Results scale appropriately with matrix size
- Multiple runs show consistent results (<1% variation)
- "Passed" verification message appears

❌ **Failure Indicators:**
- Cycle count shows 0
- Performance shows "%f" literally  
- Build errors or compilation failures
- Verification fails with error codes

## Troubleshooting Common Issues

### Issue 1: Still Getting 0 Cycles

**Check:** Ensure `runtime.h` changes are applied correctly
```bash
grep -A 5 -B 5 "get_cycle_count" /path/to/ara/apps/common/runtime.h
```

### Issue 2: Printf Showing "%f"

**Check:** Ensure SPIKE-compatible printf formatting is implemented
```bash
grep -A 10 -B 5 "SPIKE" /path/to/ara/apps/fmatmul/main.c
```

### Issue 3: Build Failures

**Solution:** Clean and rebuild:
```bash
cd /path/to/ara/apps
make clean
make bin/fmatmul.spike
```

### Issue 4: SPIKE Not Found

**Check SPIKE installation:**
```bash
which spike
/path/to/ara/install/riscv-isa-sim/bin/spike --version
```

## Performance Analysis Guidelines

### SPIKE Results Interpretation

- **Functional Accuracy**: ✅ Perfect for algorithm verification
- **Relative Performance**: ✅ Good for comparing implementations  
- **Absolute Performance**: ⚠️ Not cycle-accurate vs real hardware
- **Use Cases**: Development, debugging, functional testing

### RTL Results Interpretation  

- **Cycle Accuracy**: ✅ Matches real hardware behavior
- **Performance Analysis**: ✅ Suitable for optimization
- **Timing Analysis**: ✅ Real pipeline and memory effects
- **Use Cases**: Performance tuning, final validation

## Summary

This guide provides a complete workflow to:

1. ✅ Fix timer function issues in `runtime.h`
2. ✅ Resolve printf formatting problems in application code  
3. ✅ Build both SPIKE and regular versions of applications
4. ✅ Run functional verification with cycle counting
5. ✅ Validate results and verify accuracy
6. ✅ Perform both SPIKE and RTL benchmarking

The fixes enable proper cycle counting in SPIKE simulation while maintaining compatibility with hardware simulation, providing a complete benchmarking solution for the Ara vector processor.
