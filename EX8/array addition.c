#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char* argv[]) {
    int n;                  // Size of arrays
    int thread_count;       // Number of threads
    char *end;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <thread_count>\n", argv[0]);
        return 1;
    }

    long t = strtol(argv[1], &end, 10);
    if (*end != '\0' || t <= 0) {
        fprintf(stderr, "Invalid thread count: %s\n", argv[1]);
        return 1;
    }
    thread_count = (int)t;

    printf("Enter size of arrays:\n");
    if (scanf("%d", &n) != 1 || n <= 0) {
        fprintf(stderr, "Invalid array size\n");
        return 1;
    }

    double* A = (double*) malloc((size_t)n * sizeof(double));
    double* B = (double*) malloc((size_t)n * sizeof(double));
    double* C = (double*) malloc((size_t)n * sizeof(double));

    if (!A || !B || !C) {
        perror("malloc");
        free(A); free(B); free(C);
        return 1;
    }

    for (int i = 0; i < n; i++) {
        A[i] = (double)i;
        B[i] = (double)(i * 2);
    }

    #pragma omp parallel for num_threads(thread_count)
    for (int i = 0; i < n; i++) {
        C[i] = A[i] + B[i];
    }

    printf("result array:\n");
    for (int i = 0; i < n && i < n; i++) {
        printf("C[%d] = %.2f\t", i, C[i]);
    }

    free(A);
    free(B);
    free(C);

    return 0;
}
