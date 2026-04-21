#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <mpi.h>
#include <sys/time.h>

#define MAX_STRING 100
#define TYPE1 1
#define TYPE2 2

int main(void) {
    char message[MAX_STRING];
    char result[MAX_STRING];
    int comm_sz, my_rank, msg_type;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /* Worker processes */
    if (my_rank != 0) {
        if (my_rank % 2 != 0) {
            /* Odd rank → Type 1 */
            strcpy(message, "mepco");
            msg_type = TYPE1;
        } else {
            /* Even rank → Type 2 */
            strcpy(message, "dhoni");
            msg_type = TYPE2;
        }

        MPI_Send(message, strlen(message) + 1, MPI_CHAR, 0, msg_type, MPI_COMM_WORLD);
    } else {
        /* Master process */
        for (int q = 1; q < comm_sz; q++) {
            MPI_Status status;
            MPI_Recv(message, MAX_STRING, MPI_CHAR, q, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            msg_type = status.MPI_TAG;

            double time_used;
            struct timeval start, end;

            if (msg_type == TYPE1) {
                /* Convert to uppercase */
                gettimeofday(&start, NULL);
                
              
                for (int i = 0; message[i] != '\0'; i++) {
                    result[i] = toupper(message[i]);
                }
                result[strlen(message)] = '\0';

                gettimeofday(&end, NULL);
                time_used = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec); // Microseconds
                time_used *= 1e-3; // Convert to milliseconds

                printf("From process %d (Type 1): %s. Execution Time: %f milliseconds\n", q, result, time_used);
            } else if (msg_type == TYPE2) {
                /* Palindrome check */
                gettimeofday(&start, NULL);
                
               

                int len = strlen(message);
                int flag = 1;

                for (int i = 0; i < len / 2; i++) {
                    if (message[i] != message[len - i - 1]) {
                        flag = 0;
                        break;
                    }
                }

                gettimeofday(&end, NULL);
                time_used = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec); // Microseconds
                time_used *= 1e-3; // Convert to milliseconds

                if (flag)
                    printf("From process %d (Type 2): %s is a Palindrome. Execution Time: %f milliseconds\n", q, message, time_used);
                else
                    printf("From process %d (Type 2): %s is NOT a Palindrome. Execution Time: %f milliseconds\n", q, message, time_used);
            }
        }
    }

    MPI_Finalize();
    return 0;
}
