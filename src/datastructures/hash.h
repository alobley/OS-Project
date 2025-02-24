#ifndef HASH_H
#define HASH_H

#include <stddef.h>

// Allows building and using the hash table for the integrated shell

typedef struct Hash_Entry {
    char* command;                      // The command to be executed
    void (*func)(char*);                // The function to be executed
    struct Hash_Entry* next;            // If there is a collision, the next entry is stored here
} hash_entry_t;

typedef struct {
    hash_entry_t* table;              // The hash table
    size_t size;                      // The size of the hash table
} hash_table_t;

int HashInsert(hash_table_t* table, char* command, void (*func)(char*));
hash_entry_t* hash(hash_table_t* table, char* command);
hash_table_t* CreateTable(size_t size);

#endif