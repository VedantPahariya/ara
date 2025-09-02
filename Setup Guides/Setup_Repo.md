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
<file_name> = x;
make bin/<file_name>
make bin/<file_name>.spike
make spike-run-<file_name>
```

**Example filenames:** `hello_world`, `fdotproduct`, `fmatmul`

**Note:** Check the `ara/apps` directory for available functions and applications. Name of the folder is `file_name`

make bin/fmatmul
make bin/fmatmul.spike
make spike-run-fmatmul

For Results:
Check folder 'spike_runs' under apps for the output of above files.