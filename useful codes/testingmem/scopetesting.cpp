// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51
//
// Author: Matheus Cavalcante <matheusd@iis.ee.ethz.ch>
// Description:
// Top-level Verilator test-bench for Ara.

#include <fstream>
#include <iostream>

#include "verilated_toplevel.h"
#include "verilator_memutil.h"
#include "verilator_sim_ctrl.h"

int main(int argc, char **argv) {
  // Create an instance of the DUT
  ara_tb_verilator *tb = new ara_tb_verilator;

  // Initialize lowRISC's verilator utilities
  VerilatorMemUtil memutil;
  VerilatorSimCtrl &simctrl = VerilatorSimCtrl::GetInstance();
  simctrl.SetTop(tb, &tb->clk_i, &tb->rst_ni,
                 VerilatorSimCtrlFlags::ResetPolarityNegative);

  // Determine number of L2 banks based on NR_CORES
  #ifdef NR_CORES
    const int NrAraSystems = NR_CORES;
  #else
    const int NrAraSystems = 1;
  #endif
  
  const int L2NumBanks = NrAraSystems;

  // Memory configuration (matching ara_tb.sv)
  const uint64_t L2NumWords = 1 << 20;  // 2^20
  const uint64_t L2BankNumWords = L2NumWords / L2NumBanks;
  const uint64_t L2BankWidth = 64 * NR_LANES / 2;  // AxiWideDataWidth
  const uint64_t L2BankBeWidth = L2BankWidth / 8;
  const uint64_t L2BankSize = L2BankNumWords * L2BankBeWidth;
  const uint64_t L2BeWidth = L2BankBeWidth * L2NumBanks;
  const uint64_t L2ByteOffset = __builtin_ctz(L2BeWidth);

  std::cout << "Simulation of Ara" << std::endl
            << "=================" << std::endl
            << "  NR_CORES  = " << NrAraSystems << std::endl
            << "  NR_LANES  = " << NR_LANES << std::endl
            << "  L2 Banks  = " << L2NumBanks << std::endl
            << "  Bank Size = 0x" << std::hex << L2BankSize << std::dec << std::endl
            << std::endl;

  // Initialize ALL L2 memory banks
  // This matches the loop in ara_tb.sv lines 119-157
  // IMPORTANT: Use full address range for all banks - memutil handles interleaving
  for (int bank = 0; bank < L2NumBanks; bank++) {
    char mem_name[64];
    char mem_path[512];
    char mem_path_internal[512];
    
    snprintf(mem_name, sizeof(mem_name), "ram");  // Use same name "ram" for all banks
    snprintf(mem_path, sizeof(mem_path), 
             "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem",
             bank);
    
    // Internal format
    snprintf(mem_path_internal, sizeof(mem_path_internal), 
              "TOP.ara_tb_verilator__DOT__dut__DOT__i_ara_soc__DOT__gen_l2_banks__BRA__%d__KET____DOT__l2_mem",
              bank);

    // Register with FULL address space (0x80000000 - 0x80100000)
    // MemUtil will interleave based on L2ByteOffset automatically
    // MemAreaLoc l2_mem_bank = {
    //   .base = 0x80000000,
    //   .size = 0x00100000  // 1MB total (interleaved across banks)
    // };
    // Split address space: each bank gets 1/Nth of the range
    MemAreaLoc l2_mem_bank = {
        .base = (uint32_t)(0x80000000 + (bank * L2BankSize)),  // Offset by bank
        .size = L2BankSize  // Size per bank
    };
    
    // memutil.RegisterMemoryArea(mem_name, mem_path, L2BankWidth, &l2_mem_bank);
    
    // std::cout << "Bank " << bank << ": " << mem_path << std::endl;
    try {
        memutil.RegisterMemoryArea("ram", mem_path_internal, L2BankWidth, &l2_mem_bank);
        std::cout << "Bank " << bank << ": " << mem_path_internal << " (internal format)" << std::endl;
    } catch (...) {
        // Fall back to dot format
        memutil.RegisterMemoryArea("ram", mem_path, L2BankWidth, &l2_mem_bank);
        std::cout << "Bank " << bank << ": " << mem_path << " (dot format)" << std::endl;
    }
  }

// After your scope check, add this:
std::cout << "\n=== Testing Multiple Path Formats ===" << std::endl;

// Test 1: Dot format (what you're using)
const VerilatedScope* scope1 = Verilated::scopeFind("TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[0].l2_mem.sram");
std::cout << "Dot format [0]:     " << (scope1 ? "✓ Found" : "✗ NOT found") << std::endl;

// Test 2: Verilator internal format
const VerilatedScope* scope2 = Verilated::scopeFind("TOP.ara_tb_verilator__DOT__dut__DOT__i_ara_soc__DOT__gen_l2_banks__BRA__0__KET____DOT__l2_mem");
std::cout << "Internal format:    " << (scope2 ? "✓ Found" : "✗ NOT found") << std::endl;

// Test 3: Without TOP prefix
const VerilatedScope* scope3 = Verilated::scopeFind("ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[0].l2_mem");
std::cout << "Without TOP:        " << (scope3 ? "✓ Found" : "✗ NOT found") << std::endl;

// Test 4: List all available scopes
std::cout << "\n=== Available Scopes (first 10) ===" << std::endl;
const VerilatedScope* scope_iter = Verilated::scopeFind("");
int count = 0;
while (scope_iter && count < 10) {
    std::cout << "  " << scope_iter->name() << std::endl;
    // Move to next scope (this API may vary by Verilator version)
    count++;
    break; // Remove this if there's a way to iterate
}
std::cout << "============================\n" << std::endl;

  simctrl.RegisterExtension(&memutil);

  simctrl.SetInitialResetDelay(5);
  simctrl.SetResetDuration(5);

  bool exit_app = false;
  int ret_code = simctrl.ParseCommandArgs(argc, argv, exit_app);
  if (exit_app) {
    return ret_code;
  }

  simctrl.RunSimulation();

  return tb->exit_o >> 1;
}