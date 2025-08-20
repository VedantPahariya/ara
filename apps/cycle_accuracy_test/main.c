// Simple cycle counter accuracy test
#include <stdint.h>
#include "runtime.h"

#ifdef SPIKE
#include <stdio.h>
#else
#include "printf.h"
#endif

int main() {
    printf("===================\n");
    printf("= Cycle Test Suite =\n");
    printf("===================\n\n");
    
    // Test 1: Basic cycle counter functionality
    printf("Test 1: Basic cycle counter functionality\n");
    int64_t cycles1 = get_cycle_count();
    int64_t cycles2 = get_cycle_count();
    printf("Cycle count 1: %ld\n", cycles1);
    printf("Cycle count 2: %ld\n", cycles2);
    printf("Delta: %ld cycles\n", cycles2 - cycles1);
    
    // Test 2: Timer function accuracy  
    printf("\nTest 2: Timer function accuracy\n");
    start_timer();
    
    // Simple computation loop
    volatile int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += i;
    }
    
    stop_timer();
    int64_t measured_cycles = get_timer();
    printf("Measured cycles for 1000 iterations: %ld\n", measured_cycles);
    printf("Computed sum (verification): %d\n", sum);
    
    // Test 3: Multiple measurements for consistency
    printf("\nTest 3: Consistency check (5 measurements)\n");
    for (int test = 0; test < 5; test++) {
        start_timer();
        
        volatile int temp_sum = 0;
        for (int i = 0; i < 100; i++) {
            temp_sum += i * i;
        }
        
        stop_timer();
        int64_t test_cycles = get_timer();
        printf("Test %d: %ld cycles (sum=%d)\n", test + 1, test_cycles, temp_sum);
    }
    
    // Test 4: Minimum cycle resolution
    printf("\nTest 4: Minimum cycle resolution\n");
    start_timer();
    // Minimal operation
    volatile int x = 1;
    x++;
    stop_timer();
    int64_t minimal_cycles = get_timer();
    printf("Minimal operation cycles: %ld (x=%d)\n", minimal_cycles, x);
    
    printf("\n=== Cycle Test Complete ===\n");
    return 0;
}
