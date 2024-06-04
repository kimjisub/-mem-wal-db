#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "hash_table.h"

#define MAX_LINE 256
#define DB_SIZE 1000

HashTable database;

typedef struct {
    HashTable db;
    FILE *wal_file;
    int replaying;
} MemWalDB;

int init_db(MemWalDB *db, const char *wal_filename);
int close_db(MemWalDB *db);
int replay_transactions(MemWalDB *db, const char *wal_filename);
int log_transaction(MemWalDB *db, const char *transaction);
int process_command(MemWalDB *db, int fd, char *command);
int get_value(MemWalDB *db, const char *key, char *buffer, size_t buffer_size);
int set_value(MemWalDB *db, const char *key, const char *value);
int del_value(MemWalDB *db, const char *key);
int is_key_exist(MemWalDB *db, const char *key);