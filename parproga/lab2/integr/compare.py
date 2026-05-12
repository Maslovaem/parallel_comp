import subprocess
import numpy as np
from scipy import integrate
import re
import os

def compile_program():
    if not os.path.exists("./integral"):
        subprocess.run(["gcc", "-o", "adaptive_integral", "adaptive_integral_pthread.c", "-lm", "-lpthread"])

def run_my_integral(a, b, tol, threads):
    result = subprocess.run(["./integral", str(a), str(b), str(tol), str(threads)], 
                           capture_output=True, text=True)
    print(result.stdout)
    match = re.search(r"[-+]?\d+\.\d+", result.stdout)
    if match:
        return float(match.group(0))
    return None

def run_scipy_integral(a, b):
    def func(x):
        if x == 0:
            return 0.0
        return np.sin(1.0 / x)
    result, error = integrate.quad(func, a, b)
    return result, error

def main():
    compile_program()
    
    a = 0.1
    b = 1.0
    tol = 1e-6
    threads = 4
    
    print(f"Параметры: a={a}, b={b}, tolerance={tol}, threads={threads}")
    
    my_result = run_my_integral(a, b, tol, threads)
    scipy_result, scipy_error = run_scipy_integral(a, b)
    
    if my_result is None:
        return
    
    print(f"integral.c:    {my_result:.15f}")
    print(f"SciPy:   {scipy_result:.15f}")
    
    diff = abs(my_result - scipy_result)
    print(f"Разница:          {diff:.2e}")
    
    test_intervals = [(0.1, 0.5), (0.5, 1.0), (0.2, 0.8), (0.01, 0.1)]
    
    for a_test, b_test in test_intervals:
        my_val = run_my_integral(a_test, b_test, 1e-6, 4)
        scipy_val, _ = run_scipy_integral(a_test, b_test)
        if my_val is not None and scipy_val is not None:
            print(f"[{a_test}, {b_test}]: integral.c={my_val:.8f}, SciPy={scipy_val:.8f}, Разница={abs(my_val-scipy_val):.2e}")

if __name__ == "__main__":
    main()
