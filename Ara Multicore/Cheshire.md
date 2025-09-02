# Cheshire in Ara: Multicore Vector Processing Platform

## Overview

**Cheshire** is a complete System-on-Chip (SoC) platform developed by PULP Platform that serves as the foundational infrastructure for deploying Ara vector processors in both single-core and multicore configurations. This document explains Cheshire's role within the Ara repository and its critical importance for multicore vector processing research.

## What is Cheshire?

Cheshire is a **complete SoC platform** that provides:
- CVA6 RISC-V cores with RV64GC support
- Memory subsystem with caches and interconnects
- Peripheral interfaces and I/O controllers
- Linux boot capability
- FPGA deployment infrastructure

In the context of Ara, Cheshire acts as the **host system** that integrates multiple CVA6 cores, each coupled with an Ara vector processing unit.

## Cheshire's Role in Ara Repository

### 1. **FPGA Deployment Platform** 🔧
- **Purpose**: Deploy Ara on real FPGA hardware (VCU128/VCU118)
- **Function**: Provides complete SoC infrastructure needed for hardware implementation
- **Not needed for**: RTL simulation or functional verification

### 2. **Linux and Operating System Support** 🐧
- **Purpose**: Boot Linux with RISC-V Vector (RVV) extension support
- **Function**: Enables running complex vector applications on real hardware
- **Capability**: Run RVV-enabled Linux with vector kernels

### 3. **Multicore Configuration Infrastructure** 🏗️
- **Purpose**: Enable multicore vector processing research
- **Function**: Configure and deploy multiple CVA6+Ara pairs
- **Research Use**: Compare single-core vs multicore vector architectures

## Architecture Integration

```
┌─────────────────────────────────────────────────────┐
│                 Cheshire SoC                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │
│  │CVA6 Core #1 │  │CVA6 Core #2 │  │CVA6 Core #N │  │
│  │     +       │  │     +       │  │     +       │  │
│  │ Ara Vector  │  │ Ara Vector  │  │ Ara Vector  │  │
│  │   Unit      │  │   Unit      │  │   Unit      │  │
│  └─────────────┘  └─────────────┘  └─────────────┘  │
│                                                     │
│  Memory Subsystem | I/O | Interconnect             │
└─────────────────────────────────────────────────────┘
```

## Key Use Cases

### ✅ **When You NEED Cheshire:**
- **FPGA Implementation**: Deploying Ara on physical hardware
- **Linux Deployment**: Running RVV applications on Linux
- **Multicore Research**: Comparing different core/lane configurations
- **Real Hardware Benchmarking**: Performance on actual silicon/FPGA

### ❌ **When You DON'T Need Cheshire:**
- **RTL Simulation**: Cycle-accurate performance analysis
- **Functional Verification**: Testing Ara's vector instructions
- **Basic Benchmarking**: Getting clock cycles for applications
- **SPIKE Simulation**: ISA-level functional testing

## Multicore Configuration in Research

### Single-Core Configuration (Paper Reference)
- **Setup**: 1 CVA6 core + 1 Ara unit (16 lanes)
- **Configuration**: `ARA_CONFIGURATION=16_lanes`
- **Total Vector Capability**: 16 lanes

### Multicore Configuration (Paper Reference)
- **Setup**: 8 CVA6 cores + 8 Ara units (2 lanes each)
- **Configuration**: `ARA_CONFIGURATION=2_lanes` + Cheshire multicore patches
- **Total Vector Capability**: 8 × 2 = 16 lanes
- **Research Question**: Distributed vs. centralized vector processing

## How Cheshire Works in Ara Repository

### 1. **Hardware Integration**
```makefile
# Configure Ara with specific lane count
ARA_CONFIGURATION ?= 2_lanes

# Set up CVA6 + Ara integration targets
COMMON_CUSTOM_TARGETS := -t cv64a6_imafdcv_sv39 -t cva6 -t rtl \
                        --define ARA --define NR_LANES=$(nr_lanes) \
                        --define VLEN=$(vlen)
```

### 2. **System Patches**
```bash
# Apply multicore patches to Cheshire
make patch_cheshire_high_perf_hw    # Hardware configuration
make patch_cheshire_high_perf_sw    # Software configuration
```

### 3. **FPGA Deployment**
```bash
# Generate custom TCL for FPGA synthesis
make update_xilinx_src

# Build complete system
make ara-chs-xilinx     # FPGA bitstream
make ara-chs-image      # Linux image
```

## Practical Workflow

### For Multicore Research:
1. **Configure**: Set lane count per core (`2_lanes` for 8-core setup)
2. **Patch**: Apply Cheshire patches for multicore support
3. **Build**: Generate FPGA bitstream with integrated system
4. **Deploy**: Run on FPGA hardware with Linux support
5. **Benchmark**: Compare different configurations

### For Performance Analysis Only:
1. **Skip Cheshire**: Use direct RTL simulation
2. **Use**: `scripts/benchmark.sh` for cycle-accurate results
3. **Analyze**: Compare configurations in simulation

## Files and Structure

```
ara/cheshire/
├── Makefile              # Build system for Cheshire integration
├── README.md             # Usage instructions
├── patches/              # System patches for multicore support
└── sw/                   # Software stack (Linux, buildroot, etc.)
    ├── cva6-sdk/         # CVA6 SDK with Linux support
    ├── Makefile          # Software build system
    └── include/          # Headers for Cheshire integration
```

## Important Notes

### Limitations
- **Testing**: Only 2-lane Ara configuration thoroughly tested with Cheshire
- **Complexity**: Cheshire adds significant build complexity
- **Dependencies**: Requires FPGA tools and Linux build environment

### Performance Considerations
- **RTL Simulation**: Faster and sufficient for most benchmarking
- **FPGA Deployment**: Required for real-world performance validation
- **Multicore Analysis**: Cheshire essential for comparing architectures

## Research Applications

This Cheshire integration enables research questions like:
- **Scalability**: How does vector performance scale with core count?
- **Memory Bandwidth**: How do multiple cores share memory resources?
- **Application Mapping**: Which applications benefit from distributed vs. centralized processing?
- **Energy Efficiency**: Power characteristics of different configurations

## Getting Started

For FPGA deployment and multicore research:
```bash
# 1. Set up Cheshire as parent project
git clone git@github.com:pulp-platform/cheshire.git
cd cheshire
bender checkout
ARA_ROOT=$(bender path ara)

# 2. Configure and deploy
cd ${ARA_ROOT}
make patch_cheshire_high_perf_hw
make ara-chs-xilinx
```

For performance benchmarking only:
```bash
# Use direct RTL simulation (no Cheshire needed)
cd ara
scripts/benchmark.sh fmatmul
```

---

**Summary**: Cheshire is the SoC platform that enables Ara's deployment on real hardware and Linux systems. While not needed for basic RTL simulation and benchmarking, it's essential for multicore research, FPGA implementation, and complete system deployment.
