# Modification in Multicore

- In `ara/hardware/Makefile`, 
Line numbers 23,25 set the default configuration for multicore to `default_mc` instead of `mc_default`.
Line number 203 added a compiler flag to define the number of cores (`NR_CORES`) during Verilator compilation.
Line number 218 modified the `make` command to use all available processors for faster compilation by replacing `-j4` with `-j$(shell nproc)`.

Information:
ara_tb.sv (`ara/hardware/ara_tb.sv`) is updated to support multicore simulation for QuestaSim.
ara_tb.cpp (`ara/hardware/tb/verilator/ara_tb.cpp`) is responsible for verilator.

- Updated  ara_tb.cpp and ara_tb_verilator.sv to support multicore simulation.

Don't forget to apply the patch to add DPI functions to tc_sram.sv


