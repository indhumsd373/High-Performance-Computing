#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main()
{
    int i, j, rows, cols;
    int fd[2];
    double serial_time, parent_time, child_time, parallel_time;
    struct timespec s1, s2, p1, p2;

    printf("Enter number of rows: ");
    scanf("%d", &rows);

    printf("Enter number of columns: ");
    scanf("%d", &cols);

    int A[rows][cols], B[rows][cols];
    int C[rows][cols], D[rows][cols];

    srand(time(NULL));

    /* -------- GENERATE MATRICES -------- */
    printf("\nMatrix A:\n");
    for(i = 0; i < rows; i++)
    {
        for(j = 0; j < cols; j++)
        {
            A[i][j] = rand() % 10;
            printf("%d ", A[i][j]);
        }
        printf("\n");
    }

    printf("\nMatrix B:\n");
    for(i = 0; i < rows; i++)
    {
        for(j = 0; j < cols; j++)
        {
            B[i][j] = rand() % 10;
            printf("%d ", B[i][j]);
        }
        printf("\n");
    }

    /* -------- SERIAL EXECUTION -------- */
    clock_gettime(CLOCK_MONOTONIC, &s1);

    for(i = 0; i < rows; i++)
        for(j = 0; j < cols; j++)
            C[i][j] = A[i][j] + B[i][j];

    for(i = 0; i < rows; i++)
        for(j = 0; j < cols; j++)
            D[i][j] = A[i][j] - B[i][j];

    clock_gettime(CLOCK_MONOTONIC, &s2);

    serial_time =
        (s2.tv_sec - s1.tv_sec) * 1000.0 +
        (s2.tv_nsec - s1.tv_nsec) / 1e6;

    printf("\nSerial Addition Result (C = A + B):\n");
    for(i = 0; i < rows; i++)
    {
        for(j = 0; j < cols; j++)
            printf("%d ", C[i][j]);
        printf("\n");
    }

    printf("\nSerial Subtraction Result (D = A - B):\n");
    for(i = 0; i < rows; i++)
    {
        for(j = 0; j < cols; j++)
            printf("%d ", D[i][j]);
        printf("\n");
    }

    /* -------- PARALLEL EXECUTION -------- */
    pipe(fd);
    pid_t pid = fork();

    if(pid == 0)   /* CHILD → SUBTRACTION */
    {
        close(fd[0]);

        clock_gettime(CLOCK_MONOTONIC, &p1);

        for(i = 0; i < rows; i++)
            for(j = 0; j < cols; j++)
                D[i][j] = A[i][j] - B[i][j];

        clock_gettime(CLOCK_MONOTONIC, &p2);

        child_time =
            (p2.tv_sec - p1.tv_sec) * 1000.0 +
            (p2.tv_nsec - p1.tv_nsec) / 1e6;

        write(fd[1], &child_time, sizeof(double));
        close(fd[1]);

        printf("\nParallel Subtraction Result (Child):\n");
        for(i = 0; i < rows; i++)
        {
            for(j = 0; j < cols; j++)
                printf("%d ", D[i][j]);
            printf("\n");
        }
        return 0;
    }
    else          /* PARENT → ADDITION */
    {
        close(fd[1]);

        clock_gettime(CLOCK_MONOTONIC, &p1);

        for(i = 0; i < rows; i++)
            for(j = 0; j < cols; j++)
                C[i][j] = A[i][j] + B[i][j];

        clock_gettime(CLOCK_MONOTONIC, &p2);

        parent_time =
            (p2.tv_sec - p1.tv_sec) * 1000.0 +
            (p2.tv_nsec - p1.tv_nsec) / 1e6;

        read(fd[0], &child_time, sizeof(double));
        close(fd[0]);
        wait(NULL);

        printf("\nParallel Addition Result (Parent):\n");
        for(i = 0; i < rows; i++)
        {
            for(j = 0; j < cols; j++)
                printf("%d ", C[i][j]);
            printf("\n");
        }

        parallel_time = (parent_time > child_time)
                        ? parent_time : child_time;
    }

    printf("\nSerial Execution Time   : %.4f ms", serial_time);
    printf("\nParallel Execution Time : %.4f ms\n", parallel_time);

    return 0;
}
