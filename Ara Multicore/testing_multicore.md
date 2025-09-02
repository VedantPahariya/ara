# Testing and Benchmarking Multicore Ara Configurations

This guide provides comprehensive instructions for testing and benchmarking different Ara configurations, including both single-core and multicore setups.

## Prerequisites

Ensure you have completed the basic setup from `Setup_Repo.md`:
- LLVM Toolchain built (`make toolchain-llvm`)
- Spike Simulator built (`make riscv-isa-sim`)

## Configuration Options

Ara supports several predefined configurations in the `config/` folder:
- `2_lanes` - 2 vector lanes
- `4_lanes` - 4 vector lanes  
- `8_lanes` - 8 vector lanes
- `16_lanes` - 16 vector lanes
- `default` - Default configuration

## Method 1: Single-Core RTL Simulation (Fastest for Cycle Counting)

### IMPORTANT: Setup Hardware Dependencies First

Before running any RTL simulation, you must initialize the hardware dependencies:

```bash
# Navigate to hardware directory
cd hardware/

# Initialize git submodules
make git-submodules

# Checkout required repositories
make checkout

# Build simulation dependencies
make deps

# Build Verilator (recommended over QuestaSim/ModelSim)
make verilator
```

### Basic Configuration Testing

Test different lane configurations using RTL simulation:

```bash
# Navigate to ara root directory
cd /ssd_scratch/vedant.pahariya/ara

# Test 2-lane configuration
config=2_lanes scripts/benchmark.sh fmatmul

# Test 4-lane configuration  
config=4_lanes scripts/benchmark.sh fmatmul

# Test 8-lane configuration
config=8_lanes scripts/benchmark.sh fmatmul

# Test 16-lane configuration
config=16_lanes scripts/benchmark.sh fmatmul
```

### Alternative: Manual Steps (If benchmark.sh Fails)

If the automated script fails, use manual steps:

```bash
# 1. Build the application
cd apps/
config=4_lanes make bin/fmatmul

# 2. Generate benchmark data
python3 fmatmul/script/gen_data.py 128 128 128 > benchmarks/data/data.S

# 3. Build benchmarks
config=4_lanes ENV_DEFINES="-DFMATMUL=1" make bin/benchmarks

# 4. Run simulation manually
cd ../hardware/
config=4_lanes make simv app=benchmarks

# Check results
grep "hw-cycles\|performance" build/ara_tb.log
```

### Manual RTL Simulation Steps

For more control over the simulation process:

```bash
# Generate benchmark data
python3 apps/fmatmul/script/gen_data.py 128 128 128 > apps/benchmarks/data/data.S

# Build benchmarks with specific configuration
config=4_lanes ENV_DEFINES="-DFMATMUL=1" make -C apps/ bin/benchmarks

# Run RTL simulation with ModelSim
config=4_lanes make -C hardware/ sim app=benchmarks

# OR run with Verilator (faster)
config=4_lanes make -C hardware/ simv app=benchmarks
```

### Using Ideal Dispatcher (Ara Only, No CVA6 Overhead)

For benchmarking pure Ara performance without CVA6:

```bash
# Run with ideal dispatcher
config=8_lanes make -C hardware/ simv app=benchmarks DEFINES=IDEAL_DISPATCHER
```

## Method 2: SPIKE Simulation (Functional Verification)

### Basic SPIKE Testing

```bash
# Navigate to apps directory
cd apps

# Build for different configurations
config=2_lanes make bin/fmatmul
config=2_lanes make bin/fmatmul.spike

# Run SPIKE simulation
config=2_lanes make spike-run-fmatmul

# Check results in spike_runs folder
cat spike_runs/spike-run-fmatmul
```

### Batch SPIKE Testing

```bash
# Test multiple configurations with SPIKE
for config in 2_lanes 4_lanes 8_lanes 16_lanes; do
  echo "=== Testing $config with SPIKE ==="
  config=$config make bin/fmatmul
  config=$config make bin/fmatmul.spike  
  config=$config make spike-run-fmatmul
  echo "Results saved to spike_runs/spike-run-fmatmul"
  echo ""
done
```

## Method 3: Multicore Configuration (Using Cheshire)

### Setup Cheshire for Multicore

```bash
# Clone Cheshire as parent project
git clone git@github.com:pulp-platform/cheshire.git
cd cheshire
git checkout main  # or specific commit
bender checkout
ARA_ROOT=$(bender path ara)
cd ${ARA_ROOT}
```

### Apply Multicore Patches

```bash
# Apply hardware patches for multicore support
cd cheshire
make patch_cheshire_high_perf_hw

# Apply software patches
make -C sw patch_cheshire_high_perf_sw
```

### Configure for Multicore RTL Simulation

```bash
# Set 2-lane configuration (for 8-core x 2-lane setup)
export ARA_CONFIGURATION=2_lanes

# Update simulation sources
make -B update_vsim_src

# Build the complete system
make ara-chs-all
```

### Run Multicore RTL Simulation

```bash
# Navigate to Cheshire simulation directory
cd ${BACKREF_CHS_ROOT}/target/sim/vsim/

# Run RISC-V tests in multicore environment
make run-riscv-tests

# OR run specific benchmarks
# (This may require additional configuration)
```

## Method 4: FPGA Deployment (Full System)

### Build Linux Image with Benchmarks

```bash
cd ${ARA_ROOT}/cheshire/sw

# Build specific benchmark for Linux
make fmatmul-linux

# Generate complete Linux image
make linux-img

# Generate Cheshire's Linux image
cd ${ARA_ROOT}/cheshire
make ara-chs-image
```

### Generate FPGA Bitstream

```bash
cd ${ARA_ROOT}/cheshire

# Generate FPGA bitstream
make ara-chs-xilinx

# Flash FPGA (if hardware available)
make ara-chs-xilinx-flash

# Program FPGA
make ara-chs-xilinx-program
```

## Comparative Analysis Scripts

### Single-Core Configuration Comparison

```bash
#!/bin/bash
# Compare different single-core configurations

echo "=== Single-Core Configuration Comparison ==="
cd /ssd_scratch/vedant.pahariya/ara

for config in 2_lanes 4_lanes 8_lanes 16_lanes; do
  echo "Testing $config configuration..."
  config=$config scripts/benchmark.sh fmatmul > results_${config}_single.log 2>&1
  
  # Extract performance metrics
  cycles=$(grep "execution took.*cycles" results_${config}_single.log)
  performance=$(grep "performance is.*FLOP/cycle" results_${config}_single.log)
  
  echo "Config: $config"
  echo "$cycles"
  echo "$performance"
  echo "---"
done
```

### Paper Replication: Single vs Multicore

```bash
#!/bin/bash
# Replicate paper analysis: 1x16-lane vs 8x2-lane

echo "=== Paper Configuration Comparison ==="

# Single-core 16-lane configuration
echo "Testing Single-core 16-lane..."
config=16_lanes scripts/benchmark.sh fmatmul > results_single_16lane.log 2>&1

# Multicore 8x2-lane configuration (requires Cheshire setup)
echo "Testing Multicore 8x2-lane..."
# This requires the Cheshire multicore setup from Method 3
# Results would come from Cheshire simulation environment

echo "Results:"
echo "Single-core 16-lane:"
grep "execution took.*cycles\|performance is.*FLOP/cycle" results_single_16lane.log

echo ""
echo "Multicore 8x2-lane:"
echo "(Results from Cheshire multicore simulation)"
```

## Custom Configuration Testing

### Create Custom Configuration

```bash
# Create custom configuration file
cat > config/custom_6lanes.mk << EOF
# Custom 6-lane configuration
nr_lanes    := 6
vlen        := 4096
EOF

# Test custom configuration
config=custom_6lanes scripts/benchmark.sh fmatmul
```

### Environment Variable Method

```bash
# Set configuration via environment variable
export ARA_CONFIGURATION=8_lanes

# Run benchmarks
scripts/benchmark.sh fmatmul
make -C hardware/ simv app=benchmarks
```

## Available Benchmarks

Test different applications beyond `fmatmul`:

```bash
# Available benchmarks in ara/apps/:
# - hello_world
# - fdotproduct  
# - fmatmul
# - dotproduct
# - fconv2d
# - dropout
# - exp
# - jacobi2d
# - softmax

# Example: Test fdotproduct with different configurations
for config in 4_lanes 8_lanes 16_lanes; do
  config=$config scripts/benchmark.sh fdotproduct
done
```

## Results Analysis

### Check RTL Simulation Results

```bash
# RTL simulation provides cycle-accurate results
grep "hw-cycles" hardware/build/ara_tb.log
grep "performance" hardware/build/ara_tb.log
```

### Check SPIKE Results

```bash
# SPIKE provides functional verification (no cycle counts)
ls apps/spike_runs/
cat apps/spike_runs/spike-run-fmatmul
```

### Expected Output Format

RTL simulation results typically show:
```
[hw-cycles]: 12543
execution took 12543 cycles
performance is 2.15 FLOP/cycle (26.9% utilization)
```

## Troubleshooting

### Common Issues

1. **Build Errors**: Ensure toolchain is properly built
2. **Configuration Not Found**: Check `config/` folder for available configurations
3. **Simulation Hangs**: Try using Verilator instead of ModelSim (`simv` vs `sim`)
4. **Cheshire Setup**: Ensure proper git submodule initialization
5. **QuestaSim/ModelSim Issues**: Missing simulator in PATH
6. **Missing Dependencies**: DPI files and simulation infrastructure not built

### Specific Error Solutions

#### Error: "Specified QuestaSim version not found in PATH"
```bash
# Solution 1: Use Verilator instead (recommended)
make -C hardware/ verilator  # Build Verilator first
config=4_lanes make -C hardware/ simv app=benchmarks

# Solution 2: Install QuestaSim/ModelSim (if available)
# Add QuestaSim to PATH or update hardware/Makefile
```

#### Error: "No rule to make target 'tb/dpi/elfloader.cc'"
```bash
# Missing simulation dependencies - build them first
cd hardware/
make deps
make build-deps

# Or clean and rebuild everything
make clean-deps
make deps
```

#### Error: "find: 'deps': No such file or directory"
```bash
# Initialize hardware dependencies
cd hardware/
make checkout
make deps

# Then try the simulation again
config=4_lanes make simv app=benchmarks
```

### Debug Commands

```bash
# Check available configurations
ls config/

# Verify toolchain
which riscv64-unknown-elf-gcc

# Initialize hardware dependencies (IMPORTANT)
cd hardware/
make checkout      # Checkout required repositories
make deps          # Build simulation dependencies

# Check simulation build
make -C hardware/ clean
make -C hardware/ simv app=benchmarks config=4_lanes -n  # dry run

# Alternative: Use Verilator (more reliable)
make -C hardware/ verilator  # Build Verilator first
config=4_lanes make -C hardware/ simv app=benchmarks
```

### Step-by-Step Fix for Your Current Error

Based on your error, follow these steps in order:

```bash
# 1. Navigate to hardware directory
cd /ssd_scratch/vedant.pahariya/ara/hardware

# 2. Initialize git submodules (critical step)
make git-submodules

# 3. Checkout dependencies
make checkout

# 4. Build simulation dependencies  
make deps

# 5. Build Verilator (recommended simulator)
make verilator

# 6. Now try the benchmark again
cd ..
config=4_lanes scripts/benchmark.sh fmatmul
```

## Performance Metrics

When comparing configurations, focus on:
- **Cycles**: Total execution cycles
- **FLOP/cycle**: Floating-point operations per cycle
- **Utilization**: Percentage of theoretical peak performance
- **Memory Bandwidth**: Memory access patterns and efficiency

---

**Note**: For multicore analysis, the Cheshire setup is essential. For single-core performance analysis, direct RTL simulation is sufficient and faster.

<!----------------------------------------------------------------------------------------------->
## Summary of Current Setup Status

✅ **Working:**
- LLVM Toolchain: Built successfully
- SPIKE Simulator: Built and working
- Application Building: Applications build successfully for different configurations
- SPIKE Simulation: Functional verification working with performance metrics

❌ **Not Working:**
- Verilator Build: C++ compiler issues prevent Verilator compilation
- RTL Simulation: Cannot run cycle-accurate RTL simulation without Verilator
- QuestaSim/ModelSim: Not available in current environment

## Working Alternative: SPIKE Simulation for Configuration Testing

Since Verilator build is failing, you can still perform configuration testing using SPIKE simulation:

### Test Different Configurations with SPIKE

# Navigate to apps directory
cd /ssd_scratch/vedant.pahariya/ara/apps

# Test different lane configurations
for config in 2_lanes 4_lanes 8_lanes 16_lanes; do
  echo "=== Testing $config with SPIKE ==="
  config=$config make bin/fmatmul.spike
  config=$config make spike-run-fmatmul
  echo "Results saved to spike_runs/spike-run-fmatmul"
  echo ""
done
```

### SPIKE Results Analysis

SPIKE provides functional verification and performance metrics (though not cycle-accurate like RTL):
- Execution cycles for different matrix sizes
- FLOP/cycle performance
- Utilization percentages
- Correctness verification

**Example Output (4-lane configuration):**
```
The execution took 49949 cycles.
The performance is 83.971 FLOP/cycle (1049.646% utilization).
Passed.