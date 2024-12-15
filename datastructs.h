#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

#include <pthread.h>

// A flexible container for storing a set of file IDs without duplicates.
typedef struct {
    int *data;       
    int size;        
    int capacity;    
} FileIDSet;

// Initializes an empty FileIDSet, preparing it for use.
void FileIDSet_init(FileIDSet *s);

// Frees the memory used by a FileIDSet, cleaning it up.
void FileIDSet_free(FileIDSet *s);

// Adds a file ID to the set, ensuring no duplicates.
void FileIDSet_insert(FileIDSet *s, int value);

// Represents a single word in the index and the files it appears in.
typedef struct InvertedIndexEntry {
    char *word;                  
    FileIDSet files;             
    struct InvertedIndexEntry *next;
} InvertedIndexEntry;

// A hash table-based index for storing words and the files they appear in.
typedef struct {
    InvertedIndexEntry **buckets;
    int capacity;                
    pthread_mutex_t lock;         
} InvertedIndex;

// Prepares an empty inverted index with a given number of buckets.
void InvertedIndex_init(InvertedIndex *d, int capacity);

// Frees all memory used by an inverted index.
void InvertedIndex_free(InvertedIndex *d);

// Adds a word and its associated file ID to the inverted index.
void InvertedIndex_insert(InvertedIndex *d, const char *word, int fileID);

// Computes a hash value for a string using the FNV-1a hash algorithm.
unsigned long hash_str(const char *str);

// Searches for a specific word in a linked list of entries.
InvertedIndexEntry *find_entry(InvertedIndexEntry *e, const char *word);

// Creates a new index entry for a word and associates it with a file ID.
InvertedIndexEntry *create_entry(const char *word, int fileID);

#endif
