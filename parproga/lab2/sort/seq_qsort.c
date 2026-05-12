#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int compare_int(const void *a, const void *b) {
    int int_a = *((int*)a);
    int int_b = *((int*)b);
    if (int_a == int_b) return 0;
    else if (int_a < int_b) return -1;
    else return 1;
}

int main(int argc, char *argv[]) {
    int n_global;
    int *array = NULL;
    struct timespec start_time, end_time; 

    if (argc < 2) {
        n_global = 10000000;
    } else {
        n_global = atoi(argv[1]);
        if (n_global <= 0) { fprintf(stderr,"N > 0\n"); return 1; }
    }

    array = (int*)malloc(n_global * sizeof(int));
    if (!array) { fprintf(stderr,"Malloc failed\n"); return 1; }

    srand(time(NULL));
    for (int i = 0; i < n_global; ++i) array[i] = rand() % (n_global * 10);

    printf("Сортировка qsort...\n");
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    qsort(array, n_global, sizeof(int), compare_int);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("Время сортировки (T1): %.6f секунд\n", elapsed_time);

    free(array);
    return 0;
}
