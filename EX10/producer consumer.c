#define _POSIX_C_SOURCE 200809L // Fixes the strdup warning/segfault
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define MAX_LINE 1024
#define QUEUE_SIZE 10

char *queue[QUEUE_SIZE];
int head = 0, tail = 0, count = 0;
int finished_producers = 0;
int total_producers = 3;
omp_lock_t queue_lock;

void produce(const char *filename) {
    int tid = omp_get_thread_num();
    FILE *file = fopen(filename, "r");
    if (!file) {
        omp_set_lock(&queue_lock);
        finished_producers++;
        omp_unset_lock(&queue_lock);
        return;
    }

    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, file)) {
        int added = 0;
        while (!added) {
            omp_set_lock(&queue_lock);
            if (count < QUEUE_SIZE) {
                queue[tail] = strdup(line);
                printf("[Thread %d] Producer produced: %s", tid, line);
                tail = (tail + 1) % QUEUE_SIZE;
                count++;
                added = 1;
            }
            omp_unset_lock(&queue_lock);
        }
    }
    fclose(file);

    omp_set_lock(&queue_lock);
    finished_producers++;
    omp_unset_lock(&queue_lock);
}

void consume() {
    int tid = omp_get_thread_num();
    while (1) {
        char *line = NULL;
        int all_done = 0;

        omp_set_lock(&queue_lock);
        if (count > 0) {
            line = queue[head];
            head = (head + 1) % QUEUE_SIZE;
            count--;
        } else if (finished_producers == total_producers) {
            all_done = 1;
        }
        omp_unset_lock(&queue_lock);

        if (line) {
            printf("[Thread %d] Consumer consuming: %s", tid, line);
            char *saveptr; // Thread-safe strtok
            char *token = strtok_r(line, " \t\n", &saveptr);
            while (token) {
                printf("  -> %s\n", token);
                token = strtok_r(NULL, " \t\n", &saveptr);
            }
            free(line);
        } else if (all_done) {
            break;
        }
    }
}

int main() {
    char *files[] = {"file1.txt", "file2.txt", "file3.txt"};
    omp_init_lock(&queue_lock);

    #pragma omp parallel num_threads(6)
    {
        #pragma omp single
        {
            for (int i = 0; i < total_producers; i++) {
                #pragma omp task firstprivate(i)
                produce(files[i]);
            }

            for (int i = 0; i < 3; i++) {
                #pragma omp task
                consume();
            }
        }
    }

    omp_destroy_lock(&queue_lock);
    return 0;
}
