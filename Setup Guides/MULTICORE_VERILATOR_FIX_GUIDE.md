# Problem Summary & Files Needed for Multicore Verilator Fix

**Date Created:** October 20, 2025  
**Project:** Ara RISC-V Vector Processor  
**Issue:** Verilator testbench fails with multicore configuration

---

## Problem Statement

### Error Description

When running `app=hello_world make simv` with multicore configuration, the Verilator testbench fails with:

```
ERROR: No memory found at `TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[0].l2_mem.sram' 
(the scope associated with region `ram').
```

### Complete Error Output

```
Makefile:94: "Specified QuestaSim version (questa-2021.2) not found in PATH ..."
build/verilator/Vara_tb_verilator  -l ram,/ssd_scratch/ara/apps/bin/hello_world,elf
Program header number 0 in `/ssd_scratch/ara/apps/bin/hello_world' low is 80000000
Program header number 0 in `/ssd_scratch/ara/apps/bin/hello_world' high is 80000fcd
Program header number 1 in `/ssd_scratch/ara/apps/bin/hello_world' high is 800013bf
Program header number 2 in `/ssd_scratch/ara/apps/bin/hello_world' high is 800013e7
Program header number 3 in `/ssd_scratch/ara/apps/bin/hello_world' is not of type PT_LOAD; ignoring.
Set `ram TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[0].l2_mem.sram 20 0x80000000 0x100000 write with offset: 0x0 write with size: 0x13e8
ERROR: No memory found at `TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[0].l2_mem.sram' (the scope associated with region `ram').
```

### Root Cause Analysis

**Primary Issue:** The Verilator testbench (`tb/verilator/ara_tb.cpp`) and memory loading utilities are hardcoded for single-core memory hierarchy. The multicore configuration changes the memory organization, but the Verilator testbench hasn't been updated accordingly.

**Technical Details:**
- The memory path used for loading the ELF binary doesn't match the actual multicore hardware hierarchy
- Single-core testbench works perfectly (QuestaSim via `make sim`)
- Multicore QuestaSim also works (uses SystemVerilog DPI-based initialization)
- Only Verilator with multicore configuration is broken

**Key Insight:** The Verilator testbench uses a C++-based memory loading mechanism that explicitly references hardware hierarchy paths, which differ between single-core and multicore configurations.

---

## Files Involved

### 1. Core Files to Modify (High Priority)

#### Hardware Testbench Files:

| File Path | Description | Action Needed |
|-----------|-------------|---------------|
| `hardware/tb/verilator/ara_tb.cpp` | Main Verilator C++ testbench that loads ELF and sets up memory regions | **PRIMARY FIX** - Update memory path and loading logic |
| `hardware/tb/ara_tb.sv` | SystemVerilog testbench (reference implementation) | Reference - Shows working multicore memory init via DPI |
| `hardware/tb/verilator/ara_tb_verilator.sv` | Verilator-specific top-level wrapper | May need hierarchy updates |

#### Memory Loading Utilities:

| File Path | Description | Action Needed |
|-----------|-------------|---------------|
| `hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/cpp/*.cc` | DPI memory utilities | Check memory region registration |
| `hardware/tb/verilator/lowrisc_dv_verilator_memutil_verilator/cpp/*.cc` | Verilator memory interface | Verify scope resolution |
| `hardware/tb/verilator/simutil_stubs.cc` | Simulation utility stubs | May need updates |

### 2. Hardware Design Files (For Understanding Hierarchy)

#### Top-Level and SoC:

| File Path | Description | Importance |
|-----------|-------------|------------|
| `hardware/src/ara_soc.sv` | Main SoC that instantiates cores and memory banks | **CRITICAL** - Check memory instantiation with MULTI_CORE |
| `hardware/src/ara_system.sv` | Single Ara system instantiation | Understand core structure |
| `hardware/src/ara_testharness.sv` | Test harness wrapper | Top-level connections |

#### Memory Configuration:

| File Path | Description | Importance |
|-----------|-------------|------------|
| `hardware/src/tech_cells_generic_mem/rtl/tc_sram.sv` | SRAM model used for L2 banks | Understand memory interface |

### 3. Configuration Files

| File Path | Description | Key Parameters |
|-----------|-------------|----------------|
| `config/default.mk` | Single core configuration | `nr_cores=1`, baseline memory setup |
| `config/default_mc.mk` | Multicore configuration | `nr_cores=4`, multicore memory layout |
| `hardware/Makefile` | Build system | Line 217: `simv` target definition |

### 4. Build System Files

| File Path | Description | Relevance |
|-----------|-------------|-----------|
| `Bender.yml` | Dependency and file list management | Defines verilator target files |
| `hardware/Makefile` | Verilator build rules | Lines 182-215: verilate target with memory loading |

---

## What Works vs What Doesn't

### ✅ Working: QuestaSim (`make sim`)

**Configuration:** Both single-core and multicore  
**Testbench:** Uses SystemVerilog testbench (`hardware/tb/ara_tb.sv`)  
**Memory Init Method:** DPI functions in loop (lines 127-157 in `ara_tb.sv`)

**Key Code (ara_tb.sv lines 127-157):**
```systemverilog
for (genvar bank = 0; bank < L2NumBanks; bank++) begin : gen_l2_banks_init
  initial begin : dram_init
    // ... ELF loading code ...
    for (int w = 0; w < nwords; w++) begin
      mem_row = '0;
      for (int b = 0; b < L2BankBeWidth; b++) begin
        mem_row[8 * b +: 8] = buffer[(bank + w * L2NumBanks) * L2BankBeWidth + b];
      end
      if (address >= DRAMAddrBase && address < DRAMAddrBase + DRAMLength) begin
        dut.i_ara_soc.gen_l2_banks[bank].l2_mem.init_val[...] = mem_row;
      end
    end
  end
end
```

**Why It Works:** 
- Uses generate loop to iterate over all banks
- Direct SystemVerilog hierarchy access
- `L2NumBanks = NrAraSystems` adapts to configuration

### ❌ Broken: Verilator (`make simv`)

**Configuration:** Only multicore (single-core works fine)  
**Testbench:** Uses C++ testbench (`tb/verilator/ara_tb.cpp`)  
**Memory Init Method:** Memory loading via `-l ram,<elf_path>,elf` command-line argument

**Attempted Memory Path:**
```
TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[0].l2_mem.sram
```

**Why It Fails:**
- Hardcoded path assumes single-core hierarchy
- Path doesn't exist or changed in multicore configuration
- Only tries to load bank[0], doesn't iterate over all banks
- C++ memory utilities can't resolve the hierarchical path

---

## Memory Hierarchy Analysis

### Single Core Configuration (`NrAraSystems=1`)

**Parameter Values (ara_tb.sv):**
```systemverilog
localparam NrAraSystems = 1;
localparam L2NumBanks = NrAraSystems;  // = 1
```

**Expected Hierarchy:**
```
ara_soc
  └── gen_l2_banks[0]
        └── l2_mem
              └── sram (or init_val)
```

**Memory Organization:**
- 1 system, 1 memory bank
- Simple direct path access
- Works with Verilator's hardcoded path

### Multicore Configuration (`NrAraSystems=4`)

**Parameter Values (ara_tb.sv):**
```systemverilog
localparam NrAraSystems = 4;
localparam L2NumBanks = NrAraSystems;  // = 4
```

**Expected Hierarchy:**
```
ara_soc
  ├── gen_l2_banks[0]
  │     └── l2_mem.sram
  ├── gen_l2_banks[1]
  │     └── l2_mem.sram
  ├── gen_l2_banks[2]
  │     └── l2_mem.sram
  ├── gen_l2_banks[3]
  │     └── l2_mem.sram
  └── gen_ara_systems[0..3]  // 4 Ara cores
```

**Memory Organization:**
- 4 systems, 4 memory banks
- Banks might be interleaved or partitioned
- Each bank needs separate initialization
- Single hardcoded path only hits bank[0], misses others

### Key Parameters from ara_tb.sv (lines 48-61)

```systemverilog
localparam int unsigned L2NumWords = 2**20;        // Total L2 words
localparam int unsigned L2NumBanks = NrAraSystems; // Banks = # cores

// Derived parameters
localparam int unsigned L2BankNumWords   = L2NumWords / L2NumBanks;
localparam int unsigned L2BankAddrWidth  = $clog2(L2BankNumWords);
localparam int unsigned L2BankWidth      = AxiWideDataWidth;
localparam int unsigned L2BankBeWidth    = L2BankWidth/8;
localparam int unsigned L2BeWidth        = L2BankBeWidth * L2NumBanks;
localparam int unsigned L2BankSize       = L2BankNumWords * L2BankBeWidth;

localparam integer unsigned L2Width          = L2BankWidth * L2NumBanks;
localparam integer unsigned L2ByteOffset     = $clog2(L2BeWidth);
```

---

## Expected Solution Areas

### 1. Update Memory Region Specification in Verilator Testbench

**File:** `tb/verilator/ara_tb.cpp`

**Current Approach (Single-Core):**
```cpp
// Pseudocode - hardcoded single bank
memory_region("ram", "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[0].l2_mem.sram", ...);
```

**Required Approach (Multicore):**
```cpp
// Iterate over all banks
for (int bank = 0; bank < NR_ARA_SYSTEMS; bank++) {
  char region_name[64];
  char hierarchy_path[256];
  snprintf(region_name, sizeof(region_name), "ram_bank%d", bank);
  snprintf(hierarchy_path, sizeof(hierarchy_path), 
           "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem.sram", bank);
  memory_region(region_name, hierarchy_path, ...);
}
```

**Key Changes Needed:**
- Loop over all L2 banks (4 for multicore)
- Dynamic path generation based on bank index
- Interleaved memory loading (words distributed across banks)
- Pass NR_CORES from compile-time define

### 2. Verify Memory Instantiation in ara_soc.sv

**File:** `hardware/src/ara_soc.sv`

**Check For:**
```systemverilog
for (genvar i = 0; i < L2NumBanks; i++) begin : gen_l2_banks
  tc_sram #(
    .NumWords (L2BankNumWords ),
    .DataWidth(L2BankWidth    ),
    .NumPorts (L2_NR_PORTS    )
  ) l2_mem (
    .clk_i  (clk_i),
    .rst_ni (rst_ni),
    .req_i  (...),
    .we_i   (...),
    .addr_i (...),
    .wdata_i(...),
    .be_i   (...),
    .rdata_o(...)
  );
end
```

**Questions to Answer:**
1. Is the generate loop consistent between single/multicore?
2. Does `MULTI_CORE` define change memory instantiation?
3. Are there additional memory hierarchy levels in multicore?
4. What's the exact module instance path in generated code?

### 3. Update Verilator Memory Loading Logic

**Files Affected:**
- `tb/verilator/ara_tb.cpp` (main)
- `tb/verilator/lowrisc_dv_verilator_memutil_dpi/cpp/*.cc` (utilities)

**Key Considerations:**

#### Address Interleaving
Words must be distributed across banks:
```
Word 0 -> Bank 0
Word 1 -> Bank 1
Word 2 -> Bank 2
Word 3 -> Bank 3
Word 4 -> Bank 0  (cycle repeats)
...
```

#### ELF Loading Strategy
```
For each ELF section:
  For each word in section:
    bank_index = (word_offset) % NUM_BANKS
    bank_addr = (word_offset) / NUM_BANKS
    Load word to: gen_l2_banks[bank_index] at bank_addr
```

#### Command-Line Arguments
Current: `-l ram,<elf>,elf`  
May need: `-l ram_bank0,<elf>,elf:0:4 -l ram_bank1,<elf>,elf:1:4 ...`

Or implement smart parsing in C++ to handle banking automatically.

---

## Debugging Commands

### 1. Build and Inspect Multicore Verilator Hierarchy

```bash
cd hardware
make verilate config=default_mc
```

### 2. Search for Memory Instances in Generated Files

```bash
# Find all l2_mem references
grep -r "l2_mem" build/verilator/*.h build/verilator/*.cpp | head -20

# Find bank generation
grep -r "gen_l2_banks" build/verilator/ | grep "sram"

# Check memory hierarchy
find build/verilator -name "*__Syms.h" -exec grep -A10 "l2_mem" {} \;
```

### 3. Compare Single vs Multicore Memory Paths

```bash
# Build single-core
make verilate config=default

# Compare hierarchies
diff <(grep -r "gen_l2_banks" build/verilator/ | grep sram) \
     <(cd ../hardware_mc && grep -r "gen_l2_banks" build/verilator/ | grep sram)
```

### 4. Inspect Verilator Symbol Table

```bash
# Check available memory scopes
cat build/verilator/Vara_tb_verilator__Syms.h | grep -A5 "l2_mem"
cat build/verilator/Vara_tb_verilator__Syms.cpp | grep -A5 "l2_mem"
```

### 5. Verify Memory Banking in Waveforms

```bash
# Run with trace enabled
make simv app=hello_world trace=1

# Open resulting .fst file and check:
# - gen_l2_banks[0..3] all exist
# - Memory accesses distributed across banks
```

---

## Files to Provide to AI for Complete Solution

### Minimum Required Set

1. **`hardware/tb/verilator/ara_tb.cpp`**  
   - Main file to fix
   - Contains memory loading logic
   - **ACTION:** Read and provide full file

2. **`hardware/tb/ara_tb.sv`**  
   - Reference working implementation (QuestaSim)
   - Shows correct multicore bank iteration
   - **ACTION:** Already provided, lines 127-157 most relevant

3. **`hardware/src/ara_soc.sv`**  
   - Memory hierarchy definition
   - Shows how gen_l2_banks is instantiated
   - **ACTION:** Read and provide memory instantiation section

4. **`config/default_mc.mk`**  
   - Multicore parameters (nr_cores, nr_lanes, etc.)
   - **ACTION:** Read and provide

5. **Error output from `make simv`**  
   - **STATUS:** Already provided above

6. **Output from debug commands**  
   - Grep results showing actual memory paths
   - **ACTION:** Run commands and capture output

### Helpful Additional Context

7. **`hardware/Makefile`**  
   - Build system, verilate target (lines 182-215)
   - Shows how ara_tb.cpp is compiled
   - **ACTION:** Already provided

8. **`hardware/tb/verilator/lowrisc_dv_verilator_memutil_dpi/cpp/*.cc`**  
   - Memory utility implementation
   - **ACTION:** Read if memory region registration needs changes

9. **Generated `build/verilator/Vara_tb_verilator__Syms.h`**  
   - Hierarchy map after Verilator elaboration
   - Shows actual available memory paths
   - **ACTION:** Generate and provide after building

10. **`hardware/tb/verilator/ara_tb_verilator.sv`**  
    - Verilator wrapper
    - May need to expose bank parameters
    - **ACTION:** Read and provide if hierarchy unclear

---

## Solution Checklist

### Phase 1: Investigation
- [ ] Run `make verilate config=default_mc`
- [ ] Grep for memory paths in `build/verilator/`
- [ ] Compare single vs multicore generated hierarchy
- [ ] Identify exact memory instance paths
- [ ] Check if banks exist but path is wrong, or banks don't exist at all

### Phase 2: Testbench Updates
- [ ] Modify `tb/verilator/ara_tb.cpp`:
  - [ ] Add NR_CORES parameter handling
  - [ ] Loop over all L2 banks
  - [ ] Dynamic memory region name generation
  - [ ] Dynamic hierarchy path generation
  - [ ] Implement bank-interleaved loading
- [ ] Update memory utility calls if needed
- [ ] Add compile-time checks for bank count

### Phase 3: Build System Updates
- [ ] Ensure `-DNR_CORES=$(nr_cores)` passed to Verilator C++ compiler
- [ ] Verify `-DNR_LANES=$(nr_lanes)` still works
- [ ] Update Makefile if needed

### Phase 4: Verification
- [ ] Build: `make verilate config=default_mc`
- [ ] Run: `make simv app=hello_world config=default_mc`
- [ ] Verify all banks loaded correctly
- [ ] Check simulation completes successfully
- [ ] Test with multiple apps (matmul, conv2d, etc.)
- [ ] Verify single-core still works: `make simv config=default`

### Phase 5: Documentation
- [ ] Add comments explaining bank-interleaved loading
- [ ] Update any relevant READMEs
- [ ] Document multicore Verilator support

---

## Key Insights from Working QuestaSim Implementation

### From ara_tb.sv (lines 127-157):

**Memory Bank Iteration:**
```systemverilog
for (genvar bank = 0; bank < L2NumBanks; bank++) begin : gen_l2_banks_init
```
- Uses generate loop, not runtime loop
- Each bank gets own initial block

**Bank-Interleaved Loading:**
```systemverilog
for (int b = 0; b < L2BankBeWidth; b++) begin
  mem_row[8 * b +: 8] = buffer[(bank + w * L2NumBanks) * L2BankBeWidth + b];
end
```
- Formula: `(bank + word_num * L2NumBanks) * L2BankBeWidth + byte`
- Distributes sequential words across banks in round-robin

**Memory Write:**
```systemverilog
dut.i_ara_soc.gen_l2_banks[bank].l2_mem.init_val[(address - DRAMAddrBase + (w << L2ByteOffset)) >> L2ByteOffset] = mem_row;
```
- Direct hierarchy access: `gen_l2_banks[bank].l2_mem.init_val`
- Address calculation uses L2ByteOffset

**This Must Be Replicated in C++ for Verilator!**

---

## Additional Notes

### Verilator vs QuestaSim Differences

| Aspect | QuestaSim | Verilator |
|--------|-----------|-----------|
| Language | SystemVerilog | C++ |
| Memory Access | Direct hierarchy access | Via DPI memory utilities |
| Initialization | Generate loops at elaboration | Runtime C++ loops |
| Path Resolution | Compile-time | Must match runtime hierarchy |

### Potential Pitfalls

1. **Hardcoded Indices:** Any `[0]` in paths must become `[bank]` in loops
2. **Word Distribution:** Must implement bank interleaving, not sequential loading
3. **Address Calculation:** Bank-local addresses vs global addresses
4. **Parameter Passing:** NR_CORES must reach C++ code from Makefile
5. **Path Strings:** C++ string formatting for array indices in paths

### Similar Issues in Codebase

Check if other Verilator testbench features are also broken:
- VCD dumping with multicore
- Performance counter access
- VLSU signal probing (line 174 in ara_tb.sv)

---

## Contact & References

**Original Working Code:** `hardware/tb/ara_tb.sv` (QuestaSim testbench)  
**Broken Code:** `hardware/tb/verilator/ara_tb.cpp` (Verilator testbench)  
**Build System:** `hardware/Makefile` (line 217 for simv target)  

**Key Makefile Lines:**
- Line 217: `simv` target definition
- Lines 182-215: `verilate` target with all compiler flags
- Line 48: `-CFLAGS "-DNR_LANES=$(nr_lanes)"` (add NR_CORES similar)

---

## Summary

The multicore Verilator testbench is broken because:
1. Memory loading code assumes single bank at hardcoded path
2. Multicore has 4 banks that need bank-interleaved loading
3. C++ code doesn't iterate over banks like working SystemVerilog code does

**Primary Fix:** Update `tb/verilator/ara_tb.cpp` to:
- Loop over all banks (1 for single-core, 4 for multicore)
- Generate correct hierarchical paths dynamically
- Implement bank-interleaved word distribution
- Use NR_CORES parameter from build system

**Reference:** The working QuestaSim code in `ara_tb.sv` lines 127-157 shows exactly how to do bank-interleaved loading correctly.
