// Copyright 2020 ETH Zurich and University of Bologna.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: Matheus Cavalcante, ETH Zurich
//         Samuel Riedel, ETH Zurich

// Measure from the kernel call to the kernel to the kernel end
#if   defined VCD_DUMP && ((defined MM_1 || defined MM_2 || defined MM_4) || (defined MM_8 && NR_CORES < 8) || (defined MM_16 && NR_CORES < 4))
#define VCD_DUMP_WHOLE
#pragma message("VCD_DUMP_WHOLE successfully initialized")
#elif defined VCD_DUMP && (defined MM_32 && NR_CORES == 1)
#define VCD_DUMP_MID_KERNEL
#pragma message("VCD_DUMP_MID_KERNEL successfully initialized")
#elif defined VCD_DUMP && ((defined MM_64 || defined MM_128 || defined MM_256) || (defined MM_32 && NR_CORES > 1) || (defined MM_16 && NR_CORES > 2) || (defined MM_8 && NR_CORES > 4))
#define VCD_DUMP_IN_KERNEL
#pragma message("VCD_DUMP_IN_KERNEL successfully initialized")
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kernel/fmatmul.h"
#include "runtime.h"
#include "util.h"

#ifndef SPIKE
#include "printf.h"
#endif

#define WARM_UP_CYCLES 1

// Define Matrix dimensions:
// C = AB with A=[MxN], B=[NxP], C=[MxP]
extern uint64_t M;
extern uint64_t N;
extern uint64_t P;

extern double a[] __attribute__((aligned(32 * NR_LANES), section(".l2")));
extern double b[] __attribute__((aligned(32 * NR_LANES), section(".l2")));
extern double c[] __attribute__((aligned(32 * NR_LANES), section(".l2")));
// Gold results
extern double g[] __attribute__((aligned(32 * NR_LANES), section(".l2")));

// Define half of the range for FP comparison on the results
#define THRESHOLD 0.001

// ---------------
// 1x1
// ---------------

inline void fmatmul_1x1(double *c, const double *a, const double *b,
                        const unsigned long int M, const unsigned long int N,
                        const unsigned long int P, int core_id) {
  // We work on 1 rows of the matrix at once
  const unsigned long int block_size = 1;
  unsigned long int block_size_p;

#ifdef VCD_DUMP_WHOLE
    // Start dumping VCD
    if (!core_id)
      event_trigger = +1;
#endif

  // Set the vector configuration
  asm volatile("vsetvli %0, %1, e64, m4, ta, ma" : "=r"(block_size_p) : "r"(P));

  // Slice the matrix into a manageable number of columns p_
  for (unsigned long int p = 0; p < P; p += block_size_p) {
    // Set the vector length
    const unsigned long int p_ = MIN(P - p, block_size_p);

    // Find pointers to the submatrices
    const double *b_ = b + p;
    double *c_ = c + p;

    asm volatile("vsetvli zero, %0, e64, m4, ta, ma" ::"r"(p_));

    // Iterate over the rows
    for (unsigned long int m = 0; m < M; m += block_size) {
      // Find pointer to the submatrices
      const double *a_ = a + m * N;
      double *c__ = c_ + m * P;

      fmatmul_vec_1x1_slice_init();
      fmatmul_vec_1x1(c__, a_, b_, N, P, core_id);
    }
  }
}

inline void fmatmul_vec_1x1_slice_init() {
  asm volatile("vmv.v.i v0,  0");
}

inline void fmatmul_vec_1x1(double *c, const double *a, const double *b,
                            const unsigned long int N,
                            const unsigned long int P, int core_id) {
  // Temporary variables
  double t0;

  // Original pointer
  const double *a_ = a;

#ifdef VCD_DUMP_MID_KERNEL
    // Start dumping VCD
    if (!core_id)
      event_trigger = +1;
#endif

  // Prefetch one row of matrix B
  asm volatile("vle64.v v16, (%0);" ::"r"(b));
  b += P;

  // Prefetch one row of scalar values
  t0 = *a;

  // Compute the multiplication
  unsigned long int n = 0;

  while (n != N) {
#ifdef VCD_DUMP_IN_KERNEL
    // Start dumping VCD
    if (n == 12 && !core_id)
      event_trigger = +1;
    // Stop dumping VCD
    if (n == 16 && !core_id)
      event_trigger = -1;
#endif

    // Calculate pointer to the matrix A
    a = a_ + ++n;

    asm volatile("vfmacc.vf v0, %0, v16" ::"f"(t0));
    t0 = *a, a += N;

    // Load one row of B
    asm volatile("vle64.v v20, (%0);" ::"r"(b));
    b += P;

    a = a_ + ++n;

    if (n == N)
      break;

    asm volatile("vfmacc.vf v0, %0, v20" ::"f"(t0));
    t0 = *a, a += N;

    // Load one row of B
    asm volatile("vle64.v v16, (%0);" ::"r"(b));
    b += P;
  }

  // Last iteration: store results
  asm volatile("vfmacc.vf v0, %0, v20" ::"f"(t0));
  asm volatile("vse64.v v0, (%0);" ::"r"(c));

#ifdef VCD_DUMP_MID_KERNEL
    // Stop dumping VCD
    if (!core_id)
      event_trigger = -1;
#endif
}

// Verify the matrix
int verify_matrix(double *result, double *gold, size_t R, size_t C,
                  double threshold) {
  for (uint64_t i = 0; i < R; ++i) {
    for (uint64_t j = 0; j < C; ++j) {
      uint64_t idx = i * C + j;
      if (!similarity_check(result[idx], gold[idx], threshold)) {
        return (i + j) == 0 ? -1 : idx;
      }
    }
  }
  return 0;
}

void print_matrix(const char *name, double *mat, int rows, int cols, int core_id) {
  if (core_id != 0) return;  // Only core 0 prints
  
  printf("\n%s (%dx%d):\n", name, rows, cols);
  for (int i = 0; i < rows && i < 4; i++) {  // Print first 4 rows only
    for (int j = 0; j < cols && j < 4; j++) {  // Print first 4 cols only
      printf("  [%d,%d]=%.6f", i, j, mat[i * cols + j]);
    }
    if (cols > 4) printf(" ...");
    printf("\n");
  }
  if (rows > 4) printf("  ...\n");
}

// Helper function to print memory addresses
void print_addresses(int core_id) {
  if (core_id != 0) return;
  
  printf("\nMemory addresses:\n");
  printf("  a @ 0x%lx\n", (unsigned long)a);
  printf("  b @ 0x%lx\n", (unsigned long)b);
  printf("  c @ 0x%lx\n", (unsigned long)c);
  printf("  g @ 0x%lx\n", (unsigned long)g);
}


// This main function is working and giving no error for single core compute
int main(int core_id) {
  uint64_t m_priv = (M >> LOG2_NR_CORES);
  double *a_priv = a + core_id * (m_priv * N);
  double *c_priv = c + core_id * (m_priv * P);

  // if(core_id == 0){
  //   // print_matrix("Matrix A", a, M, N, core_id);
  //   // print_matrix("Matrix B", b, N, P, core_id);
  //   print_matrix("Matrix G (expected output)", g, M, P, core_id);
  // }
  primitive_synch(core_id);

  // ONLY CORE 0 DOES COMPUTATION FOR NOW
  // if (core_id == 0) {
  //   printf("\n[Core 0] Starting computation...\n");
  //   printf("M=%lu, N=%lu, P=%lu, m_priv=%lu\n", M, N, P, m_priv);
  // }
    
  #ifndef VCD_DUMP
    if (!core_id) {
      start_timer();
    }
  #endif

    fmatmul_1x1(c_priv, a_priv, b, m_priv, N, P, core_id);
    
    // asm volatile("fence" ::: "memory");
  //   if (core_id == 0) {
  //   printf("[Core 0] Computation done\n");
  //   printf("c[0]=%f, c[1]=%f, c[2]=%f, c[3]=%f\n", c[0], c[1], c[2], c[3]);
    
    
  //   printf("After fence: c[0]=%f\n", c[0]);
  // }
  // primitive_synch(core_id);
  //    if (core_id == 1) {
  //   printf("[Core 1] Computation done\n");
  //   printf("c[0]=%f, c[1]=%f, c[2]=%f, c[3]=%f\n", c[0], c[1], c[2], c[3]);
    
    
  //   printf("After fence: c[0]=%f\n", c[0]);
  // }

  primitive_synch(core_id);

  #ifndef VCD_DUMP
    if (!core_id) {
      stop_timer();
    }
  #endif

  if(core_id == 0){

    print_matrix("Matrix C (computed output)", c, M, P, core_id);
  }
  primitive_synch(core_id);

  if (!core_id) {
    int error = verify_matrix(c, g, M, P, THRESHOLD);
    if (error == 0) {
      printf("PASSED!\n");
    } else {
      printf("FAILED at index %d\n", error);
    }
  }

  if (!core_id) {
    // Calculate performance metrics
    int64_t runtime = get_timer();
    float performance = 2.0 * M * N * P / runtime;  // FLOPs per cycle
    float utilization = 100 * performance / (2.0 * NR_LANES * NR_CORES);

    printf("\n");
    printf("========================================\n");
    printf("  RESULTS\n");
    printf("========================================\n");
    printf("\n");
    printf("Execution time: %ld cycles\n", runtime);
    printf("Performance: %.3f FLOP/cycle\n", performance);
    printf("Utilization: %.2f%% of peak\n", utilization);
    printf("  (Peak = %d lanes × %d cores × 2 ops/cycle = %d FLOP/cycle)\n",
           NR_LANES, NR_CORES, 2 * NR_LANES * NR_CORES);
    printf("\n");
  }
  
  primitive_synch(core_id);
  return 0;
}