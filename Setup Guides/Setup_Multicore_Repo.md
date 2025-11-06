# Setup Instructions

After cloning this repository, execute the following commands in order:

### 0. Cloning Submodules Recursively
Use the following instead of normal submodule update command to avoid timeout issues:
```bash
git -c submodule.recurse=true submodule update --init --recursive --depth 1
git submodule sync --recursive
```
**Note:** While cloning the submodules, you might encounter errors similar to the ones shown below:

- Just after cloning the branch `feat/multi-core` in the Ara repository, it may give the following error:
```console
  $ git submodule update --init --recursive
  Cloning into '/ssd_scratch/vedant.pahariya/multicore/ara/toolchain/newlib'...
  error: RPC failed; curl 18 transfer closed with outstanding read data remaining
  fetch-pack: unexpected disconnect while reading sideband packet
  fatal: early EOF
  fatal: fetch-pack: invalid index-pack output
  fatal: clone of 'https://sourceware.org/git/newlib-cygwin.git' into submodule path '/ssd_scratch/vedant.pahariya/multicore/ara/toolchain/newlib' failed
  Failed to clone 'toolchain/newlib'. Retry scheduled
  Cloning into '/ssd_scratch/vedant.pahariya/multicore/ara/toolchain/riscv-gnu-toolchain'...
  Cloning into '/ssd_scratch/vedant.pahariya/multicore/ara/toolchain/riscv-isa-sim'...
  Cloning into '/ssd_scratch/vedant.pahariya/multicore/ara/toolchain/riscv-llvm'...
  Cloning into '/ssd_scratch/vedant.pahariya/multicore/ara/toolchain/verilator'...
  Cloning into '/ssd_scratch/vedant.pahariya/multicore/ara/toolchain/newlib'...
  error: RPC failed; curl 18 transfer closed with outstanding read data remaining
  fetch-pack: unexpected disconnect while reading sideband packet
  fatal: early EOF
  fatal: fetch-pack: invalid index-pack output
  fatal: clone of 'https://sourceware.org/git/newlib-cygwin.git' into submodule path '/ssd_scratch/vedant.pahariya/multicore/ara/toolchain/newlib' failed
  Failed to clone 'toolchain/newlib' a second time, aborting
```

  git error "curl 18 transfer closed with outstanding read data remaining" means the HTTP transfer was interrupted by the server/network (timeout, proxy, flaky connection, or source server rate limiting). Above command avoids timeout. 

### 1. Build LLVM Toolchain
```bash
make toolchain-llvm
```
**Note:** If you encounter any errors, refer to the `Toolchain_Setup_Guide.md` for troubleshooting.
Ignore the errors of libgloss in ada

From the Toolchain_Setup_Guide.md, do the setup for following:
Manual Newlib Build
Build Compiler-RT Runtime

### 2. Build Spike Simulator
```bash
make riscv-isa-sim
```
This command builds the Spike RISC-V ISA simulator library.

# Benchmarking

For detailed benchmarking instructions, see the [`benchmarking guide.md`](benchmarking%20guide.md) file.

### Running Applications

To build and run applications:

```bash
file_name=hello_world
make bin/$file_name
make bin/$file_name.spike
make spike-run-$file_name
```

**Example filenames:** `hello_world`, `fdotproduct`, `fmatmul`

**Note:** Check the `ara/apps` directory for available functions and applications. Name of the folder is `file_name`

make bin/fmatmul
make bin/fmatmul.spike
make spike-run-fmatmul

For Results:   
Check folder 'spike_runs' under apps for the output of above files.

### Using Verilator 

Refer [`verilator_setup_fix.md`](verilator_setup_fix.md) for more details.    
For future builds, first make sure these environment variables are set:

```bash
export ARA_ROOT=$(pwd) # Set this to your actual Ara installation path
export LD_LIBRARY_PATH="${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:$LD_LIBRARY_PATH"
export LIBRARY_PATH="${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:$LIBRARY_PATH"
```

For any file you want to run: 
1. Step-1:  make bin/$file_name
   - Here, file_name means any kernel or operation that you want to run/benchmark

2. Step-2: Run the following commands
```bash
cd ${ARA_ROOT}/hardware
app=hello_world make verilate  # Build Verilator simulation
app=hello_world make simv      # Run simulation
```

# Understanding Ara: Behind the Commands

For an in-depth explanation of Ara and its workflow, refer to the [Command Workflow](https://docs.google.com/document/d/1PHaVQt3r2g7qtwZMLI2ezhs9PnT-vSlgOIZjWayBUj4/edit?usp=sharing) document.

This document discusses about the working of following:
- make verilate
- make simv
- ara_tb.sv VS ara_tb.cpp
- Memory Hierarchical Path
- Bankinterleaving