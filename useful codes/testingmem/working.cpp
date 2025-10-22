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

  std::cout << "Simulation of Ara" << std::endl
            << "=================" << std::endl
            << "  NR_CORES  = " << L2NumBanks << std::endl
            << "  NR_LANES  = " << NR_LANES << std::endl
            << "  L2 Banks  = " << L2NumBanks << std::endl
            << "  Bank Size = 0x" << std::hex << L2BankSize << std::dec << std::endl
            << std::endl;

  // Register ALL L2 memory banks
  for (int bank = 0; bank < L2NumBanks; bank++) {
    char mem_path[512];
    
    // CORRECT: Path without .sram (as confirmed by scope test)
    snprintf(mem_path, sizeof(mem_path), 
             "TOP.ara_tb_verilator.dut.i_ara_soc.gen_l2_banks[%d].l2_mem",
             bank);
    
    // Full address space for each bank - memutil handles interleaving
    MemAreaLoc l2_mem_bank = {
      .base = 0x80000000,
      .size = 0x00100000  // 1MB total
    };

    // MemAreaLoc l2_mem_bank = {
    //     .base = (0x80000000 + (bank * L2BankSize)),  // Offset by bank
    //     .size = L2BankSize  // Size per bank
    // };
    
    memutil.RegisterMemoryArea("ram", mem_path, L2BankWidth, &l2_mem_bank);
    std::cout << "Bank " << bank << ": " << mem_path << std::endl;
  }

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

// # Simulation
// .PHONY: simv
// simv:
//     $(veril_library)/V$(veril_top) $(if $(trace),-t,) -l ram,$(app_path)/$(app),elf

// .PHONY: riscv_tests_simv
// riscv_tests_simv: $(tests)

// $(tests): rv%: $(app_path)/rv%
//     $(veril_library)/V$(veril_top) $(if $(trace),-t,) -l ram,$<,elf &> $(buildpath)/$@.trace


// these lines use ram keyword, if i am using multiple names then I have to create ram1,ram2...