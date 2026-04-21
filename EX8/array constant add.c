#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char* argv[]) {
    int n;                  // Size of array
    int thread_count;       // Number of threads
    double constant;        // Value to add

    // Get number of threads from command line
    thread_count = strtol(argv[1], NULL, 10);

    printf("Enter size of array:\n");
    scanf("%d", &n);

    printf("Enter constant value to add:\n");
    scanf("%lf", &constant);

    // Allocate memory
    double* A = (double*) malloc(n * sizeof(double));

    // Initialize array
    for (int i = 0; i < n; i++) {
        A[i] = i * 1.0;
    }

    // Parallel loop to add constant
    #pragma omp parallel for num_threads(thread_count)
    for (int i = 0; i < n; i++) {
        A[i] = A[i] + constant;
    }

    // Print first few elements to verify
    printf("elements after addition:\n");
    for (int i = 0; i < n && i < n; i++) {
        printf("A[%d] = %.2f\t", i, A[i]);
    }

    // Free memory
    free(A);

    return 0;
}
