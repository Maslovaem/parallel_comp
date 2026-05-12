#include <mpi.h>
#include <stdio.h>
#include <stdlib.h> 

#define N_REPETITIONS 1000
#define N_WARMUP 10

int main(int argc, char *argv[]) {
    int rank, size;
    double start_time, end_time, total_rtt = 0.0;
    double message_buffer; 
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2) {
        if (rank == 0) {
            fprintf(stderr, "Надо 2 процесса мин\n");
        }
        MPI_Finalize();
        exit(1); 
    }

    for (int i = 0; i < N_WARMUP; ++i) {
         if (rank == 0) {
            MPI_Send(&message_buffer, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&message_buffer, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
        } else if (rank == 1) {
            MPI_Recv(&message_buffer, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Send(&message_buffer, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < N_REPETITIONS; ++i) {
        if (rank == 0) {
            start_time = MPI_Wtime();
            MPI_Send(&message_buffer, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&message_buffer, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD, &status);
            end_time = MPI_Wtime();
            total_rtt += (end_time - start_time);

        } else if (rank == 1) {
            MPI_Recv(&message_buffer, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Send(&message_buffer, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }

    if (rank == 0) {
        if (N_REPETITIONS > 0) {
            double average_rtt = total_rtt / N_REPETITIONS;
            double average_latency = average_rtt / 2.0;
            printf("Число процессов: %d\n", size);
            printf("Измерялось между rank 0 и rank 1\n");
            printf("Среднее время: %.9f секунд (%.6f мс)\n",
                   average_rtt, average_rtt * 1000.0);
            printf("Оценка задержки:    %.9f секунд (%.6f мс)\n",
                   average_latency, average_latency * 1000.0);
        } else {
             printf("Количество повторений N_REPETITIONS равно 0, измерение не проводилось.\n");
        }
    }

    MPI_Finalize();
    return 0;
}
