#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define MAX_TASKS 200000
double func(double x) { if (x == 0.0) return 0.0; return sin(1.0 / x); }
typedef struct { double a; double b; double tolerance; } Task;
Task task_queue[MAX_TASKS];
int task_count = 0;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER; 
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER; 

double global_integral_sum = 0.0;
pthread_mutex_t sum_mutex = PTHREAD_MUTEX_INITIALIZER;

int tasks_in_progress = 0; 
bool shutdown_flag = false; 
pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER; 

double simpson(double (*f)(double), double a, double b, int n_intervals) { 
    if (n_intervals % 2 != 0) n_intervals++;
    double h = (b - a) / n_intervals;
    double sum = f(a) + f(b);
    for (int i = 1; i < n_intervals; i += 2) sum += 4.0 * f(a + i * h);
    for (int i = 2; i < n_intervals - 1; i += 2) sum += 2.0 * f(a + i * h);
    return (h / 3.0) * sum;
 }

bool process_task(Task current_task) {
    double a = current_task.a;
    double b = current_task.b;
    double tolerance = current_task.tolerance;

    double integral1 = simpson(func, a, b, 2);
    double integral2 = simpson(func, a, b, 4);
    double error_estimate = fabs(integral1 - integral2) / 15.0;

    if (error_estimate < tolerance || (b - a) < 1e-12) {
        pthread_mutex_lock(&sum_mutex);
        global_integral_sum += integral2;
        pthread_mutex_unlock(&sum_mutex);
        return true; 
    } else {
        double mid = a + (b - a) / 2.0;
        Task task_left = {a, mid, tolerance / 2.0};
        Task task_right = {mid, b, tolerance / 2.0};

        pthread_mutex_lock(&queue_mutex);
        bool success = true;
        if (task_count + 1 >= MAX_TASKS) {
             pthread_mutex_unlock(&queue_mutex); 
             pthread_mutex_lock(&sum_mutex);
             global_integral_sum += integral2;
             pthread_mutex_unlock(&sum_mutex);
             success = false; 
        } else {
             task_queue[task_count++] = task_left;
             task_queue[task_count++] = task_right;
             pthread_cond_signal(&queue_not_empty); 
             pthread_mutex_unlock(&queue_mutex);
        }
        return success; 
    }
}

void* worker_thread_func(void* arg) {
    Task current_task;
    bool task_found;

    while (true) {
        task_found = false;
        pthread_mutex_lock(&queue_mutex); 

        while (task_count == 0) {
            pthread_mutex_lock(&state_mutex);
            bool should_shutdown = shutdown_flag;
            pthread_mutex_unlock(&state_mutex);
            if (should_shutdown) {
                pthread_mutex_unlock(&queue_mutex); 
                return NULL; 
            }
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }

        current_task = task_queue[--task_count];
        pthread_mutex_unlock(&queue_mutex); 

        pthread_mutex_lock(&state_mutex);
        tasks_in_progress++;
        pthread_mutex_unlock(&state_mutex);

        process_task(current_task);

        pthread_mutex_lock(&state_mutex);
        tasks_in_progress--;
        if (tasks_in_progress == 0) {
             pthread_mutex_lock(&queue_mutex); 
             bool queue_really_empty = (task_count == 0);
             pthread_mutex_unlock(&queue_mutex);
             if (queue_really_empty) {
                  pthread_cond_signal(&queue_cond);
             }
        }
        pthread_mutex_unlock(&state_mutex);

    } 
    return NULL;
}

int main(int argc, char *argv[]) {
    double a_global, b_global, tolerance;
    int num_threads;
    struct timespec start_time, end_time;

    if (argc < 5) { return 1;}
    a_global = atof(argv[1]);
    b_global = atof(argv[2]);
    tolerance = atof(argv[3]);
    num_threads = atoi(argv[4]);
    if (a_global <= 0 || b_global <= a_global || tolerance <= 0 || num_threads <= 0) { return 1;}

    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    if (!threads) { perror("malloc threads"); return 1; }

    Task initial_task = {a_global, b_global, tolerance};
    task_queue[task_count++] = initial_task; 

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (int i = 0; i < num_threads; ++i) {
        if (pthread_create(&threads[i], NULL, worker_thread_func, NULL) != 0) { exit(EXIT_FAILURE);}
    }

    pthread_mutex_lock(&state_mutex); 
    while (true) {
        pthread_mutex_lock(&queue_mutex); 
        bool queue_empty = (task_count == 0);
        pthread_mutex_unlock(&queue_mutex);

        if (queue_empty && tasks_in_progress == 0) {
            break; 
        }
        pthread_cond_wait(&queue_cond, &state_mutex);
    }
    
    shutdown_flag = true;
    pthread_mutex_unlock(&state_mutex); 

    pthread_mutex_lock(&queue_mutex);
    pthread_cond_broadcast(&queue_not_empty);
    pthread_mutex_unlock(&queue_mutex);

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Значение интеграла: %.15f\n", global_integral_sum);
    printf("Время выполнения: %.6f секунд\n", elapsed_time);

    free(threads);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&sum_mutex);
    pthread_mutex_destroy(&state_mutex);
    pthread_cond_destroy(&queue_not_empty);
    pthread_cond_destroy(&queue_cond);

    return 0;
}
