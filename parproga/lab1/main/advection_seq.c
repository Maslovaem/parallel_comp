#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 

const double A = 1.0;
const double X_END = 1.0;
const double T_END = 2.0;
const int M_GLOBAL = 101;
const int K_GLOBAL = 20000;
double phi(double x) { return sin(2.0 * M_PI * x / X_END); }
double psi(double t) { return sin(-2.0 * M_PI * A * t / X_END); }
double f(double t, double x) { return 0.0; }
double exact_solution(double t, double x) { return sin(2.0 * M_PI * (x - A * t) / X_END); }

int main() {
    double h = X_END / (M_GLOBAL - 1);
    double tau = T_END / K_GLOBAL;
    double sigma = A * tau / h;
    if (sigma > 1.0 + 1e-9) { return 1; }

    double *u_curr = (double*)malloc(M_GLOBAL * sizeof(double));
    double *u_next = (double*)malloc(M_GLOBAL * sizeof(double));
    if (!u_curr || !u_next) { return 2; }

    for (int m = 0; m < M_GLOBAL; ++m) u_curr[m] = phi(m * h);

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (int k = 0; k < K_GLOBAL; ++k) {
        double t_curr = k * tau;
        u_curr[0] = psi(t_curr); 
        for (int m = 1; m < M_GLOBAL; ++m) {
            double x = m * h;
            double f_val = f(t_curr, x);
            u_next[m] = u_curr[m] - sigma * (u_curr[m] - u_curr[m-1]) + tau * f_val;
        }
         double *temp = u_curr; u_curr = u_next; u_next = temp;
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    double max_error = 0.0;
    for (int m = 0; m < M_GLOBAL; ++m) {
        double error = fabs(u_curr[m] - exact_solution(T_END, m * h));
        if (error > max_error) max_error = error;
    }

    printf("Время выполнения: %.6f сек\n", elapsed_time);
    printf("Макс. ошибка: %.5e\n", max_error);

    FILE *fp = fopen("solution_seq.dat", "w");
    if (fp == NULL) {
        fprintf(stderr, "Ошибка открытия файла solution_seq.dat для записи!\n");
    } else {
        fprintf(fp, "# x\t u_numerical(T=%.2f)\t u_exact(T=%.2f)\n", T_END, T_END);
        for (int m = 0; m < M_GLOBAL; ++m) {
            double x = m * h;
            fprintf(fp, "%.6f\t%.15e\t%.15e\n", x, u_curr[m], exact_solution(T_END, x));
        }
        fclose(fp);
    }

    free(u_curr);
    free(u_next);

    return 0;
}
