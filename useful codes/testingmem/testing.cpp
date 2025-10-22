// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51
//
// Author: Matheus Cavalcante <matheusd@iis.ee.ethz.ch>
// Description:
// Top-level Verilator test-bench for Ara.

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "verilated_toplevel.h"
#include "verilator_memutil.h"
#include "verilator_sim_ctrl.h"

// Debug function to probe for the correct memory path
bool tryRegisterMemory(VerilatorMemUtil &memutil, const char* path, int width) {
  std::cout << "Trying path: " << path << " ... ";
  
  try {
    // Use a test region that won't conflict with real memory
    MemAreaLoc test_mem = {
      .base = 0xA0000000,  // Test address far from real memory
      .size = 0x1000       // Small size for testing
    };
    
    // Try to register this path
    memutil.RegisterMemoryArea("test_region", path, width, &test_mem);
    std::cout << "SUCCESS!" << std::endl;
    return true;
  } catch (const std::exception& e) {
    std::cout << "FAILED: " << e.what() << std::endl;
    return false;
  } catch (...) {
    std::cout << "FAILED with unknown exception" << std::endl;
    return false;
  }
}

int main(int argc, char **argv) {
  // Create an instance of the DUT
  ara_tb_verilator *tb = new ara_tb_verilator;

  // Initialize lowRISC's verilator utilities
  VerilatorMemUtil memutil;
  VerilatorSimCtrl &simctrl = VerilatorSimCtrl::GetInstance();
  simctrl.SetTop(tb, &tb->clk_i, &tb->rst_ni,
                 VerilatorSimCtrlFlags::ResetPolarityNegative);

  // Determine number of L2 banks based on NR_CORES
  // In multicore configuration: L2NumBanks = NrAraSystems
  #ifdef NR_CORES
    const int L2NumBanks = NR_CORES;
  #else
    const int L2NumBanks = 1;
  #endif

  // Memory configuration (matching ara_tb.sv)
  const uint64_t L2NumWords = 1 << 20;  // 2^20
  const uint64_t L2BankNumWords = L2NumWords / L2NumBanks;
  const uint64_t L2BankWidth = 64 * NR_LANES / 2;  // AxiWideDataWidth
  const uint64_t L2BankBeWidth = L2BankWidth / 8;
  const uint64_t L2BankSize = L2BankNumWords * L2BankBeWidth;

  // Initialize ALL L2 memory banks
  std::cout << "Initializing " << L2NumBanks << " L2 memory banks..." << std::endl;
  std::cout << "Memory configuration:" << std::endl;
  std::cout << "  L2NumWords    = " << L2NumWords << std::endl;
  std::cout << "  L2BankNumWords = " << L2BankNumWords << std::endl;
  std::cout << "  L2BankWidth   = " << L2BankWidth << " bits" << std::endl;
  std::cout << "  L2BankBeWidth = " << L2BankBeWidth << " bytes" << std::endl;
  std::cout << "  L2BankSize    = 0x" << std::hex << L2BankSize << std::dec << " bytes" << std::endl;
  std::cout << std::endl;
  
  // Probe different memory path formats to find the correct one
  std::cout << "====== MEMORY PATH DEBUG ======" << std::endl;
  std::cout << "Probing for valid memory paths..." << std::endl;
  
  // List of possible path formats to try
  const char* path_templates[] = {
    // Standard path - original attempt
    "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem.sram",
    // Try with trailing [0] for array indexing
    "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem.sram[0]",
    // Try without .sram suffix
    "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem",
    // Try with init_val instead of sram (from ara_tb.sv)
    "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem.init_val",
    // Try with memory instead of sram
    "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem.memory",
    // Try with flattened Verilator naming convention
    "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks_%d__l2_mem__sram",
    // Try with double indexing
    "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem.sram[0][0]",
    // Try without TOP prefix
    "ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem.sram",
    // Try with Verilator DOT format
    "TOP.ara_tb_verilator__DOT__dut__DOT__i_ara_soc__DOT__gen_l2_banks__BRA__%d__KET____DOT__l2_mem__DOT__sram"
  };
  
  const int num_templates = sizeof(path_templates) / sizeof(char*);
  bool found_valid_path = false;
  std::string working_template;
  
  // Try each template with bank 0
  for (int i = 0; i < num_templates; i++) {
    char test_path[512];
    snprintf(test_path, sizeof(test_path), path_templates[i], 0);
    
    if (tryRegisterMemory(memutil, test_path, L2BankWidth)) {
      found_valid_path = true;
      working_template = path_templates[i];
      
      // We can't unregister memory areas, so we'll just skip future tests
      // after finding a working path to avoid conflicts
      std::cout << "Found working path format, stopping search." << std::endl;
      break;
    }
  }
  
  // Note: No UnregisterMemoryArea function exists in VerilatorMemUtil
  // So we'll just have to live with the test_region still registered
  
  std::cout << "====== END DEBUG ======" << std::endl;
  std::cout << std::endl;
  
  if (!found_valid_path) {
    std::cerr << "ERROR: Could not find a valid memory path format!" << std::endl;
    std::cerr << "Try checking the Verilator generated code for the actual memory path." << std::endl;
    
    // Fall back to original path as last resort
    working_template = "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem.sram";
    std::cout << "Falling back to: " << working_template << std::endl;
  } else {
    std::cout << "Found working memory path template: " << working_template << std::endl;
  }
  
  // Now register the actual memory banks using the working path template
  // Use a different region name ("ram_actual") to avoid conflict with test region
  // std::string mem_region = found_valid_path ? "ram_actual" : "ram";
  std::string mem_region = "ram";
  
  for (int bank = 0; bank < L2NumBanks; bank++) {
    char mem_name[64];
    char mem_path[512];
    // snprintf(mem_path, sizeof(mem_path), working_template.c_str(), bank);
    
    snprintf(mem_name, sizeof(mem_name), "ram");  // Use same name "ram" for all banks
    snprintf(mem_path, sizeof(mem_path), 
             "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem.sram",
             bank);
    
    // Memory region configuration
    MemAreaLoc l2_mem_bank = {
      .base = 0x80000000,
      .size = 0x00100000  // 1MB total
    };
    
    // Register this memory bank
    try {
      memutil.RegisterMemoryArea(mem_name, mem_path, L2BankWidth, &l2_mem_bank);
      std::cout << "  Bank " << bank << ": " << mem_path << " registered successfully" << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "  Bank " << bank << ": " << mem_path << " failed to register: " << e.what() << std::endl;
      return 1;
    } catch (...) {
      std::cerr << "  Bank " << bank << ": " << mem_path << " failed to register with unknown error" << std::endl;
      return 1;
    }
  }
  
  simctrl.RegisterExtension(&memutil);
  
  // Note: No PrintMemRegions function exists in VerilatorMemUtil
  std::cout << "\nMemory regions registered." << std::endl;

  simctrl.SetInitialResetDelay(5);
  simctrl.SetResetDuration(5);

  bool exit_app = false;
  int ret_code = simctrl.ParseCommandArgs(argc, argv, exit_app);
  if (exit_app) {
    return ret_code;
  }

  std::cout << "Simulation of Ara" << std::endl
            << "================" << std::endl
            << "  NR_CORES  = " << L2NumBanks << std::endl
            << "  NR_LANES  = " << NR_LANES << std::endl
            << "  L2 Banks  = " << L2NumBanks << std::endl
            << std::endl;

  simctrl.RunSimulation();

  return tb->exit_o >> 1;
}