#!/usr/bin/env python3

import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
import argparse

def create_heatmap(input_file, output_file, lane_config=None):
    """Create Raw Throughput Ideality heatmap from benchmark results"""
    
    # Read the data
    data = []
    with open(input_file, 'r') as f:
        for line in f:
            if line.startswith('#') or not line.strip():
                continue
            parts = line.strip().split()
            if len(parts) >= 6:
                kernel = parts[0]
                lanes = int(parts[1])
                vsize = int(parts[2])  # Vector size (elements)
                sew = int(parts[3])    # Size element width (bytes)
                perf = float(parts[4])  # Actual performance
                max_perf = float(parts[5])  # Maximum theoretical performance
                
                # Calculate vector length in bytes and Raw Throughput Ideality
                vlen_bytes = vsize * sew
                raw_throughput_ideality = perf / max_perf if max_perf > 0 else 0
                
                data.append({
                    'kernel': kernel,
                    'lanes': lanes,
                    'vsize_elements': vsize,
                    'sew': sew,
                    'vlen_bytes': vlen_bytes,
                    'perf': perf,
                    'max_perf': max_perf,
                    'raw_throughput_ideality': raw_throughput_ideality
                })
    
    # Convert to DataFrame
    df = pd.DataFrame(data)
    
    if df.empty:
        print("No data found in input file")
        return
    
    # Filter by lane configuration if specified
    if lane_config:
        df = df[df['lanes'] == lane_config]
        title_suffix = f" ({lane_config}-lane configuration)"
    else:
        title_suffix = ""
    
    # Create pivot table for heatmap
    # Use common vector lengths across kernels
    common_vlens = sorted(df['vlen_bytes'].value_counts().index)
    
    # Filter to kernels and vector lengths that have data
    pivot_data = []
    kernels = sorted(df['kernel'].unique())
    
    for kernel in kernels:
        kernel_data = df[df['kernel'] == kernel]
        row = {}
        for vlen in common_vlens:
            vlen_data = kernel_data[kernel_data['vlen_bytes'] == vlen]
            if not vlen_data.empty:
                # Use the mean if there are multiple entries
                row[vlen] = vlen_data['raw_throughput_ideality'].mean()
            else:
                row[vlen] = np.nan
        pivot_data.append(row)
    
    # Create DataFrame from pivot data
    heatmap_df = pd.DataFrame(pivot_data, index=kernels)
    heatmap_df = heatmap_df.reindex(columns=sorted(heatmap_df.columns))
    
    # Create the heatmap
    plt.figure(figsize=(12, 8))
    
    # Use a mask for NaN values
    mask = heatmap_df.isnull()
    
    sns.heatmap(heatmap_df, 
                annot=True, 
                fmt='.3f', 
                cmap='RdYlGn', 
                vmin=0.0, 
                vmax=1.0,
                mask=mask,
                cbar_kws={'label': 'Raw Throughput Ideality'},
                square=False)
    
    plt.title(f'Raw Throughput Ideality Heatmap{title_suffix}', fontsize=14, fontweight='bold')
    plt.xlabel('Vector Length [Bytes]', fontsize=12)
    plt.ylabel('Kernel', fontsize=12)
    plt.tight_layout()
    
    # Save the plot
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"Heatmap saved to {output_file}")
    
    # Print some statistics
    print(f"\nStatistics for {len(kernels)} kernels:")
    print(f"Vector lengths: {sorted(common_vlens)} bytes")
    print(f"Mean Raw Throughput Ideality: {heatmap_df.mean().mean():.3f}")
    print(f"Max Raw Throughput Ideality: {heatmap_df.max().max():.3f}")
    print(f"Min Raw Throughput Ideality: {heatmap_df.min().min():.3f}")

def main():
    parser = argparse.ArgumentParser(description='Create Raw Throughput Ideality heatmaps')
    parser.add_argument('-f', '--input', required=True, help='Input benchmark results file')
    parser.add_argument('-o', '--output', required=True, help='Output heatmap image file')
    parser.add_argument('-l', '--lanes', type=int, help='Filter by lane configuration')
    
    args = parser.parse_args()
    
    create_heatmap(args.input, args.output, args.lanes)

if __name__ == '__main__':
    main()
