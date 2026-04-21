#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

int main(int argc, char** argv) {
    int my_rank, comm_sz, ser_num, local_n, i;
    int n = 20;
    int *array = NULL;
    int *sub_array = NULL;
    int local_count = 0, total_count = 0;

    struct timespec start, end;
    long long local_time, max_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    local_n = n / comm_sz;
    sub_array = (int*)malloc(local_n * sizeof(int));

    if (my_rank == 0) {
        array = (int*)malloc(n * sizeof(int));
        srand(time(NULL));

        printf("Enter number to find frequency (0-9): ");
        if (scanf("%d", &ser_num) != 1) ser_num = 5;

        printf("Array elements: ");
        for (i = 0; i < n; i++) {
            array[i] = rand() % 10;
            printf("%d ", array[i]);
        }
        printf("\n");
    }

    // Broadcast search number
    MPI_Bcast(&ser_num, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Scatter data
    MPI_Scatter(array, local_n, MPI_INT,
                sub_array, local_n, MPI_INT,
                0, MPI_COMM_WORLD);

    // Start timing (each process)
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Local computation
    for (i = 0; i < local_n; i++) {
        if (sub_array[i] == ser_num)
            local_count++;
    }

    // Stop timing
    clock_gettime(CLOCK_MONOTONIC, &end);

    local_time = (end.tv_sec - start.tv_sec) * 1000000000LL +
                 (end.tv_nsec - start.tv_nsec);

    // Print each process result
    printf("Process %d -> Local Count: %d, Execution Time: %lld ns\n",
           my_rank, local_count, local_time);

    // Find total frequency
    MPI_Reduce(&local_count, &total_count, 1, MPI_INT,
               MPI_SUM, 0, MPI_COMM_WORLD);

    // Find maximum execution time (parallel time)
    MPI_Reduce(&local_time, &max_time, 1, MPI_LONG_LONG,
               MPI_MAX, 0, MPI_COMM_WORLD);

    if (my_rank == 0) {
        printf("\nTotal frequency of %d is %d\n",
               ser_num, total_count);

        printf("Parallel Execution Time (Max of all processes): %lld ns\n",
               max_time);

        free(array);
    }

    free(sub_array);
    MPI_Finalize();
    return 0;
}
