#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#define BUFFER_SIZE 7
#define TOTAL_TASKS 25

// Global Buffer Data (No Structs)
int op_a[BUFFER_SIZE];
int op_b[BUFFER_SIZE];
char op_type[BUFFER_SIZE]; // '+', '-', '*', '/'
int head = 0, tail = 0;
int count = 0; // number of items currently in buffer
// Counters
int tasks_produced = 0;
int tasks_consumed = 0;

// Synchronization Primitives
sem_t empty_slots;  // Tracks available space in buffer
sem_t full_slots;   // Tracks items available to consume
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Shared file pointer
FILE *result_file;

void* compute_thread(void* arg) {
    char my_op = *((char*)arg);
    free(arg);

    while (1) {
        // Check for global completion
        pthread_mutex_lock(&buffer_mutex);
        if (tasks_consumed >= TOTAL_TASKS && tasks_produced >= TOTAL_TASKS) {
            pthread_mutex_unlock(&buffer_mutex);
            break;
        }
        pthread_mutex_unlock(&buffer_mutex);

        // Wait until there's at least one item
        if (sem_wait(&full_slots) != 0) continue;

        pthread_mutex_lock(&buffer_mutex);
        // Re-check completion to avoid blocking forever
        if (tasks_consumed >= TOTAL_TASKS && tasks_produced >= TOTAL_TASKS) {
            pthread_mutex_unlock(&buffer_mutex);
            sem_post(&full_slots);
            break;
        }

        // If buffer empty (race), release and continue
        if (count == 0) {
            pthread_mutex_unlock(&buffer_mutex);
            sem_post(&full_slots);
            continue;
        }

        // If head item matches this thread's op, consume it
        if (op_type[head] == my_op) {
            int a = op_a[head];
            int b = op_b[head];
            char op = op_type[head];

            head = (head + 1) % BUFFER_SIZE;
            count--;
            tasks_consumed++;

            pthread_mutex_unlock(&buffer_mutex);
            sem_post(&empty_slots);

            // Perform calculation
            double result = 0;
            if (op == '+') result = a + b;
            else if (op == '-') result = a - b;
            else if (op == '*') result = a * b;
            else if (op == '/') result = (b != 0) ? (double)a / b : 0;

            // Write to Result File and print which thread (op) processed it
            pthread_mutex_lock(&file_mutex);
            fprintf(result_file, "Thread[%c]: %d %c %d = %.2f\n", op, a, op, b, result);
            fflush(result_file);
            pthread_mutex_unlock(&file_mutex);

            printf("Thread[%c] processed: %d %c %d = %.2f\n", op, a, op, b, result);

        } else {
            // Not my operation: rotate the head item to tail (preserve order) and try again.
            // Move one item from head to tail.
            int a = op_a[head];
            int b = op_b[head];
            char op = op_type[head];

            head = (head + 1) % BUFFER_SIZE;
            // place at tail
            op_a[tail] = a;
            op_b[tail] = b;
            op_type[tail] = op;
            tail = (tail + 1) % BUFFER_SIZE;
            // count remains same
            pthread_mutex_unlock(&buffer_mutex);

            // Repost full_slots because item still in buffer
            sem_post(&full_slots);

            // tiny pause to avoid tight rotation loop
            usleep(1000);
        }
    }

    return NULL;
}

int main() {
    char ops[] = {'+', '-', '*', '/'};
    pthread_t threads[4];
    result_file = fopen("results.txt", "w");
    if (!result_file) {
        perror("fopen");
        return 1;
    }

    // Initialize Semaphores
    sem_init(&empty_slots, 0, BUFFER_SIZE);
    sem_init(&full_slots, 0, 0);

    // Create 4 computing threads with separate args
    for (int i = 0; i < 4; i++) {
        char *arg = malloc(sizeof(char));
        *arg = ops[i];
        pthread_create(&threads[i], NULL, compute_thread, arg);
    }

    // Server: Generate random work
    srand((unsigned)time(NULL));
    for (int i = 0; i < TOTAL_TASKS; i++) {
        int a = rand() % 100;
        int b = rand() % 100;
        char op = ops[rand() % 4];

        sem_wait(&empty_slots);
        pthread_mutex_lock(&buffer_mutex);

        op_a[tail] = a;
        op_b[tail] = b;
        op_type[tail] = op;
        tail = (tail + 1) % BUFFER_SIZE;
        count++;
        tasks_produced++;

        pthread_mutex_unlock(&buffer_mutex);
        sem_post(&full_slots);
    }

    // Wait for workers to finish
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Server finished placing %d tasks. Checking result_file...\n", TOTAL_TASKS);

    fclose(result_file);
    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    return 0;
}

