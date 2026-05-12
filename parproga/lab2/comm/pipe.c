#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/wait.h> 
#include <time.h>

#define MESSAGE_SIZE 1 
#define N_ROUND_TRIPS 100000 

int main() {
    int pipe_p2c[2]; 
    int pipe_c2p[2]; 
    pid_t pid;
    char buffer[MESSAGE_SIZE];
    struct timespec start_time, end_time;
    long long total_nsec = 0;

    if (pipe(pipe_p2c) == -1 || pipe(pipe_c2p) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork(); 

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { 
        close(pipe_p2c[1]); 
        close(pipe_c2p[0]); 

        for (int i = 0; i < N_ROUND_TRIPS; ++i) {
            if (read(pipe_p2c[0], buffer, MESSAGE_SIZE) != MESSAGE_SIZE) {
                perror("Child read"); break;
            }
            if (write(pipe_c2p[1], buffer, MESSAGE_SIZE) != MESSAGE_SIZE) {
                perror("Child write"); break;
            }
        }

        close(pipe_p2c[0]);
        close(pipe_c2p[1]);
        exit(EXIT_SUCCESS); 

    } else { 
        close(pipe_p2c[0]); 
        close(pipe_c2p[1]); 

        for (int i = 0; i < N_ROUND_TRIPS; ++i) {
            clock_gettime(CLOCK_MONOTONIC, &start_time);
            if (write(pipe_p2c[1], buffer, MESSAGE_SIZE) != MESSAGE_SIZE) {
                 perror("Parent write"); break;
            }
            if (read(pipe_c2p[0], buffer, MESSAGE_SIZE) != MESSAGE_SIZE) {
                 perror("Parent read"); break;
            }
            clock_gettime(CLOCK_MONOTONIC, &end_time);
            total_nsec += (end_time.tv_sec - start_time.tv_sec) * 1000000000LL +
                          (end_time.tv_nsec - start_time.tv_nsec);
        }

        close(pipe_p2c[1]);
        close(pipe_c2p[0]);
        wait(NULL); 

        double avg_rtt_sec = (double)total_nsec / N_ROUND_TRIPS / 1e9;
        double avg_latency_sec = avg_rtt_sec / 2.0;

        printf("Среднее RTT: %.9f сек (%.6f мс)\n", avg_rtt_sec, avg_rtt_sec * 1000.0);
        printf("Время коммуникации: %.9f сек (%.6f мс)\n", avg_latency_sec, avg_latency_sec * 1000.0);
    }

    return 0;
}
