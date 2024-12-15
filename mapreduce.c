#include "mapreduce.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <locale.h>
#include <unistd.h>

int NUM_MAPPERS;
int NUM_REDUCERS;
int num_files;
char **input_files;
InvertedIndex global_inverted_index;

pthread_mutex_t file_queue_lock = PTHREAD_MUTEX_INITIALIZER;
int next_file_index = 0;

pthread_mutex_t mapper_done_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mapper_done_cond = PTHREAD_COND_INITIALIZER;
int mappers_done_count = 0;

void normalize_word(char *w) {
    if (w == NULL) {
        return;
    }

    int write_index = 0;
    for (int read_index = 0; w[read_index] != '\0'; read_index++) {
        char c = w[read_index];

        if (isalpha((unsigned char)c)) {
            w[write_index] = (char)tolower((unsigned char)c);
            write_index++;
        }
    }

    w[write_index] = '\0';
}


void process_file(const char *filename, int file_id, InvertedIndex *temp) {
    if (!filename || !temp) {
        fprintf(stderr, "Error: Invalid input to process_file\n");
        return;
    }

    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Error opening file");
        return;
    }

    char buffer[1024];
    while (fscanf(f, "%1023s", buffer) == 1) {
        normalize_word(buffer);

        if (strlen(buffer) == 0) {
            continue;
        }

        insert_word_into_index(temp, buffer, file_id);
    }

    fclose(f);
}

void insert_word_into_index(InvertedIndex *temp, const char *word, int file_id) {
    unsigned long h = hash_str(word) % temp->capacity;

    InvertedIndexEntry *e = temp->buckets[h];
    while (e) {
        if (strcmp(e->word, word) == 0) {
            FileIDSet_insert(&e->files, file_id);
            return;
        }
        e = e->next;
    }

    e = malloc(sizeof(InvertedIndexEntry));
    if (!e) {
        perror("Error allocating memory for index entry");
        exit(EXIT_FAILURE);
    }

    e->word = strdup(word);
    if (!e->word) {
        perror("Error allocating memory for word");
        free(e);
        exit(EXIT_FAILURE);
    }

    FileIDSet_init(&e->files);
    FileIDSet_insert(&e->files, file_id);

    e->next = temp->buckets[h];
    temp->buckets[h] = e;
}

int cmp_entries(const void *a, const void *b) {
    const WordEntry *ea = (const WordEntry *)a;
    const WordEntry *eb = (const WordEntry *)b;
    int diff = eb->files.size - ea->files.size;
    if (diff != 0) return diff;
    return strcmp(ea->word, eb->word);
}

int cmp_int(const void *a, const void *b) {
    int x = *(int *)a;
    int y = *(int *)b;
    return x - y;
}


void *mapper_thread(void *arg) {
    InvertedIndex temp_dict;
    InvertedIndex_init(&temp_dict, 1031);

    while (1) {
        pthread_mutex_lock(&file_queue_lock);

        if (next_file_index < num_files) {
            int file_id = next_file_index + 1;
            const char *fname = input_files[next_file_index];
            next_file_index++;
            pthread_mutex_unlock(&file_queue_lock);

            process_file(fname, file_id, &temp_dict);
        } else {
            pthread_mutex_unlock(&file_queue_lock);
            break;
        }
    }

    merge_temp_index_into_global(&temp_dict);

    InvertedIndex_free(&temp_dict);

    signal_mapper_done();

    return NULL;
}

void merge_temp_index_into_global(InvertedIndex *temp_dict) {
    for (int i = 0; i < temp_dict->capacity; i++) {
        InvertedIndexEntry *e = temp_dict->buckets[i];
        while (e) {
            for (int k = 0; k < e->files.size; k++) {
                InvertedIndex_insert(&global_inverted_index, e->word, e->files.data[k]);
            }
            e = e->next;
        }
    }
}

void signal_mapper_done() {
    pthread_mutex_lock(&mapper_done_lock);
    mappers_done_count++;
    if (mappers_done_count == NUM_MAPPERS) {
        pthread_cond_broadcast(&mapper_done_cond);
    }
    pthread_mutex_unlock(&mapper_done_lock);
}

void *reducer_thread(void *arg) {
    wait_for_mappers();

    int reducer_id = (int)(size_t)arg;

    int start_letter, end_letter;
    calculate_letter_range(reducer_id, &start_letter, &end_letter);

    WordEntry *entries = NULL;
    int entries_size = 0, entries_cap = 0;
    collect_entries_for_reducer(start_letter, end_letter, &entries, &entries_size, &entries_cap);

    qsort(entries, entries_size, sizeof(WordEntry), cmp_entries);

    write_results_to_files(start_letter, end_letter, entries, entries_size);

    free_entries(entries, entries_size);

    return NULL;
}

void wait_for_mappers() {
    pthread_mutex_lock(&mapper_done_lock);
    while (mappers_done_count < NUM_MAPPERS) {
        pthread_cond_wait(&mapper_done_cond, &mapper_done_lock);
    }
    pthread_mutex_unlock(&mapper_done_lock);
}

void calculate_letter_range(int reducer_id, int *start_letter, int *end_letter) {
    int letters_per_reducer = 26 / NUM_REDUCERS;
    int extra_letters = 26 % NUM_REDUCERS;

    int start = reducer_id * letters_per_reducer + (reducer_id < extra_letters ? reducer_id : extra_letters);

    int count = letters_per_reducer + (reducer_id < extra_letters ? 1 : 0);

    *start_letter = start;
    *end_letter = start + count;
}


void collect_entries_for_reducer(int start_letter, int end_letter, WordEntry **entries, int *entries_size, int *entries_cap) {
    for (int i = 0; i < global_inverted_index.capacity; i++) {
        InvertedIndexEntry *e = global_inverted_index.buckets[i];
        while (e) {
            char c = e->word[0];
            if (c >= 'a' && c <= 'z') {
                int letter_index = c - 'a';
                if (letter_index >= start_letter && letter_index < end_letter) {
                    if (*entries_size == *entries_cap) {
                        *entries_cap = *entries_cap == 0 ? 32 : (*entries_cap * 2);
                        *entries = realloc(*entries, *entries_cap * sizeof(WordEntry));
                        if (!*entries) {
                            perror("Error reallocating entries");
                            exit(EXIT_FAILURE);
                        }
                    }
                    (*entries)[*entries_size].word = e->word;

                    FileIDSet news;
                    FileIDSet_init(&news);
                    for (int fi = 0; fi < e->files.size; fi++) {
                        FileIDSet_insert(&news, e->files.data[fi]);
                    }
                    (*entries)[*entries_size].files = news;
                    (*entries_size)++;
                }
            }
            e = e->next;
        }
    }
}

void write_results_to_files(int start_letter, int end_letter, WordEntry *entries, int entries_size) {
    for (int L = start_letter; L < end_letter; L++) {
        char fname[10];
        snprintf(fname, sizeof(fname), "%c.txt", 'a' + L);
        FILE *outf = fopen(fname, "w");
        if (!outf) {
            perror("Error opening output file");
            continue;
        }

        for (int ei = 0; ei < entries_size; ei++) {
            if (entries[ei].word[0] - 'a' == L) {
                qsort(entries[ei].files.data, entries[ei].files.size, sizeof(int), cmp_int);
                fprintf(outf, "%s:[", entries[ei].word);
                for (int k = 0; k < entries[ei].files.size; k++) {
                    if (k > 0) fprintf(outf, " ");
                    fprintf(outf, "%d", entries[ei].files.data[k]);
                }
                fprintf(outf, "]\n");
            }
        }
        fclose(outf);
    }
}

void free_entries(WordEntry *entries, int entries_size) {
    for (int ei = 0; ei < entries_size; ei++) {
        FileIDSet_free(&entries[ei].files);
    }
    free(entries);
}
