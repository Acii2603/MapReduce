#include "mapreduce.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <pthread.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <num_mappers> <num_reducers> <input_list>\n", argv[0]);
        return 1;
    }

    NUM_MAPPERS = atoi(argv[1]);
    NUM_REDUCERS = atoi(argv[2]);
    const char *input_list = argv[3];

    FILE *f = fopen(input_list, "r");
    if (!f) {
        perror("Error opening input file");
        return 1;
    }

    if (fscanf(f, "%d\n", &num_files) != 1) {
        fprintf(stderr, "Error reading number of files\n");
        fclose(f);
        return 1;
    }

    input_files = malloc(num_files * sizeof(char *));
    if (!input_files) {
        perror("Error allocating memory for input files");
        fclose(f);
        return 1;
    }

    for (int i = 0; i < num_files; i++) {
        char buf[1024];
        if (!fgets(buf, sizeof(buf), f)) {
            fprintf(stderr, "Error reading file name\n");
            fclose(f);
            return 1;
        }
        char *nl = strchr(buf, '\n');
        if (nl) *nl = '\0';
        input_files[i] = strdup(buf);
        if (!input_files[i]) {
            perror("Error allocating memory for file name");
            fclose(f);
            return 1;
        }
    }
    fclose(f);

    InvertedIndex_init(&global_inverted_index, 10007);

    pthread_t *mappers = malloc(NUM_MAPPERS * sizeof(pthread_t));
    pthread_t *reducers = malloc(NUM_REDUCERS * sizeof(pthread_t));
    if (!mappers || !reducers) {
        perror("Error allocating memory for threads");
        return 1;
    }

    for (int i = 0; i < NUM_MAPPERS; i++) {
        int ret = pthread_create(&mappers[i], NULL, mapper_thread, NULL);
        if (ret != 0) {
            fprintf(stderr, "Error creating mapper thread %d: %s\n", i, strerror(ret));
            return 1;
        }
    }

    for (int i = 0; i < NUM_REDUCERS; i++) {
        int ret = pthread_create(&reducers[i], NULL, reducer_thread, (void *)(size_t)i);
        if (ret != 0) {
            fprintf(stderr, "Error creating reducer thread %d: %s\n", i, strerror(ret));
            return 1;
        }
    }

    for (int i = 0; i < NUM_MAPPERS; i++) {
        int ret = pthread_join(mappers[i], NULL);
        if (ret != 0) {
            fprintf(stderr, "Error joining mapper thread %d: %s\n", i, strerror(ret));
            return 1;
        }
    }

    for (int i = 0; i < NUM_REDUCERS; i++) {
        int ret = pthread_join(reducers[i], NULL);
        if (ret != 0) {
            fprintf(stderr, "Error joining reducer thread %d: %s\n", i, strerror(ret));
            return 1;
        }
    }

    InvertedIndex_free(&global_inverted_index);
    for (int i = 0; i < num_files; i++) {
        free(input_files[i]);
    }
    free(input_files);
    free(mappers);
    free(reducers);

    return 0;
}
