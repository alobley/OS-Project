#include <hash.h>
#include <alloc.h>
#include <util.h>
#include <string.h>

// Returns an array of hash_entry_t
MALLOC hash_table_t* CreateTable(size_t size){
    hash_table_t* hash_table = (hash_table_t*)halloc(sizeof(hash_table_t));
    hash_table->table = (hash_entry_t*)halloc(sizeof(hash_entry_t) * size);
    hash_table->size = size;
    memset(hash_table->table, 0, sizeof(hash_entry_t) * size);
    return hash_table;
}

hash_entry_t* hash(hash_table_t* table, char* command){
    size_t hash = 0;
    for(size_t i = 0; i < strlen(command); i++){
        hash += command[i];
    }
    hash %= table->size;
    // Compare the table entry to check for collision
    if(table->table[hash].command != NULL){
        hash_entry_t* entry = &table->table[hash];
        if(strcmp(entry->command, command) == 0){
            return entry;
        }
        else{
            while(entry->next != NULL){
                entry = entry->next;
                if(strcmp(entry->command, command) == 0){
                    return entry;
                }
            }
        }
    }

    return NULL;
}

int HashInsert(hash_table_t* table, char* command, void (*func)(char*)){
    hash_entry_t* entry = hash(table, command);
    if(entry == NULL){
        size_t hash = 0;
        for(size_t i = 0; i < strlen(command); i++){
            hash += command[i];
        }
        hash %= table->size;
        entry = &table->table[hash];
    }
    if(entry->command == NULL){
        entry->command = command;
        entry->func = func;
        return 0;
    }
    else{
        while(entry->next != NULL){
            entry = entry->next;
        }
        entry->next = (hash_entry_t*)halloc(sizeof(hash_entry_t));
        memset(entry->next, 0, sizeof(hash_entry_t));
        entry->next->command = command;
        entry->next->func = func;
        return 0;
    }
}

void ClearTable(hash_table_t* table){
    for(size_t i = 0; i < table->size; i++){
        hash_entry_t* entry = &table->table[i];
        if(entry != NULL && entry->next != NULL){
            entry = entry->next;
            while(entry != NULL){
                hash_entry_t* next = entry->next;
                hfree(entry);
                entry = next;
            }
        }
    }
    hfree(table->table);
    hfree(table);
}