#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>
#include <stddef.h>

typedef struct Hash_Entry {
    char* key;
    void* value;
} hash_entry_t;

typedef struct Hash_Table {
    hash_entry_t** entries;
    size_t size;
    uint32_t count;
} hash_table_t;

uint32_t Hash(const char* key);
hash_entry_t* CreateItem(const char* key, char* value);
hash_table_t* CreateTable(size_t size);
void Insert(hash_table_t* table, const char* key, char* value);
void DestroyItem(hash_entry_t* item);
void DestroyTable(hash_table_t* table);

#endif // HASHTABLE_H