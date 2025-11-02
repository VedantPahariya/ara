// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#pragma once

//
// A wrapper class that converts a VerilatorMemutil into a SimCtrlExtension
//

#include <memory>

#include "dpi_memutil.h"
#include "sim_ctrl_extension.h"

class VerilatorMemUtil : public SimCtrlExtension {
 public:
  // No-argument constructor makes a VerilatorMemUtil. Single-argument
  // constructor wraps its mem_util argument (but does not take ownership).
  VerilatorMemUtil();
  explicit VerilatorMemUtil(DpiMemUtil *mem_util);

  // Declared in SimCtrlExtension
  bool ParseCLIArguments(int argc, char **argv, bool &exit_app) override;

  // Get underlying DpiMemUtil object
  DpiMemUtil *GetUnderlying() { return mem_util_; }

  // Pass-thru functions to underlying object
  bool RegisterMemoryArea(const std::string name, const std::string location,
                          size_t width_bit, const MemAreaLoc *addr_loc) {
    return mem_util_->RegisterMemoryArea(name, location, width_bit, addr_loc);
  }

  bool RegisterMemoryArea(const std::string name, const std::string location) {
    return mem_util_->RegisterMemoryArea(name, location);
  }

  /**
   * Load ELF file with bank interleaving
   * Convenience wrapper for DpiMemUtil::LoadElfToNamedMemBanked
   */
  void LoadElfWithBanking(bool verbose,
                          const std::string &name,
                          const std::string &filepath,
                          int num_banks,
                          int bank_id) {
    mem_util_->LoadElfToNamedMemBanked(verbose, name, filepath, 
                                       num_banks, bank_id);
  }
  
  // Expose mem_util_ for direct access if needed
  DpiMemUtil* GetMemUtil() { return mem_util_; }

 private:
  DpiMemUtil *mem_util_;
  std::unique_ptr<DpiMemUtil> allocation_;
};
