# Modification in Multicore

- In `ara/hardware/Makefile`,  
  - Line numbers 23,25 set the default configuration for multicore to `default_mc` instead of `mc_default`.  
  - Line number 203 added a compiler flag to define the number of cores (`NR_CORES`) during Verilator compilation.  
  - Line number 218 modified the `make` command to use all available processors for faster compilation by replacing `-j4` with `-j$(shell nproc)`.  

- In `ara/apps/common/runtime.mk`,  
Line numbers 36 set the default configuration for multicore to `default_mc` instead of `mc_default`.  

- Information about Test Bench Files:  
  - ara_tb.sv (`ara/hardware/ara_tb.sv`) is updated to support multicore simulation for QuestaSim.  
  - ara_tb.cpp (`ara/hardware/tb/verilator/ara_tb.cpp`) is responsible for Verilator.  

- Updated ara_tb.cpp and ara_tb_verilator.sv to support multicore simulation. Fixed files are uploaded to the `useful codes` folder

- In `ara/hardware/tb/verilator`,  
  - Under `lowrisc_dv_verilator_memutil_dpi`, change the files `dpi_memutil.cc` & `dpi_memutil.h` with the updated one in `useful codes` folder  
  - Under `lowrisc_dv_verilator_memutil_verilator`, change the file `verilator_memutil.h` with the updated.  

- Don't forget to apply the patch to add DPI functions to tc_sram.sv using the following command:  
  ```bash
  # Go to the hardware folder
  cd hardware
  # Apply the patches (only need to run this once)
  make apply-patches
  ```




