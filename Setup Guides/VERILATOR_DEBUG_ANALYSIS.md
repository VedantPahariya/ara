# Verilator Memory Access Debug Analysis
**Date:** October 21, 2025  
**Issue:** Verilator memory registration succeeds but ELF loading fails

---

## Step-by-Step Diagnostic Results

### ✅ 1. ELF Binary Validation

**File Check:**
```
apps/bin/hello_world: ELF 64-bit LSB executable, UCB RISC-V, RVC, double-float ABI, version 1 (SYSV), statically linked, stripped
```
**Result:** ✅ ELF is valid

**Architecture:**
```
architecture: riscv:rv64, flags 0x00000102:
EXEC_P, D_PAGED
start address: 0x0000000080000000
```
**Result:** ✅ Correct RISC-V 64-bit, starts at 0x80000000

**Memory Sections:**
```
.text    00000fce  0x80000000  (code)
.rodata  000003e0  0x80000fe0  (read-only data)
.sdata   00000018  0x800013c0  (small data)
.l2      00000008  0x800013e0  (L2 data)
```
**Result:** ✅ All sections within 0x80000000-0x800013e7 range

**LOAD Segments:**
```
LOAD  0x80000000 - 0x80000fcd (text)
LOAD  0x80000fe0 - 0x800013bf (rodata)
LOAD  0x800013c0 - 0x800013c7 (sdata + l2)
```
**Result:** ✅ Total size 0x13e8 bytes matches error message

---

### ✅ 2. Verilator Version Check

```
Verilator 4.214 2021-10-17 rev v4.214
```
**Result:** ✅ Relatively old version (2021), might be compatibility issue

---

### ❌ 3. QuestaSim Test (Cannot Verify)

```
bash: vlib: command not found
```
**Result:** ❌ QuestaSim not installed, cannot compare with working reference

---

### ✅ 4. Memory Structure Analysis (CRITICAL FINDING)

**Verilator Generated Memory Declaration:**
```cpp
// From build/verilator/Vara_tb_verilator___024root.h:
VlUnpacked<VlWide<4>/*127:0*/, 524288> ara_tb_verilator__DOT__dut__DOT__i_ara_soc__DOT__gen_l2_banks__BRA__0__KET____DOT__l2_mem__DOT__sram;
```

**Memory Type Breakdown:**
- `VlUnpacked<...>` = Verilator unpacked array
- `VlWide<4>` = 4 × 32-bit = 128 bits wide per word
- `524288` = 2^19 = 0x80000 words deep
- **Total per bank:** 0x80000 words × 16 bytes/word = **0x800000 bytes = 8MB** ❗

**Configuration in ara_tb.cpp:**
```cpp
L2BankSize = L2BankNumWords * L2BankBeWidth
           = 1048576 * 16
           = 0x1000000 bytes = 16MB per bank
```

---

## 🔴 ROOT CAUSE IDENTIFIED

### Problem 1: NR_CORES Build Mismatch

**Verilator Build Has 2 Banks:**
```
gen_l2_banks__BRA__0__KET____DOT__l2_mem__DOT__sram  (524288 words)
gen_l2_banks__BRA__1__KET____DOT__l2_mem__DOT__sram  (524288 words)
```

**But ara_tb.cpp Thinks It Has 1 Bank:**
```
Output: "NR_CORES  = 1"
Output: "L2 Banks  = 1"
```

**The Math:**
- Total L2: 2^20 = 1M words
- Banks: 2 (from Verilator build)
- Per bank: 1M / 2 = 524K words = 0x80000 words
- Per bank size: 524K × 16 bytes = 8MB ✅ Matches Verilator!

**But ara_tb.cpp calculates:**
```cpp
L2NumBanks = 1  // WRONG! Should be 2
L2BankNumWords = 1M / 1 = 1M words
L2BankSize = 1M × 16 = 16MB  // WRONG! Should be 8MB
```

### Problem 2: Only Registering Bank 0

**Current code registers:**
```cpp
for (int bank = 0; bank < L2NumBanks; bank++) {  // L2NumBanks=1, so only bank 0
  // Registers: gen_l2_banks__BRA__0__KET__...
}
```

**But Verilator has:**
- gen_l2_banks__BRA__0__KET__... ✅ Registered
- gen_l2_banks__BRA__1__KET__... ❌ NOT registered!

---

## Error Sequence Explained

1. **Registration Phase:**
   ```
   RegisterMemoryArea("ram", path, 128bits, {base=0x80000000, size=0x1000000})
   ```
   ✅ Succeeds - memutil accepts the registration

2. **ELF Loading Phase:**
   ```
   Set `ram ... write with offset: 0x0 write with size: 0x13e8
   ```
   ✅ Starts - ELF is parsed correctly

3. **Memory Write Phase:**
   ```
   ERROR: No memory found at `TOP.ara_tb_verilator__DOT__...__DOT__sram'
   ```
   ❌ Fails - memutil tries to access memory but Verilator says path doesn't exist OR array bounds exceeded

---

## ✅ SOLUTION

### Root Cause Summary

**The Verilator model was built with NR_CORES=2, but ara_tb.cpp thinks NR_CORES=1**

This causes:
1. ara_tb.cpp only registers bank 0
2. ara_tb.cpp uses wrong memory size (16MB instead of 8MB per bank)
3. Bank 1 is never registered
4. Memory writes fail because parameters don't match actual hardware

### Fix: Rebuild Verilator with Correct Configuration

```bash
cd hardware

# Clean everything
rm -rf build/verilator

# Rebuild with EXPLICIT single-core config
config=default make verilate

# OR for multicore (4 cores):
config=default_mc make verilate
```

### Verify the Build

After rebuilding, check:

```bash
# Count how many banks Verilator generated
grep "gen_l2_banks__BRA__" build/verilator/Vara_tb_verilator___024root.h | grep "DOT__sram" | wc -l

# Should output:
# - "1" for single-core (config=default)
# - "4" for multicore (config=default_mc)
```

### Why This Happened

You likely built with `config=default_mc` (or NrAraSystems=2 was set somewhere) but are running with ara_tb.cpp that defaults to NR_CORES=1 when the macro isn't defined.

Check hardware/Makefile lines where NR_CORES is passed to Verilator C++ compilation.

---

## Commands for Single-Core Repo Test

To test if single-core original works, run these in the **single-core repository**:

```bash
# 1. Check current ara_tb.cpp
cat hardware/tb/verilator/ara_tb.cpp | grep -A5 "L2NumWords"

# 2. Clean rebuild
cd hardware
rm -rf build/verilator
make verilate

# 3. Test with hello_world
app=hello_world make simv

# 4. If it works, check the memory configuration
grep "L2NumWords\|L2BankSize" hardware/tb/verilator/ara_tb.cpp

# 5. Compare Verilator generated memory size
grep "l2_mem__DOT__sram" build/verilator/Vara_tb_verilator___024root.h
```

If single-core works, check what `L2NumWords` value it uses.

---

## Next Steps

1. ✅ **Fix memory size mismatch** - Change `L2NumWords = 1 << 19`
2. ⚠️ **Test if this resolves the issue**
3. ❓ **If still fails:** Check if single-core uses different memory access method
4. ❓ **Alternative:** Check if SystemVerilog testbench (ara_tb.sv) has different memory parameters

---

## Key Files to Check

1. `hardware/src/ara_soc.sv` - L2 memory instantiation parameters
2. `hardware/tb/ara_tb.sv` - Working QuestaSim memory parameters (lines 51-62)
3. Single-core `ara_tb.cpp` - Reference memory configuration
4. `hardware/include/ara/ara.svh` - Memory size defines

---

## Memory Parameter Comparison Needed

| Parameter | ara_tb.sv (QuestaSim) | ara_tb.cpp (Current) | Verilator Model | Status |
|-----------|----------------------|---------------------|-----------------|---------|
| L2NumWords | Check line 51 | 1 << 20 (1M) | 524288 (512K) | ❌ MISMATCH |
| L2BankWidth | Check line 55 | 128 bits | 128 bits | ✅ Match |
| L2BankNumWords | Check line 56 | ? | 524288 | ? |
| L2BankSize | Calc from above | 16MB | 8MB | ❌ MISMATCH |

**ACTION:** Verify ara_tb.sv line 51 `L2NumWords` value.