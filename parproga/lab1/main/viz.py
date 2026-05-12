#!/usr/bin/env python3

import subprocess
import re
import matplotlib.pyplot as plt
import numpy as np
import os

processes = [1, 2, 4, 8]
k_values = [20000, 40000, 80000, 160000]

seq_exec = "./advection_seq"
mpi_exec = "./advection_mpi"

def get_time_from_output(output, is_mpi=False):
    if is_mpi:
        match = re.search(r'Макс\.\s+время\s+\(Tp\):\s+([\d.]+)', output)
    else:
        match = re.search(r'Время выполнения:\s+([\d.]+)', output)
    return float(match.group(1)) if match else None

def run_seq(k):
    with open("advection_seq.c", "r") as f:
        content = f.read()
    content = re.sub(r'(const int K_GLOBAL = )\d+;', rf'\g<1>{k};', content)
    with open("advection_seq_temp.c", "w") as f:
        f.write(content)
    
    subprocess.run(f"gcc -o advection_seq_temp advection_seq_temp.c -lm", shell=True, capture_output=True)
    result = subprocess.run("./advection_seq_temp", shell=True, capture_output=True, text=True)
    return get_time_from_output(result.stdout, is_mpi=False)

def run_mpi(p, k):
    with open("advection_mpi.c", "r") as f:
        content = f.read()
    content = re.sub(r'(const int K_GLOBAL = )\d+;', rf'\g<1>{k};', content)
    with open("advection_mpi_temp.c", "w") as f:
        f.write(content)
    
    subprocess.run(f"mpicc -o advection_mpi_temp advection_mpi_temp.c -lm", shell=True, capture_output=True)
    
    cmd = f"mpirun -n {p} ./advection_mpi_temp"
    if os.geteuid() == 0:
        cmd = f"mpirun -n {p} --allow-run-as-root ./advection_mpi_temp"
    
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return get_time_from_output(result.stdout, is_mpi=True)

seq_times = {}
mpi_times = {}

for k in k_values:
    seq_times[k] = run_seq(k)
    
    mpi_times[k] = {}
    for p in processes:
        mpi_times[k][p] = run_mpi(p, k)

fig, axes = plt.subplots(2, 2, figsize=(12, 10))

for k in k_values:
    speedup = []
    for p in processes:
        if mpi_times[k][p] and seq_times[k]:
            speedup.append(seq_times[k] / mpi_times[k][p])
        else:
            speedup.append(0)
    
    axes[0, 0].plot(processes, speedup, 'o-', label=f'K={k}')
    axes[0, 1].plot(processes, [s/p for s in speedup], 'o-', label=f'K={k}')

axes[0, 0].set_xlabel('Число процессов (p)')
axes[0, 0].set_ylabel('Ускорение (S)')
axes[0, 0].set_title('Ускорение от числа процессов')
axes[0, 0].legend()
axes[0, 0].grid(True)
axes[0, 0].plot(processes, processes, 'k--', alpha=0.3, label='Идеальное')

axes[0, 1].set_xlabel('Число процессов (p)')
axes[0, 1].set_ylabel('Эффективность (E)')
axes[0, 1].set_title('Эффективность от числа процессов')
axes[0, 1].legend()
axes[0, 1].grid(True)

p_fixed = 4
speedup_vs_k = []
efficiency_vs_k = []

for k in k_values:
    if mpi_times[k][p_fixed] and seq_times[k]:
        s = seq_times[k] / mpi_times[k][p_fixed]
        speedup_vs_k.append(s)
        efficiency_vs_k.append(s / p_fixed)
    else:
        speedup_vs_k.append(0)
        efficiency_vs_k.append(0)

axes[1, 0].plot(k_values, speedup_vs_k, 'o-', color='green', linewidth=2)
axes[1, 0].set_xlabel('Шаг сетки по времени (K)')
axes[1, 0].set_ylabel('Ускорение (S)')
axes[1, 0].set_title(f'Ускорение от K (p={p_fixed})')
axes[1, 0].grid(True)

axes[1, 1].plot(k_values, efficiency_vs_k, 'o-', color='red', linewidth=2)
axes[1, 1].set_xlabel('Шаг сетки по времени (K)')
axes[1, 1].set_ylabel('Эффективность (E)')
axes[1, 1].set_title(f'Эффективность от K (p={p_fixed})')
axes[1, 1].grid(True)

plt.tight_layout()
plt.savefig('benchmark_results.png', dpi=150)
plt.show()

for k in k_values:
    for p in processes:
        if mpi_times[k][p]:
            s = seq_times[k] / mpi_times[k][p]
            e = s / p
