#!/usr/bin/env python3

import subprocess
import sys
import os

def generate_benchmark_data():
    """Generate simulated benchmark data for Raw Throughput Ideality analysis"""
    
    # Define kernels and their characteristics
    kernels = {
        'fmatmul': {'sew': 4, 'complexity': 'high'},
        'dotproduct': {'sew': 4, 'complexity': 'low'},
        'fdotproduct': {'sew': 4, 'complexity': 'low'},
        'fconv2d': {'sew': 4, 'complexity': 'medium'},
        'exp': {'sew': 4, 'complexity': 'high'},
        'softmax': {'sew': 4, 'complexity': 'high'},
        'pathfinder': {'sew': 4, 'complexity': 'medium'},
        'jacobi2d': {'sew': 4, 'complexity': 'medium'},
        'fft': {'sew': 4, 'complexity': 'high'},
        'dwt': {'sew': 4, 'complexity': 'medium'},
        'roi_align': {'sew': 4, 'complexity': 'medium'}
    }
    
    lane_configs = [2, 4, 8, 16]
    vector_lengths = [32, 64, 128, 256, 512, 1024]  # in bytes
    
    results = []
    
    for kernel_name, kernel_info in kernels.items():
        sew = kernel_info['sew']
        
        for lanes in lane_configs:
            for vlen_bytes in vector_lengths:
                # Calculate vsize (number of elements that fit in vector length)
                vsize = vlen_bytes // sew
                
                # Generate realistic cycle counts based on kernel complexity
                # and system parameters (lanes, vector size)
                cycles = estimate_cycles(kernel_name, lanes, vsize, kernel_info['complexity'])
                
                if cycles is None:
                    continue
                    
                # Prepare metadata and arguments for performance script
                metadata, args = prepare_kernel_args(kernel_name, lanes, vsize, sew)
                
                if metadata and args:
                    try:
                        # Add ideal_dispatcher as '1' to metadata for complete output format
                        metadata_with_disp = metadata + ' 1'
                        
                        # Call the performance script to get full performance metrics
                        # With extra arguments (dcache_stall, icache_stall, sb_full) to get full output
                        cmd = [sys.executable, 'scripts/performance.py', metadata_with_disp, args, str(cycles), '0', '0', '0']
                        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
                        
                        output = result.stdout.strip()
                        if output:
                            results.append(output)
                            
                    except subprocess.CalledProcessError:
                        # Skip failed calculations
                        continue
    
    return results

def estimate_cycles(kernel, lanes, vsize, complexity):
    """Estimate realistic cycle counts for different kernels"""
    
    # Base efficiency factors based on complexity
    efficiency_factors = {
        'low': 0.8,      # Simple operations like dotproduct
        'medium': 0.6,   # Moderate complexity like conv2d
        'high': 0.4      # Complex operations like fft, exp
    }
    
    efficiency = efficiency_factors.get(complexity, 0.5)
    
    if kernel == 'fmatmul':
        # Matrix multiply scales as O(N^3)
        size = max(4, int(vsize**0.5))
        ideal_cycles = (2 * size**3) // (lanes * 8)  # 8 elements per lane for 32-bit
        return max(100, int(ideal_cycles / efficiency))
        
    elif kernel in ['dotproduct', 'fdotproduct']:
        # Linear in vector size
        ideal_cycles = (2 * vsize) // (lanes * 8)
        return max(50, int(ideal_cycles / efficiency))
        
    elif kernel == 'fconv2d':
        size = max(4, int(vsize**0.5))
        filter_size = 3
        ideal_cycles = (2 * filter_size**2 * size**2) // (lanes * 8)
        return max(100, int(ideal_cycles / efficiency))
        
    elif kernel == 'fft':
        # FFT is O(N log N)
        if vsize < 4:
            return None
        log_factor = max(1, vsize.bit_length() - 1)
        ideal_cycles = (5 * vsize * log_factor) // (lanes * 8)
        return max(100, int(ideal_cycles / efficiency))
        
    elif kernel == 'exp':
        # Exponential requires many operations per element
        ideal_cycles = (30 * vsize) // (lanes * 8)
        return max(100, int(ideal_cycles / efficiency))
        
    elif kernel == 'softmax':
        # Softmax is compute intensive
        ideal_cycles = (34 * vsize) // (lanes * 8)
        return max(100, int(ideal_cycles / efficiency))
        
    elif kernel == 'pathfinder':
        cols = max(4, int(vsize**0.5))
        ideal_cycles = (3 * (cols-1)**2) // (lanes * 8)
        return max(100, int(ideal_cycles / efficiency))
        
    elif kernel == 'jacobi2d':
        size = max(4, int(vsize**0.5))
        ideal_cycles = (10 * (size-1)**2) // (lanes * 8)
        return max(100, int(ideal_cycles / efficiency))
        
    elif kernel == 'dwt':
        if vsize < 4:
            return None
        ideal_cycles = (3 * vsize) // (lanes * 8)
        return max(50, int(ideal_cycles / efficiency))
        
    elif kernel == 'roi_align':
        depth = min(64, vsize)
        ideal_cycles = (9 * depth) // (lanes * 8)
        return max(100, int(ideal_cycles / efficiency))
    
    return None

def prepare_kernel_args(kernel, lanes, vsize, sew):
    """Prepare metadata and arguments for each kernel type"""
    
    if kernel == 'fmatmul':
        size = max(4, int(vsize**0.5))
        metadata = f'{kernel} {lanes} {vsize} {sew}'
        args = f'{size} {size} {size}'
        
    elif kernel in ['dotproduct', 'fdotproduct']:
        metadata = f'{kernel} {lanes} {vsize} {sew}'
        args = f'{vsize}'
        
    elif kernel == 'fconv2d':
        size = max(4, int(vsize**0.5))
        filter_size = 3
        metadata = f'{kernel} {lanes} {vsize} {sew}'
        args = f'{size} {filter_size}'
        
    elif kernel == 'fft':
        if vsize < 4:
            return None, None
        metadata = f'{kernel} {lanes} {vsize} {sew}'
        args = f'{vsize} float32'
        
    elif kernel == 'exp':
        metadata = f'{kernel} {lanes} {vsize} {sew}'
        args = f'{vsize}'
        
    elif kernel == 'softmax':
        channels = max(1, vsize // 32)
        insize = max(32, vsize // channels) if channels > 0 else vsize
        metadata = f'{kernel} {lanes} {insize} {sew}'
        args = f'{channels} {insize}'
        
    elif kernel == 'pathfinder':
        cols = max(4, int(vsize**0.5))
        rows = cols
        runs = 1
        metadata = f'{kernel} {lanes} {cols} {sew}'
        args = f'{runs} {cols} {rows}'
        
    elif kernel == 'jacobi2d':
        size = max(4, int(vsize**0.5))
        metadata = f'{kernel} {lanes} {vsize} {sew}'
        args = f'{size} 0'
        
    elif kernel == 'dwt':
        if vsize < 4:
            return None, None
        metadata = f'{kernel} {lanes} {vsize} {sew}'
        args = f'{vsize}'
        
    elif kernel == 'roi_align':
        batch, depth, height, width = 1, min(64, vsize), 32, 32
        n_boxes, crop_h, crop_w = 8, 7, 7
        metadata = f'{kernel} {lanes} {depth} {sew}'
        args = f'{batch} {depth} {height} {width} {n_boxes} {crop_h} {crop_w}'
    
    else:
        return None, None
    
    return metadata, args

def main():
    print("Generating comprehensive benchmark data...")
    results = generate_benchmark_data()
    
    # Save results to files for different lane configurations
    for lanes in [2, 4, 8, 16]:
        filename = f'benchmark_results_{lanes}_lanes.txt'
        with open(filename, 'w') as f:
            f.write(f"# Benchmark results for {lanes}-lane Ara configuration\n")
            f.write("# Format: kernel lanes vsize sew perf max_perf ideal_disp\n")
            for result in results:
                if f' {lanes} ' in result:
                    f.write(result + '\n')
        print(f"Generated {filename}")
    
    # Also save combined results
    with open('benchmark_results_combined.txt', 'w') as f:
        f.write("# Combined benchmark results for all lane configurations\n")
        f.write("# Format: kernel lanes vsize sew perf max_perf ideal_disp\n")
        for result in results:
            f.write(result + '\n')
    
    print(f"Generated benchmark_results_combined.txt with {len(results)} entries")
    return results

if __name__ == '__main__':
    main()
