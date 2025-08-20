#!/bin/bash

# RISC-V Tests with LLVM Compiler
# This script compiles RISC-V tests using LLVM instead of GCC

LLVM_CC="/ssd_scratch/vedant.pahariya/ara/install/riscv-llvm/bin/clang --target=riscv64-unknown-elf --sysroot=/ssd_scratch/vedant.pahariya/ara/install/riscv-llvm/riscv64-unknown-elf"
LLVM_LDFLAGS="-static -nostartfiles -lm -mcmodel=medany -march=rv64gcv -mabi=lp64d -I/ssd_scratch/vedant.pahariya/ara/apps/common -static -O3 -ffast-math -fno-common -fno-builtin-printf -DNR_LANES=4 -DVLEN=4096 -Wunused-variable -Wall -Wextra -Wno-unused-command-line-argument -std=gnu99 -T/ssd_scratch/vedant.pahariya/ara/apps/common/link.ld"

echo "Building RISC-V tests with LLVM..."
cd /ssd_scratch/vedant.pahariya/ara/apps

make riscv_tests \
    RISCV_CC_GCC="$LLVM_CC" \
    RISCV_LDFLAGS_GCC="$LLVM_LDFLAGS"
