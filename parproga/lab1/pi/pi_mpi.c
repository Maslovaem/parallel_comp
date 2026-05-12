#include <mpi.h>
#include <stdio.h>
#include <stdlib.h> 
#include <math.h>   

int main(int argc, char *argv[]) {
    int rank, size;
    int n_intervals; 
    double h;         
    double local_sum;  
    double global_pi;  
    double start_time, end_time, elapsed_time, max_elapsed_time;
    int i;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        if (argc < 2) {
            n_intervals = 100000000; 
            printf("Число интервалов N не указано, используется N = %d\n", n_intervals);
        } else {
            n_intervals = atoi(argv[1]); 
            if (n_intervals <= 0) {
                 fprintf(stderr, "Ошибка: Число интервалов N должно быть положительным целым числом.\n");
                 n_intervals = -1; 
            }
        }
    }

    MPI_Bcast(&n_intervals, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
	if (n_intervals <= 0) {
        MPI_Finalize();
        return 1;
    }

    MPI_Barrier(MPI_COMM_WORLD); 
    start_time = MPI_Wtime();

    h = 1.0 / (double)n_intervals; 
    local_sum = 0.0;

    for (i = rank; i < n_intervals; i += size) {
        double x = h * ((double)i + 0.5); 
        local_sum += 4.0 / (1.0 + x * x);
    }

    double local_pi = h * local_sum;

    MPI_Reduce(
        &local_pi,        
        &global_pi,      
        1,               
        MPI_DOUBLE,      
        MPI_SUM,         
        0,               
        MPI_COMM_WORLD   
    );

    end_time = MPI_Wtime();
    elapsed_time = end_time - start_time;

    MPI_Reduce(&elapsed_time, &max_elapsed_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Параллельное вычисление pi\n");
        printf("Число MPI процессов: %d\n", size);
        printf("Число подынтервалов (N): %d\n", n_intervals);
        printf("Вычисленное значение pi: %.16f\n", global_pi);
        printf("      Точное значение pi: %.16f\n", M_PI);
        printf("   Абсолютная погрешность: %.5e\n", fabs(global_pi - M_PI));
        printf(" Максимальное время счета: %.6f секунд\n", max_elapsed_time);
    }

    MPI_Finalize();
    return 0;
}
