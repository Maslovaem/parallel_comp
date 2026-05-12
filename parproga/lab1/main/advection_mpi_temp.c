#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

const double A = 1.0;
const double X_END = 1.0;
const double T_END = 2.0;
const int M_GLOBAL = 101;
const int K_GLOBAL = 160000;
double phi(double x) { return sin(2.0 * M_PI * x / X_END); }
double psi(double t) { return sin(-2.0 * M_PI * A * t / X_END); }
double f(double t, double x) { return 0.0; }
double exact_solution(double t, double x) { return sin(2.0 * M_PI * (x - A * t) / X_END); }

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double h = X_END / (M_GLOBAL - 1);
    double tau = T_END / K_GLOBAL;
    double sigma = A * tau / h;
    if (rank == 0) {  }
    if (sigma > 1.0 + 1e-9) { MPI_Abort(MPI_COMM_WORLD, 1); }

    int total_internal_points = M_GLOBAL - 1;
    int points_per_proc = total_internal_points / size;
    int remainder = total_internal_points % size;
    int local_points;
    int start_global_m; 
    if (rank < remainder) {
        local_points = points_per_proc + 1;
        start_global_m = rank * local_points + 1;
    } else {
        local_points = points_per_proc;
        start_global_m = rank * points_per_proc + remainder + 1;
    }
    int local_M = local_points + 1; 

    double *u_curr = (double*)malloc(local_M * sizeof(double));
    double *u_next = (double*)malloc(local_M * sizeof(double));
    if (!u_curr || !u_next) { MPI_Abort(MPI_COMM_WORLD, 2); }

    for (int i = 1; i < local_M; ++i) u_curr[i] = phi((start_global_m + i - 1) * h);

    MPI_Request send_request = MPI_REQUEST_NULL, recv_request = MPI_REQUEST_NULL;
    MPI_Status status;
    double start_wtime, end_wtime, local_elapsed_time, max_elapsed_time;
    MPI_Barrier(MPI_COMM_WORLD);
    start_wtime = MPI_Wtime();
    for (int k = 0; k < K_GLOBAL; ++k) {
        double t_curr = k * tau;
        int left_neighbor = (rank == 0) ? MPI_PROC_NULL : rank - 1;
        int right_neighbor = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;
        if (right_neighbor != MPI_PROC_NULL) { MPI_Isend(&u_curr[local_M - 1], 1, MPI_DOUBLE, right_neighbor, k, MPI_COMM_WORLD, &send_request); }
        if (left_neighbor != MPI_PROC_NULL) { MPI_Irecv(&u_curr[0], 1, MPI_DOUBLE, left_neighbor, k, MPI_COMM_WORLD, &recv_request); }
        if (right_neighbor != MPI_PROC_NULL) { MPI_Wait(&send_request, MPI_STATUS_IGNORE); }
        if (left_neighbor != MPI_PROC_NULL) { MPI_Wait(&recv_request, MPI_STATUS_IGNORE); }
        
	if (rank == 0) { u_curr[0] = psi(t_curr); }
        for (int i = 1; i < local_M; ++i) {
            double x = (start_global_m + i - 1) * h;
            double f_val = f(t_curr, x);
            u_next[i] = u_curr[i] - sigma * (u_curr[i] - u_curr[i-1]) + tau * f_val;
        }
        double *temp = u_curr; u_curr = u_next; u_next = temp;
    }
    end_wtime = MPI_Wtime();
    local_elapsed_time = end_wtime - start_wtime;

    double max_error = 0.0;
    double local_max_error = 0.0;
    for (int i = 1; i < local_M; ++i) { 
	 double error = fabs(u_curr[i] - exact_solution(T_END, (start_global_m + i - 1) * h));
         if (error > local_max_error) local_max_error = error;
    }
    MPI_Reduce(&local_max_error, &max_error, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local_elapsed_time, &max_elapsed_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    double *global_solution_internal = NULL; 
    int *recvcounts = NULL;
    int *displs = NULL;

    if (rank == 0) {
        global_solution_internal = (double*)malloc(total_internal_points * sizeof(double));
        recvcounts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));
        if (!global_solution_internal || !recvcounts || !displs) {
             free(global_solution_internal); free(recvcounts); free(displs);
             MPI_Abort(MPI_COMM_WORLD, 3);
        }

        int current_displ = 0;
        for (int i = 0; i < size; ++i) {
            int points_i; 
            if (i < remainder) {
                points_i = points_per_proc + 1;
            } else {
                points_i = points_per_proc;
            }
            recvcounts[i] = points_i;
            displs[i] = current_displ;
            current_displ += points_i;
        }
    }

    MPI_Gatherv(
        &u_curr[1],           
        local_points,         
        MPI_DOUBLE,           
        global_solution_internal, 
        recvcounts,           
        displs,              
        MPI_DOUBLE,           
        0,                    
        MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Макс. время (Tp): %.6f сек\n", max_elapsed_time);
        printf("Макс. ошибка: %.5e\n", max_error);

        FILE *fp = fopen("solution_mpi.dat", "w");
        if (fp == NULL) {
        } else {
            fprintf(fp, "# x\t u_numerical(T=%.2f)\t u_exact(T=%.2f) (MPI, %d procs)\n", T_END, T_END, size);
            double x0 = 0.0;
            double u0_num = psi(T_END); 
            double u0_exact = exact_solution(T_END, x0);
            fprintf(fp, "%.6f\t%.15e\t%.15e\n", x0, u0_num, u0_exact);

            for (int m_internal = 0; m_internal < total_internal_points; ++m_internal) {
                int global_m = m_internal + 1; 
                double x = global_m * h;
                double u_num = global_solution_internal[m_internal];
                double u_exact = exact_solution(T_END, x);
                fprintf(fp, "%.6f\t%.15e\t%.15e\n", x, u_num, u_exact);
            }
            fclose(fp);
        }

        free(global_solution_internal);
        free(recvcounts);
        free(displs);
    }

    free(u_curr);
    free(u_next);

    MPI_Finalize();
    return 0;
}
