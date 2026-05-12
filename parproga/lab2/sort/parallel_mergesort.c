#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h> 

typedef struct {
    int *array;         
    size_t size;        
    int threads_avail;  
} SortArgs;

int compare_int(const void *a, const void *b) {
    int int_a = *((int*)a);
    int int_b = *((int*)b);
    if (int_a == int_b) return 0;
    else if (int_a < int_b) return -1;
    else return 1;
}

void merge(int *array, size_t left_size, size_t right_size) {
    size_t total_size = left_size + right_size;
    int *left_part = array;
    int *right_part = array + left_size;
    int *temp_buffer = (int*)malloc(total_size * sizeof(int));
    if (!temp_buffer) {
        perror("Failed to allocate temp buffer in merge");
        return;
    }

    size_t i = 0, j = 0, k = 0;
    while (i < left_size && j < right_size) {
        if (left_part[i] <= right_part[j]) {
            temp_buffer[k++] = left_part[i++];
        } else {
            temp_buffer[k++] = right_part[j++];
        }
    }
    while (i < left_size) temp_buffer[k++] = left_part[i++];
    while (j < right_size) temp_buffer[k++] = right_part[j++];

    memcpy(array, temp_buffer, total_size * sizeof(int));
    free(temp_buffer);
}

#define QSQRT_THRESHOLD 1000

void* parallel_merge_sort_thread(void* arg) {
    SortArgs *args = (SortArgs*)arg;
    int *array = args->array;
    size_t size = args->size;
    int threads = args->threads_avail;

    if (size <= QSQRT_THRESHOLD || threads <= 1) {
        qsort(array, size, sizeof(int), compare_int);
        return NULL;
    }

    size_t mid = size / 2;
    pthread_t right_thread_id;
    int right_threads = threads / 2;
    int left_threads = threads - right_threads; 

    SortArgs right_args;
    right_args.array = array + mid;
    right_args.size = size - mid;
    right_args.threads_avail = right_threads;

    int rc = pthread_create(&right_thread_id, NULL, parallel_merge_sort_thread, &right_args);
    if (rc) {
        fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
        qsort(array, size, sizeof(int), compare_int); 
        return NULL;
    }

    SortArgs left_args;
    left_args.array = array;
    left_args.size = mid;
    left_args.threads_avail = left_threads;
    parallel_merge_sort_thread(&left_args);

    rc = pthread_join(right_thread_id, NULL);
    if (rc) {
        fprintf(stderr, "ERROR; return code from pthread_join() is %d\n", rc);
    }

    merge(array, mid, size - mid);

    return NULL;
}

int main(int argc, char *argv[]) {
    int n_global;
    int num_threads;
    int *array = NULL;
    struct timespec start_time, end_time;

    if (argc < 3) {
        return 1;
    }
    n_global = atoi(argv[1]);
    num_threads = atoi(argv[2]);
    if (n_global <= 0 || num_threads <= 0) {
        return 1;
    }
    printf("Параллельная Pthread сортировка слиянием, N = %d, Потоков = %d\n", n_global, num_threads);

    array = (int*)malloc(n_global * sizeof(int));
    if (!array) { fprintf(stderr,"Malloc failed\n"); return 1; }

    srand(time(NULL));
    for (int i = 0; i < n_global; ++i) array[i] = rand() % (n_global * 10);

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    SortArgs root_args;
    root_args.array = array;
    root_args.size = n_global;
    root_args.threads_avail = num_threads;
    parallel_merge_sort_thread(&root_args); 

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Время сортировки (Tp): %.6f секунд\n", elapsed_time);
    free(array);
    return 0;
}
