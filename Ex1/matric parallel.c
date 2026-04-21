#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/time.h>
#include<time.h>

int main() {
    int N;
    printf("Enter the value of N for NxN matrix: ");
    scanf("%d", &N);
    int i,j,k;
    // Allocate memory for matrices
    int **A = (int **)malloc(N * sizeof(int *));
    int **B = (int **)malloc(N * sizeof(int *));
    int **C = (int **)malloc(N * sizeof(int *));
    int (*p2c)[2] = (int (*)[2])malloc(N * sizeof(int[2]));

    for (i = 0; i < N; i++) {
        A[i] = (int *)malloc(N * sizeof(int));
        B[i] = (int *)malloc(N * sizeof(int));
        C[i] = (int *)malloc(N * sizeof(int));
    }

    // Seed for random number generation
    //srand(time(0));
    printf("Matrix A:\n");
    // Generate random matrix A
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            A[i][j] = rand() % 10;
            printf("%d\t",A[i][j]);
        }
        printf("\n");
    }
   printf("Matrix B:\n");
    // Generate random matrix B
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            B[i][j] = rand() % 10;
            printf("%d\t",B[i][j]);
        }
        printf("\n");
    }

    // Create pipes
    for (i = 0; i < N; i++) {
        pipe(p2c[i]);
    }

    // Create child processes
    for (i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid == 0) { // Child process
            int *result = (int *)malloc(N * sizeof(int));
            struct timeval start, end;
            double time_used;
            int j,k;
            gettimeofday(&start, NULL);
            for (j = 0; j < N; j++) {
                result[j] = 0;
                for (k = 0; k < N; k++) {
                    result[j] += A[i][k] * B[k][j];
                }
            }
            gettimeofday(&end, NULL);

            time_used = (end.tv_sec - start.tv_sec) * 1e6;
            time_used = (time_used + (end.tv_usec - start.tv_usec)) * 1e-6;

            close(p2c[i][0]); // Close read end
            write(p2c[i][1], result, N * sizeof(int));
            write(p2c[i][1], &time_used, sizeof(time_used));
            close(p2c[i][1]); // Close write end
            free(result);
            return 0; // Exit child process
        }
    }

    // Parent process
    double max_time = 0;
    for (i = 0; i < N; i++) {
        int *result = (int *)malloc(N * sizeof(int));
        double child_time;
        int j;
        close(p2c[i][1]); // Close write end
        read(p2c[i][0], result, N * sizeof(int));
        read(p2c[i][0], &child_time, sizeof(child_time));
        for (j = 0; j < N; j++) {
            C[i][j] = result[j];
        }
        if (child_time > max_time)
            max_time = child_time;
        close(p2c[i][0]); // Close read end
        free(result);
    }

    // Wait for all child processes to finish
    for (i = 0; i < N; i++) {
        wait(NULL);
    }
    struct timeval s, e;
    double time_used;
    gettimeofday(&s, NULL);
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
                C[i][j] = 0;
                for (k = 0; k < N; k++) {
                   C[i][j] += A[i][k] * B[k][j];
                }
        }
    }
    gettimeofday(&e, NULL);
    time_used = (e.tv_sec - s.tv_sec) * 1e6;
    time_used = (time_used + (e.tv_usec - s.tv_usec)) * 1e-6;

    // Convert to milliseconds
    max_time *= 1000;
    time_used *= 1000;

    printf("PARALLEL EXECUTION: %f milliseconds\n", max_time);
    printf("SERIAL EXECUTION: %f milliseconds\n", time_used);
    printf("Resultant matrix:\n");
    for(i=0;i<N;i++)
    {
       for(j=0;j<N;j++)
       {
          printf("%d\t",C[i][j]);
       }
       printf("\n");
    }

    // Free allocated memory
    for (i = 0; i < N; i++) {
        free(A[i]);
        free(B[i]);
        free(C[i]);
    }
    free(A);
    free(B);
    free(C);
    free(p2c);

    return 0;
}
