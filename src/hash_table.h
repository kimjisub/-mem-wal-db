#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdio.h>
#include <string.h>

#define MAX_KEY_LENGTH 50
#define MAX_VALUE_LENGTH 200
#define TABLE_SIZE 1000

typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    int is_occupied;
} HashEntry;

typedef struct {
    HashEntry entries[TABLE_SIZE];
} HashTable;

void init_table(HashTable *table);
unsigned int hash(const char *key);
int set_value(HashTable *table, const char *key, const char *value);
int get_value(HashTable *table, const char *key, char *value);
int del_value(HashTable *table, const char *key);
int exist_key(HashTable *table, const char *key);

#endif