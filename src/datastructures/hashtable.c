#include <hashtable.h>
#include <alloc.h>
#include <string.h>
#include <console.h>

#define CAPACITY 50000

// Find the hash (index in the hash table) of a key
uint32_t Hash(const char* key) {
    uint32_t i = 0;
    for(int ii = 0; key[ii] != '\0'; ii++) {
        i += key[ii];
    }

    return i % CAPACITY;
}


hash_entry_t* CreateItem(const char* key, char* value){
    hash_entry_t* item = (hash_entry_t*)halloc(sizeof(hash_entry_t));
    item->key = (char*)halloc(strlen(key) + 1);
    item->value = (char*)halloc(strlen(value) + 1);
    strcpy(item->key, key);
    strcpy(item->value, value);

    return item;
}

hash_table_t* CreateTable(size_t size){
    hash_table_t* table = (hash_table_t*)halloc(sizeof(hash_table_t));
    table->size = size;
    table->count = 0;
    table->entries = (hash_entry_t**)halloc(sizeof(hash_entry_t*) * table->size);
    for(size_t i = 0; i < table->size; i++) {
        table->entries[i] = NULL;
    }

    return table;
}

void Insert(hash_table_t* table, const char* key, char* value){
    hash_entry_t* item = CreateItem(key, value);
    uint32_t index = Hash(key);

    hash_entry_t* currentItem = table->entries[index];
    if(currentItem == NULL) {
        if(table->count == table->size) {
            // Resize the table
            hash_table_t* newTable = CreateTable(table->size * 2);
            for(size_t i = 0; i < table->size; i++) {
                hash_entry_t* item = table->entries[i];
                if(item != NULL) {
                    Insert(newTable, item->key, item->value);
                }
            }

            DestroyTable(table);
            table = newTable;
        }

        table->entries[index] = item;
        table->count++;
    }else{
        // Update the table
        if(strcmp(currentItem->key, key) == 0) {
            strcpy(currentItem->value, value);
        }else{
            // Collision
            printf("WARNING: Entry already exists!\n");
            // Handle collision...
            return;
        }
    }
}

void DestroyItem(hash_entry_t* item){
    hfree(item->key);
    hfree(item->value);
    hfree(item);
}

void DestroyTable(hash_table_t* table){
    for(size_t i = 0; i < table->size; i++) {
        hash_entry_t* item = table->entries[i];
        if(item != NULL) {
            DestroyItem(item);
        }
    }

    hfree(table->entries);
    hfree(table);
}
