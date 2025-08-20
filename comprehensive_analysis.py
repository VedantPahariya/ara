#!/usr/bin/env python3

import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np

def create_lane_comparison_heatmap():
    """Create a comprehensive comparison heatmap for all lane configurations"""
    
    # Read the combined data
    data = []
    with open('benchmark_results_combined.txt', 'r') as f:
        for line in f:
            if line.startswith('#') or not line.strip():
                continue
            parts = line.strip().split()
            if len(parts) >= 6:
                kernel = parts[0]
                lanes = int(parts[1])
                vsize = int(parts[2])
                sew = int(parts[3])
                perf = float(parts[4])
                max_perf = float(parts[5])
                
                vlen_bytes = vsize * sew
                raw_throughput_ideality = perf / max_perf if max_perf > 0 else 0
                
                data.append({
                    'kernel': kernel,
                    'lanes': lanes,
                    'vlen_bytes': vlen_bytes,
                    'raw_throughput_ideality': raw_throughput_ideality
                })
    
    df = pd.DataFrame(data)
    
    # Create subplots for each lane configuration
    fig, axes = plt.subplots(2, 2, figsize=(20, 16))
    fig.suptitle('Raw Throughput Ideality: Ara Vector Processor Performance Analysis', fontsize=16, fontweight='bold')
    
    lane_configs = [2, 4, 8, 16]
    colors = ['Blues', 'Greens', 'Oranges', 'Reds']
    
    for idx, (lanes, cmap) in enumerate(zip(lane_configs, colors)):
        row = idx // 2
        col = idx % 2
        ax = axes[row, col]
        
        # Filter data for this lane configuration
        lane_data = df[df['lanes'] == lanes]
        
        # Create pivot table
        pivot = lane_data.pivot_table(
            index='kernel', 
            columns='vlen_bytes', 
            values='raw_throughput_ideality', 
            aggfunc='mean'
        )
        
        # Sort by kernel name and vector length
        pivot = pivot.sort_index()
        pivot = pivot.reindex(columns=sorted(pivot.columns))
        
        # Create heatmap
        sns.heatmap(pivot, 
                    annot=True, 
                    fmt='.3f', 
                    cmap=cmap,
                    vmin=0.0,
                    vmax=2.0,  # Normalize scale for comparison
                    ax=ax,
                    cbar_kws={'label': 'Raw Throughput Ideality'})
        
        ax.set_title(f'{lanes}-Lane Configuration', fontweight='bold')
        ax.set_xlabel('Vector Length [Bytes]')
        ax.set_ylabel('Kernel' if col == 0 else '')
        
        # Statistics
        mean_ideality = pivot.mean().mean()
        max_ideality = pivot.max().max()
        ax.text(0.02, 0.98, f'Mean: {mean_ideality:.3f}\nMax: {max_ideality:.3f}', 
                transform=ax.transAxes, verticalalignment='top',
                bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))
    
    plt.tight_layout()
    plt.savefig('raw_throughput_ideality_comparison.png', dpi=300, bbox_inches='tight')
    print("Comparison heatmap saved to raw_throughput_ideality_comparison.png")

def analyze_performance_trends():
    """Analyze performance trends across different configurations"""
    
    # Read data
    data = []
    with open('benchmark_results_combined.txt', 'r') as f:
        for line in f:
            if line.startswith('#') or not line.strip():
                continue
            parts = line.strip().split()
            if len(parts) >= 6:
                data.append({
                    'kernel': parts[0],
                    'lanes': int(parts[1]),
                    'vsize': int(parts[2]),
                    'sew': int(parts[3]),
                    'perf': float(parts[4]),
                    'max_perf': float(parts[5]),
                    'vlen_bytes': int(parts[2]) * int(parts[3]),
                    'raw_throughput_ideality': float(parts[4]) / float(parts[5]) if float(parts[5]) > 0 else 0
                })
    
    df = pd.DataFrame(data)
    
    print("=== RAW THROUGHPUT IDEALITY ANALYSIS ===")
    print(f"Total benchmarks: {len(df)}")
    print(f"Kernels: {sorted(df['kernel'].unique())}")
    print(f"Lane configurations: {sorted(df['lanes'].unique())}")
    print(f"Vector lengths: {sorted(df['vlen_bytes'].unique())} bytes")
    
    print("\n=== PERFORMANCE BY LANE CONFIGURATION ===")
    for lanes in sorted(df['lanes'].unique()):
        lane_data = df[df['lanes'] == lanes]
        print(f"{lanes} lanes:")
        print(f"  Mean Raw Throughput Ideality: {lane_data['raw_throughput_ideality'].mean():.3f}")
        print(f"  Max Raw Throughput Ideality: {lane_data['raw_throughput_ideality'].max():.3f}")
        print(f"  Std Dev: {lane_data['raw_throughput_ideality'].std():.3f}")
        
        # Find best performing kernel
        best_kernel = lane_data.loc[lane_data['raw_throughput_ideality'].idxmax()]
        print(f"  Best: {best_kernel['kernel']} @ {best_kernel['vlen_bytes']}B -> {best_kernel['raw_throughput_ideality']:.3f}")
    
    print("\n=== PERFORMANCE BY KERNEL ===")
    for kernel in sorted(df['kernel'].unique()):
        kernel_data = df[df['kernel'] == kernel]
        print(f"{kernel}:")
        print(f"  Mean Raw Throughput Ideality: {kernel_data['raw_throughput_ideality'].mean():.3f}")
        print(f"  Best configuration: {kernel_data['lanes'].iloc[kernel_data['raw_throughput_ideality'].argmax()]} lanes @ {kernel_data['vlen_bytes'].iloc[kernel_data['raw_throughput_ideality'].argmax()]}B")

if __name__ == '__main__':
    create_lane_comparison_heatmap()
    print("\n")
    analyze_performance_trends()
