#include "datastructs.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void FileIDSet_init(FileIDSet *s) {
    s->data = NULL;
    s->size = 0;
    s->capacity = 0;
}

void FileIDSet_free(FileIDSet *s) {
    free(s->data);
    s->data = NULL;
    s->size = 0;
    s->capacity = 0;
}

void FileIDSet_insert(FileIDSet *s, int value) {
    for (int i = 0; i < s->size; i++) {
        if (s->data[i] == value) {
            return;
        }
    }
    if (s->size == s->capacity) {
        if (s->capacity == 0) {
            s->capacity = 4;
        } else {
            s->capacity *= 2;
        }
        s->data = realloc(s->data, s->capacity * sizeof(int));
    }
    s->data[s->size++] = value;
}

unsigned long hash_str(const char *str) {
    unsigned long h = 2166136261UL;
    while (*str) {
        h ^= (unsigned char)*str;
        h *= 16777619;             
        str++;
    }
    return h;
}

void InvertedIndex_init(InvertedIndex *d, int capacity) {
    d->capacity = capacity;
    d->buckets = calloc(capacity, sizeof(InvertedIndexEntry *));
    pthread_mutex_init(&d->lock, NULL);
}

void InvertedIndex_free(InvertedIndex *d) {
    for (int i = 0; i < d->capacity; i++) {
        InvertedIndexEntry *e = d->buckets[i];
        while (e) {
            InvertedIndexEntry *next = e->next;
            free(e->word);
            FileIDSet_free(&e->files);
            free(e);
            e = next;
        }
    }
    free(d->buckets);
    pthread_mutex_destroy(&d->lock);
}

void InvertedIndex_insert(InvertedIndex *d, const char *word, int fileID) {
    unsigned long h = hash_str(word) % d->capacity;

    pthread_mutex_lock(&d->lock);

    InvertedIndexEntry *e = d->buckets[h];
    InvertedIndexEntry *existingEntry = find_entry(e, word);
    if (existingEntry) {
        FileIDSet_insert(&existingEntry->files, fileID);
        pthread_mutex_unlock(&d->lock);
        return;
    }

    InvertedIndexEntry *newEntry = create_entry(word, fileID);
    newEntry->next = d->buckets[h];
    d->buckets[h] = newEntry;

    pthread_mutex_unlock(&d->lock);
}

InvertedIndexEntry *find_entry(InvertedIndexEntry *e, const char *word) {
    while (e) {
        if (strcmp(e->word, word) == 0) {
            return e;
        }
        e = e->next;
    }
    return NULL;
}

InvertedIndexEntry *create_entry(const char *word, int fileID) {
    InvertedIndexEntry *newEntry = malloc(sizeof(InvertedIndexEntry));
    if (!newEntry) {
        perror("Error: Failed to allocate memory for new index entry");
        exit(EXIT_FAILURE);
    }

    newEntry->word = strdup(word);
    if (!newEntry->word) {
        perror("Error: Failed to allocate memory for word");
        free(newEntry);
        exit(EXIT_FAILURE);
    }

    FileIDSet_init(&newEntry->files);
    FileIDSet_insert(&newEntry->files, fileID);
    newEntry->next = NULL;

    return newEntry;
}
