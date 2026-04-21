#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define BUFFER_SIZE 5
#define MAX_WORD_LEN 50
#define NUM_WORKERS 3
#define MAX_DICT_SIZE 100

// --- Input Buffer Data ---
char input_words_buf[BUFFER_SIZE][MAX_WORD_LEN];
int in_count = 0, in_head = 0, in_tail = 0;
pthread_mutex_t in_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t in_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t in_not_empty = PTHREAD_COND_INITIALIZER;

// --- Result Buffer Data ---
char res_words_buf[BUFFER_SIZE][MAX_WORD_LEN];
int res_status_buf[BUFFER_SIZE]; // 1 for Correct, 0 for Incorrect
int res_count = 0, res_head = 0, res_tail = 0;
pthread_mutex_t res_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t res_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t res_not_empty = PTHREAD_COND_INITIALIZER;

// --- Dictionary and Global Control ---
char dictionary[MAX_DICT_SIZE][MAX_WORD_LEN];
int dict_size = 0;
int total_to_process = 0;
int producer_done = 0; // Flag to signal workers to exit

void* spell_checker(void* arg) {
    while (1) {
        char current_word[MAX_WORD_LEN];

        // 1. Fetch from input buffer
        pthread_mutex_lock(&in_lock);
        while (in_count == 0 && !producer_done) {
            pthread_cond_wait(&in_not_empty, &in_lock);
        }

        // Check if finished: no words left and producer is done
        if (in_count == 0 && producer_done) {
            pthread_mutex_unlock(&in_lock);
            break; 
        }

        strcpy(current_word, input_words_buf[in_head]);
        in_head = (in_head + 1) % BUFFER_SIZE;
        in_count--;
        pthread_cond_signal(&in_not_full);
        pthread_mutex_unlock(&in_lock);

        // 2. Spell Check against user dictionary
        int is_correct = 0;
        for (int i = 0; i < dict_size; i++) {
            if (strcmp(current_word, dictionary[i]) == 0) {
                is_correct = 1;
                break;
            }
        }

        // 3. Put in result buffer
        pthread_mutex_lock(&res_lock);
        while (res_count == BUFFER_SIZE) {
            pthread_cond_wait(&res_not_full, &res_lock);
        }
        strcpy(res_words_buf[res_tail], current_word);
        res_status_buf[res_tail] = is_correct;
        res_tail = (res_tail + 1) % BUFFER_SIZE;
        res_count++;
        pthread_cond_signal(&res_not_empty);
        pthread_mutex_unlock(&res_lock);
    }
    return NULL;
}

int main() {
    // Get Dictionary from User
    printf("Enter number of words in dictionary: ");
    if (scanf("%d", &dict_size) != 1) return 1;
    for (int i = 0; i < dict_size; i++) {
        printf("Enter dictionary word %d: ", i + 1);
        scanf("%s", dictionary[i]);
    }

    // Get Input Words from User
    printf("\nEnter number of words to check: ");
    if (scanf("%d", &total_to_process) != 1) return 1;
    char check_words[total_to_process][MAX_WORD_LEN];
    for (int i = 0; i < total_to_process; i++) {
        printf("Enter word to check %d: ", i + 1);
        scanf("%s", check_words[i]);
    }

    pthread_t workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) 
        pthread_create(&workers[i], NULL, spell_checker, NULL);

    // Master: Produce words into buffer
    for (int i = 0; i < total_to_process; i++) {
        pthread_mutex_lock(&in_lock);
        while (in_count == BUFFER_SIZE) {
            pthread_cond_wait(&in_not_full, &in_lock);
        }
        strcpy(input_words_buf[in_tail], check_words[i]);
        in_tail = (in_tail + 1) % BUFFER_SIZE;
        in_count++;
        pthread_cond_signal(&in_not_empty); // Fixed: Removed second argument
        pthread_mutex_unlock(&in_lock);
    }

    // Signal workers that production is complete
    pthread_mutex_lock(&in_lock);
    producer_done = 1;
    pthread_cond_broadcast(&in_not_empty);
    pthread_mutex_unlock(&in_lock);

    // Server Printer: Consume and display results
    int printed = 0;
    while (printed < total_to_process) {
        pthread_mutex_lock(&res_lock);
        while (res_count == 0) {
            pthread_cond_wait(&res_not_empty, &res_lock);
        }
        printf("\nResult Word: %-10s | %s", 
               res_words_buf[res_head], 
               res_status_buf[res_head] ? "CORRECT" : "INCORRECT");
        
        res_head = (res_head + 1) % BUFFER_SIZE;
        res_count--;
        printed++;
        pthread_cond_signal(&res_not_full);
        pthread_mutex_unlock(&res_lock);
    }

    for (int i = 0; i < NUM_WORKERS; i++) pthread_join(workers[i], NULL);
    printf("\n\nProcessing complete.\n");

    return 0;
}
