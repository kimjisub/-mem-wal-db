#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdio.h>
#include <string.h>

#define MAX_KEY_LENGTH 50
#define MAX_VALUE_LENGTH 200
#define TABLE_SIZE 3

typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int is_occupied;
} HashEntry;

typedef struct {
    HashEntry entries[TABLE_SIZE];
} HashTable;

void hashtable_init(HashTable *table);
unsigned int hash(const char *key);
int hashtable_find_index(HashTable *table, const char *key);
int hashtable_get_next_index(HashTable *table, const char *key);
int hashtable_set(HashTable *table, const char *key, const char *value);
int hashtable_get(HashTable *table, const char *key, char *value,
                  size_t buffer_size);
int hashtable_del(HashTable *table, const char *key);
int hashtable_exist_key(HashTable *table, const char *key);

#endif