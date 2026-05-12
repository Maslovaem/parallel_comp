#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define N_ROUND_TRIPS 100000

char shared_buffer; 
int turn = 0;       
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_ping = PTHREAD_COND_INITIALIZER; 
pthread_cond_t cond_pong = PTHREAD_COND_INITIALIZER; 

void* pong_thread_func(void* arg) {
    for (int i = 0; i < N_ROUND_TRIPS; ++i) {
        pthread_mutex_lock(&mutex);
        while (turn != 1) {
            pthread_cond_wait(&cond_ping, &mutex);
        }
        turn = 0; 
        pthread_cond_signal(&cond_pong);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_t pong_thread;
    struct timespec start_time, end_time;
    long long total_nsec = 0;

    printf("Измерение времени коммуникации через общую память \n");
    if (pthread_create(&pong_thread, NULL, pong_thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < N_ROUND_TRIPS; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        pthread_mutex_lock(&mutex);
        while (turn != 0) { 
             pthread_cond_wait(&cond_pong, &mutex); 
        }
        turn = 1; 
        pthread_cond_signal(&cond_ping);

        while (turn != 0) {
            pthread_cond_wait(&cond_pong, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        clock_gettime(CLOCK_MONOTONIC, &end_time);
        total_nsec += (end_time.tv_sec - start_time.tv_sec) * 1000000000LL +
                      (end_time.tv_nsec - start_time.tv_nsec);
    }

    if (pthread_join(pong_thread, NULL) != 0) {
        perror("pthread_join");
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_ping);
    pthread_cond_destroy(&cond_pong);

    double avg_rtt_sec = (double)total_nsec / N_ROUND_TRIPS / 1e9;
    double avg_latency_sec = avg_rtt_sec / 2.0;

    printf("Среднее RTT: %.9f сек (%.6f мс, %.3f мкс)\n", avg_rtt_sec, avg_rtt_sec * 1000.0, avg_rtt_sec * 1000000.0);
    printf("Время коммуникации: %.9f сек (%.6f мс, %.3f мкс)\n", avg_latency_sec, avg_latency_sec * 1000.0, avg_latency_sec * 1000000.0);


    return 0;
}
