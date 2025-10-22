## Setup Instructions

After cloning this repository, execute the following commands in order:

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

## Benchmarking

For detailed benchmarking instructions, see the `benchmarking guide.md` file.

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

Refer verilator_setup_fix.md for more details    
For future builds, First make sure these environment variables are set:

```bash
export ARA_ROOT=$(pwd) # Set this to your actual Ara installation path
export LD_LIBRARY_PATH="${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:$LD_LIBRARY_PATH"
export LIBRARY_PATH="${ARA_ROOT}/hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/lib:$LIBRARY_PATH"
```

For any file, you want to run. 
1. Step-1:  make bin/$file_name
here file_name means any kernel or operation that you want to run/benchmark

2. Step-2: Run the following commands
```bash
cd ${ARA_ROOT}/hardware
app=hello_world make verilate  # Build Verilator simulation
app=hello_world make simv      # Run simulation
```
