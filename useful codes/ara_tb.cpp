// Copyright 2021 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51
//
// Author: Matheus Cavalcante <matheusd@iis.ee.ethz.ch>
// Description:
// Top-level Verilator test-bench for Ara (multicore support).

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
    const int L2NumBanks = NR_CORES;
  #else
    const int L2NumBanks = 1;
  #endif

  // Memory configuration (matching ara_tb.sv)
  const uint64_t L2NumWords = 1 << 20;  // 2^20 = 1,048,576 words TOTAL
  const uint64_t L2BankNumWords = L2NumWords / L2NumBanks;  // Words per bank
  const uint64_t L2BankWidth = 64 * NR_LANES / 2;  // AxiWideDataWidth (128 bits for 4 lanes)
  const uint64_t L2BankBeWidth = L2BankWidth / 8;  // Bytes per word (16 bytes)
  const uint64_t L2BankSize = L2BankNumWords * L2BankBeWidth;  // Size per bank
  const uint64_t L2TotalSize = L2NumWords * L2BankBeWidth;  // Total memory (16 MB)

  std::cout << "Simulation of Ara" << std::endl
            << "=================" << std::endl
            << "  NR_CORES      = " << L2NumBanks << std::endl
            << "  NR_LANES      = " << NR_LANES << std::endl
            << "  L2 Banks      = " << L2NumBanks << std::endl
            << "  Words/Bank    = " << L2BankNumWords << std::endl
            << "  Bank Width    = " << L2BankWidth << " bits" << std::endl
            << "  Bank Size     = 0x" << std::hex << L2BankSize << std::dec 
            << " (" << (L2BankSize >> 20) << " MB)" << std::endl
            << "  Total L2 Size = 0x" << std::hex << L2TotalSize << std::dec 
            << " (" << (L2TotalSize >> 20) << " MB)" << std::endl
            << std::endl;

  // ============================================================================
  // OPTION A: Interleaved Address Space (Hardware-accurate)
  // ============================================================================
  // All banks use the SAME address range, hardware interleaves by cache line
  
  if (L2NumBanks == 1) {
    // Single-core: Simple single registration
    char mem_path[512];
    snprintf(mem_path, sizeof(mem_path), 
             "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[0].l2_mem");
    
    MemAreaLoc l2_mem = {
      .base = 0x80000000,
      .size = static_cast<uint32_t>(L2TotalSize)  // Full 16 MB
    };
    
    memutil.RegisterMemoryArea("ram", mem_path, L2BankWidth, &l2_mem);
    std::cout << "Bank 0: " << mem_path << std::endl;
    std::cout << "  Address: 0x80000000 - 0x" << std::hex 
              << (0x80000000 + L2TotalSize - 1) << std::dec << std::endl;
    
    simctrl.RegisterExtension(&memutil);
    
    bool exit_app = false;
    int ret_code = simctrl.ParseCommandArgs(argc, argv, exit_app);
    if (exit_app) return ret_code;

  } else {
    // Multi-core: Register each bank with unique name
    // Each bank covers THE SAME address space (interleaved)
    
    for (int bank = 0; bank < L2NumBanks; bank++) {
      char mem_name[64];
      char mem_path[512];
      
      // Unique name per bank (ram0, ram1, ram2, ram3...)
      snprintf(mem_name, sizeof(mem_name), "ram%d", bank);
      
      snprintf(mem_path, sizeof(mem_path), 
               "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem",
               bank);
      
      memutil.RegisterMemoryArea(mem_name, mem_path, L2BankWidth, nullptr);
      std::cout << "Registered: " << mem_name << " at " << mem_path << std::endl;

      // std::cout << "Bank " << bank << ": " << mem_path << std::endl;
      // std::cout << "  Region: '" << mem_name << "'" << std::endl;
      // std::cout << "  Address: 0x80000000 - 0x" << std::hex 
      //           << (0x80000000 + L2TotalSize - 1) << std::dec 
      //           << " (interleaved)" << std::endl;
    }
    
    // Step 2: Extract ELF path from command line
    std::string elf_path;
    for (int i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
        std::string arg = argv[i + 1];
        size_t first_comma = arg.find(',');
        size_t second_comma = arg.find(',', first_comma + 1);
        if (first_comma != std::string::npos) {
          elf_path = arg.substr(first_comma + 1, 
                                second_comma - first_comma - 1);
          break;
        }
      }
    }
    
    if (elf_path.empty()) {
      std::cerr << "ERROR: No ELF file specified with -l" << std::endl;
      return 1;
    }
    
    std::cout << "\n=== Loading ELF with Bank Interleaving ===" << std::endl;
    std::cout << "ELF: " << elf_path << std::endl;
    std::cout << "Banks: " << L2NumBanks << std::endl;
    
    // Step 3: Load ELF to each bank with interleaving
    for (int bank = 0; bank < L2NumBanks; bank++) {
      char mem_name[64];
      snprintf(mem_name, sizeof(mem_name), "ram%d", bank);
      
      std::cout << "Loading bank " << bank << "..." << std::endl;
      
      try {
        memutil.LoadElfWithBanking(
          /*verbose=*/true,
          mem_name,
          elf_path,
          L2NumBanks,
          bank
        );
      } catch (const std::exception &err) {
        std::cerr << "ERROR: " << err.what() << std::endl;
        return 1;
      }
    }
    
    std::cout << "=== ELF Loading Complete ===\n" << std::endl;
  }
  
  // Common simulation start
  simctrl.SetInitialResetDelay(5);
  simctrl.SetResetDuration(5);
  simctrl.RunSimulation();
  
  return tb->exit_o >> 1;
}

//     std::cout << "\nNote: Memory is INTERLEAVED across banks by " 
//               << L2BankBeWidth << "-byte cache lines" << std::endl;
//   }

//   std::cout << std::endl;

//   simctrl.RegisterExtension(&memutil);
//   simctrl.SetInitialResetDelay(5);
//   simctrl.SetResetDuration(5);

//   bool exit_app = false;
//   int ret_code = simctrl.ParseCommandArgs(argc, argv, exit_app);
//   if (exit_app) {
//     return ret_code;
//   }

//   simctrl.RunSimulation();

//   return tb->exit_o >> 1;
// }
