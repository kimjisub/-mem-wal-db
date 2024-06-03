#include "core.h"

void log_transaction(const char *wal_filename, const char *transaction) {
    FILE *file = fopen(wal_filename, "a+");
    if (file == NULL) {
        fprintf(stderr, "Unable to open WAL file: %s\n", strerror(errno));
        return;
    }

    fprintf(file, "%s\n", transaction);
    fclose(file);
}

void get_value(const char *key, char *buffer, size_t buffer_size) {
    hashtable_get(&database, key, buffer);
}

void set_value(const char *key, const char *value) {
    hashtable_set(&database, key, value);
    char transaction[MAX_LINE];
    snprintf(transaction, sizeof(transaction), "SET %s %s", key, value);
    log_transaction("wal.log", transaction);
}

void del_value(const char *key) {
    hashtable_del(&database, key);
    char transaction[MAX_LINE];
    snprintf(transaction, sizeof(transaction), "DEL %s", key);
    log_transaction("wal.log", transaction);
}

int is_key_exist(const char *key) {
    int exists = hashtable_exist_key(&database, key);
    return exists;
}