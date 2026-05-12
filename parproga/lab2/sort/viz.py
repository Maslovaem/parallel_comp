import subprocess
import matplotlib.pyplot as plt
import re

def compile_programs():
    subprocess.run(["gcc", "-o", "seq_qsort", "seq_qsort.c", "-lm"], capture_output=True)
    subprocess.run(["gcc", "-o", "parallel_mergesort", "parallel_mergesort_pthread.c", "-lpthread"], capture_output=True)

def run_seq_qsort(n):
    result = subprocess.run(["./seq_qsort", str(n)], capture_output=True, text=True)
    match = re.search(r"(\d+\.\d+) секунд", result.stdout)
    return float(match.group(1)) if match else None

def run_parallel_mergesort(n, threads):
    result = subprocess.run(["./parallel_mergesort", str(n), str(threads)], capture_output=True, text=True)
    match = re.search(r"(\d+\.\d+) секунд", result.stdout)
    return float(match.group(1)) if match else None

def plot_speedup_vs_size():
    sizes = [1000000, 5000000, 10000000, 20000000, 50000000]
    threads = 4
    seq_times = []
    par_times = []
    speedups = []
    
    for n in sizes:
        t_seq = run_seq_qsort(n)
        t_par = run_parallel_mergesort(n, threads)
        if t_seq and t_par:
            seq_times.append(t_seq)
            par_times.append(t_par)
            speedups.append(t_seq / t_par)
    
    plt.figure(figsize=(10, 6))
    plt.plot(sizes, speedups, 'bo-', linewidth=2, markersize=8)
    plt.xlabel("Размер массива", fontsize=12)
    plt.ylabel("Ускорение", fontsize=12)
    plt.title(f"Зависимость ускорения от размера массива (потоков={threads})", fontsize=14)
    plt.grid(True, alpha=0.3)
    plt.xscale("log")
    for i, (x, y) in enumerate(zip(sizes, speedups)):
        plt.annotate(f"{y:.2f}", (x, y), textcoords="offset points", xytext=(0, 10), ha='center')
    plt.savefig("speedup_vs_size.png", dpi=150)
    plt.show()

def plot_speedup_vs_threads():
    n = 20000000
    threads_list = [1, 2, 4, 8, 16]
    seq_time = run_seq_qsort(n)
    if not seq_time:
        return
    
    par_times = []
    speedups = []
    
    for t in threads_list:
        t_par = run_parallel_mergesort(n, t)
        if t_par:
            par_times.append(t_par)
            speedups.append(seq_time / t_par)
    
    plt.figure(figsize=(10, 6))
    plt.plot(threads_list, speedups, 'rs-', linewidth=2, markersize=8)
    plt.plot(threads_list, threads_list, 'k--', alpha=0.5, label="Идеальное ускорение")
    plt.xlabel("Количество потоков", fontsize=12)
    plt.ylabel("Ускорение", fontsize=12)
    plt.title(f"Зависимость ускорения от числа потоков (размер={n})", fontsize=14)
    plt.grid(True, alpha=0.3)
    for i, (x, y) in enumerate(zip(threads_list, speedups)):
        plt.annotate(f"{y:.2f}", (x, y), textcoords="offset points", xytext=(0, 10), ha='center')
    plt.legend()
    plt.savefig("speedup_vs_threads.png", dpi=150)
    plt.show()

def main():
    compile_programs()
    plot_speedup_vs_size()
    plot_speedup_vs_threads()

if __name__ == "__main__":
    main()
