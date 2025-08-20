# Ara RISC-V Benchmarking - Quick Command Reference

## Essential Commands (Sequential Order)

### 1. Initial Setup and Navigation
```bash
# Navigate to Ara root directory
cd /path/to/ara

# Check directory structure
ls -la  # Should show: apps/, scripts/, hardware/, etc.
```

### 2. Apply Critical Fixes

**Fix A: Enable SPIKE Timer Functions**
```bash
# Edit apps/common/runtime.h
# In the #else block for SPIKE, change:
# OLD: inline void start_timer() { }
# NEW: inline void start_timer() { timer = -get_cycle_count(); }
# OLD: inline void stop_timer() { }  
# NEW: inline void stop_timer() { timer += get_cycle_count(); }
# OLD: inline int64_t get_timer() { return 0; }
# NEW: inline int64_t get_timer() { return timer; }
```

**Fix B: Add SPIKE Printf Compatibility**
```bash
# Edit apps/fmatmul/main.c
# Add SPIKE conditional formatting around printf statements
# Change %d to %ld for int64_t variables
# Add integer-based formatting for %f in SPIKE builds
```

### 3. Build Applications
```bash
cd apps/

# Clean previous builds
make clean

# Build regular version
make bin/fmatmul

# Build SPIKE version  
make bin/fmatmul.spike

# Verify builds
ls -la bin/fmatmul*
```

### 4. Run SPIKE Benchmarking
```bash
# Primary benchmarking command
make spike-run-fmatmul

# Alternative direct execution
/ssd_scratch/vedant.pahariya/ara/install/riscv-isa-sim/bin/spike \
  --isa=rv64gcv_zfh --varch="vlen:4096,elen:64" \
  bin/fmatmul.spike
```

### 5. Run RTL Benchmarking (Optional)
```bash
cd ..  # Back to ara root

# Automated RTL benchmark
scripts/benchmark.sh fmatmul

# Manual RTL steps
python3 apps/fmatmul/script/gen_data.py 128 128 128 > apps/benchmarks/data/data.S
config=default ENV_DEFINES="-DFMATMUL=1" make -C apps/ bin/benchmarks
config=default make -C hardware/ simv app=benchmarks
```

## Output Files and Locations

After running the benchmarking commands, you can find the results in:

### SPIKE Simulation Output:
- **Primary results**: `apps/spike_runs/spike-run-fmatmul` (clean performance data only)
- **Full build log**: `apps/spike_run_output.log` (includes compilation output + results)  
- **Build artifacts**: `apps/bin/fmatmul.spike`, `apps/bin/fmatmul.spike.dump`

### Viewing Results:
```bash
# View clean performance results
cat apps/spike_runs/spike-run-fmatmul

# View full execution log  
less apps/spike_run_output.log

# Check specific performance metrics
grep "execution took.*cycles" apps/spike_runs/spike-run-fmatmul
grep "performance is.*FLOP/cycle" apps/spike_runs/spike-run-fmatmul
```

### 6. Verification Commands
```bash
# Test consistency (run multiple times)
for i in {1..3}; do make spike-run-fmatmul | grep "execution took"; done

# Check cycle counter functionality
make bin/cycle_accuracy_test.spike
spike --isa=rv64gcv_zfh --varch="vlen:4096,elen:64" bin/cycle_accuracy_test.spike

# Validate build outputs
file bin/fmatmul bin/fmatmul.spike
```

## Expected Results Validation

### Successful SPIKE Output Pattern:
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

[...continues through larger matrices...]

------------------------------------------------------------  
Calculating a (128 x 128) x (128 x 128) matrix multiplication...
------------------------------------------------------------
Calculating fmatmul...
The execution took 49949 cycles.
The performance is 83.971 FLOP/cycle (1049.646% utilization).
Verifying result...
Passed.
```

### Key Success Indicators:
- ✅ Cycle counts > 0 (not zero)
- ✅ Performance shows decimal numbers (not "%f")
- ✅ Linear scaling with matrix size
- ✅ "Passed" verification message
- ✅ Consistent results across runs (<1% variation)

## Quick Troubleshooting

### If Still Getting 0 Cycles:
```bash
# Check runtime.h fix
grep -A 3 -B 3 "start_timer.*get_cycle_count" apps/common/runtime.h

# Rebuild cleanly
cd apps && make clean && make bin/fmatmul.spike
```

### If Still Getting "%f" Output:
```bash  
# Check printf fix
grep -A 5 -B 5 "SPIKE" apps/fmatmul/main.c

# Verify SPIKE define
grep -r "DSPIKE" apps/  # Should show in Makefile rules
```

### If Build Fails:
```bash
# Check toolchain
which clang
/path/to/ara/install/riscv-llvm/bin/clang --version

# Check dependencies
ls -la /path/to/ara/install/riscv-llvm/bin/
```

## Performance Reference Values

| Matrix Size | Expected Cycles | Performance (FLOP/cycle) |
|-------------|----------------|--------------------------|
| 4×4         | ~196           | ~0.653                  |
| 8×8         | ~311           | ~3.292                  |
| 16×16       | ~876           | ~9.351                  |
| 32×32       | ~3,083         | ~21.257                 |
| 64×64       | ~11,721        | ~44.730                 |
| 128×128     | ~49,949        | ~83.971                 |

## Automated Workflow Script

```bash
# Run the complete automated workflow
./benchmark_workflow.sh
```

This script performs all steps automatically and provides verification.
