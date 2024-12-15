#ifndef MAPREDUCE_H
#define MAPREDUCE_H

#include "datastructs.h"
#include <pthread.h>

// A structure to hold a word and the set of file IDs where it appears.
typedef struct {
    char *word;      
    FileIDSet files;
} WordEntry;

// Number of mapper and reducer threads, and the input data to process.
// These are shared between mappers and reducers for coordination.
extern int NUM_MAPPERS;          
extern int NUM_REDUCERS;         
extern char **input_files;       
extern int num_files;            
extern InvertedIndex global_inverted_index;

// Synchronization mechanisms to ensure safe, threaded execution.
// These variables manage thread access to the file queue and mapper completion.
extern pthread_mutex_t file_queue_lock; 
extern int next_file_index;             

extern pthread_mutex_t mapper_done_lock;
extern pthread_cond_t mapper_done_cond; 
extern int mappers_done_count;          

// Function run by each mapper thread to process a portion of the input files.
void *mapper_thread(void *arg);

// Cleans and standardizes a word by converting it to lowercase and removing non-alphabetic characters.
void normalize_word(char *w);

// Processes a single file, breaking it into words and adding them to a temporary inverted index.
void process_file(const char *filename, int file_id, InvertedIndex *temp);

// Adds a word and its associated file ID to a mapper's temporary inverted index.
void insert_word_into_index(InvertedIndex *temp, const char *word, int file_id);

// Combines a mapper's temporary inverted index into the shared global inverted index.
void merge_temp_index_into_global(InvertedIndex *temp_dict);

// Signals that a mapper thread has finished its work, notifying reducers if needed.
void signal_mapper_done(void);

// Function run by each reducer thread to process assigned portions of the global index.
void *reducer_thread(void *arg);

// Waits until all mapper threads have completed their work before reducers start processing.
void wait_for_mappers(void);

// Calculates which letters (a-z) a specific reducer thread is responsible for.
void calculate_letter_range(int reducer_id, int *start_letter, int *end_letter);

// Collects words from the global inverted index that fall within a reducer's assigned letter range.
void collect_entries_for_reducer(int start_letter, int end_letter, WordEntry **entries, int *entries_size, int *entries_cap);

// Writes the sorted results of a reducer's work to files, one file per letter.
void write_results_to_files(int start_letter, int end_letter, WordEntry *entries, int entries_size);

// Frees the memory used by a set of WordEntry structures.
void free_entries(WordEntry *entries, int entries_size);

// Comparison function to sort WordEntry objects by the size of their file ID set, and alphabetically as a tiebreaker.
int cmp_entries(const void *a, const void *b);

// Comparison function to sort integers in ascending order.
int cmp_int(const void *a, const void *b);

#endif
