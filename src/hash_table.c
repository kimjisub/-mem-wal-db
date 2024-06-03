#include "hash_table.h"

void hashtable_init(HashTable *table) {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        table->entries[i].is_occupied = 0;
    }
}

unsigned int hash(const char *key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash << 5) + *key++;
    }
    return hash % TABLE_SIZE;
}

int hashtable_set(HashTable *table, const char *key, const char *value) {
    unsigned int index = hash(key);
    while (table->entries[index].is_occupied &&
           strcmp(table->entries[index].key, key) != 0) {
        index = (index + 1) % TABLE_SIZE;
    }
    strcpy(table->entries[index].key, key);
    strcpy(table->entries[index].value, value);
    table->entries[index].is_occupied = 1;
    return 1;
}

int hashtable_get(HashTable *table, const char *key, char *value) {
    unsigned int index = hash(key);
    while (table->entries[index].is_occupied) {
        if (strcmp(table->entries[index].key, key) == 0) {
            strcpy(value, table->entries[index].value);
            return 1;
        }
        index = (index + 1) % TABLE_SIZE;
    }
    return 0;
}

int hashtable_del(HashTable *table, const char *key) {
    unsigned int index = hash(key);
    while (table->entries[index].is_occupied) {
        if (strcmp(table->entries[index].key, key) == 0) {
            table->entries[index].is_occupied = 0;
            return 1;
        }
        index = (index + 1) % TABLE_SIZE;
    }
    return 0;
}

int hashtable_exist_key(HashTable *table, const char *key) {
    unsigned int index = hash(key);
    while (table->entries[index].is_occupied) {
        if (strcmp(table->entries[index].key, key) == 0) {
            return 1;
        }
        index = (index + 1) % TABLE_SIZE;
    }
    return 0;
}