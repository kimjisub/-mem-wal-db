#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"

#define MAX_LINE 256
#define DB_SIZE 1000

HashTable database;

int log_transaction(const char *wal_filename, const char *transaction);
int get_value(const char *key, char *buffer, size_t buffer_size);
int set_value(const char *key, const char *value);
int del_value(const char *key);
int is_key_exist(const char *key);