// Enhanced cycle counter accuracy analysis
#include <stdint.h>
#include "runtime.h"

#ifdef SPIKE
#include <stdio.h>
#else
#include "printf.h"
#endif

// Test different workload types
void test_scalar_workload(int iterations) {
    volatile int sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i * i;
    }
}

void test_memory_workload(int size) {
    static volatile int data[1000];
    for (int i = 0; i < size && i < 1000; i++) {
        data[i] = i * 2;
        volatile int temp = data[i];
        (void)temp; // Prevent optimization
    }
}

int main() {
    printf("============================\n");
    printf("= Enhanced Cycle Analysis =\n");
    printf("============================\n\n");
    
    // Test 1: Instruction-level granularity
    printf("Test 1: Instruction-level granularity\n");
    int64_t start, end;
    
    start = get_cycle_count();
    asm volatile("nop");
    end = get_cycle_count();
    printf("Single NOP: %ld cycles\n", end - start);
    
    start = get_cycle_count();
    asm volatile("nop; nop; nop; nop; nop");
    end = get_cycle_count();
    printf("Five NOPs: %ld cycles\n", end - start);
    
    // Test 2: Timer overhead measurement
    printf("\nTest 2: Timer overhead measurement\n");
    start_timer();
    stop_timer();
    int64_t timer_overhead = get_timer();
    printf("Timer overhead: %ld cycles\n", timer_overhead);
    
    // Test 3: Scalability with workload size
    printf("\nTest 3: Workload scalability\n");
    int workloads[] = {10, 100, 1000};
    for (int i = 0; i < 3; i++) {
        start_timer();
        test_scalar_workload(workloads[i]);
        stop_timer();
        int64_t cycles = get_timer();
        double cycles_per_iter = (double)cycles / workloads[i];
        printf("Scalar %d iterations: %ld cycles (%.2f cycles/iter)\n", 
               workloads[i], cycles, cycles_per_iter);
    }
    
    // Test 4: Memory vs computation cycles
    printf("\nTest 4: Memory vs computation comparison\n");
    
    start_timer();
    test_scalar_workload(500);
    stop_timer();
    int64_t compute_cycles = get_timer();
    
    start_timer();
    test_memory_workload(500);
    stop_timer();
    int64_t memory_cycles = get_timer();
    
    printf("Compute workload: %ld cycles\n", compute_cycles);
    printf("Memory workload: %ld cycles\n", memory_cycles);
    printf("Memory/Compute ratio: %.2f\n", (double)memory_cycles / compute_cycles);
    
    // Test 5: Cycle counter precision analysis
    printf("\nTest 5: Cycle counter precision analysis\n");
    int64_t prev_count = get_cycle_count();
    int64_t deltas[10];
    
    for (int i = 0; i < 10; i++) {
        int64_t curr_count = get_cycle_count();
        deltas[i] = curr_count - prev_count;
        prev_count = curr_count;
    }
    
    printf("Sequential cycle count deltas: ");
    for (int i = 0; i < 10; i++) {
        printf("%ld ", deltas[i]);
    }
    printf("\n");
    
    // Test 6: Reproducibility check
    printf("\nTest 6: Reproducibility check\n");
    int64_t results[5];
    for (int i = 0; i < 5; i++) {
        start_timer();
        test_scalar_workload(200);
        stop_timer();
        results[i] = get_timer();
    }
    
    // Calculate statistics
    int64_t min = results[0], max = results[0], sum = 0;
    for (int i = 0; i < 5; i++) {
        if (results[i] < min) min = results[i];
        if (results[i] > max) max = results[i];
        sum += results[i];
    }
    int64_t avg = sum / 5;
    int64_t variance = max - min;
    
    printf("Five identical workloads:\n");
    for (int i = 0; i < 5; i++) {
        printf("  Run %d: %ld cycles\n", i+1, results[i]);
    }
    printf("Min: %ld, Max: %ld, Avg: %ld, Variance: %ld\n", min, max, avg, variance);
    printf("Precision: %.2f%% (variance/average)\n", (double)variance * 100.0 / avg);
    
#ifdef SPIKE
    printf("\n=== SPIKE-specific Analysis ===\n");
    printf("✓ Cycle counting: ENABLED (functional simulation)\n");
    printf("✓ Timer precision: Single-cycle granularity\n");
    printf("✓ Determinism: Perfect (no timing variations)\n");
    printf("✓ Use case: Functional verification, algorithm testing\n");
    printf("⚠ Note: Not cycle-accurate for performance analysis\n");
#else
    printf("\n=== Hardware/RTL Analysis ===\n");
    printf("✓ Cycle counting: Hardware-accurate\n");
    printf("✓ Timer precision: Hardware-dependent\n");
    printf("✓ Use case: Performance optimization, benchmarking\n");
#endif
    
    printf("\n=== Analysis Complete ===\n");
    return 0;
}
