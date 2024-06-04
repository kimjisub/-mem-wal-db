#include "core.h"

int log_transaction(const char *wal_filename, const char *transaction) {
    FILE *file = fopen(wal_filename, "a+");
    if (file == NULL) {
        return -1;
    }

    fprintf(file, "%s\n", transaction);
    fclose(file);
    return 0;
}

int get_value(const char *key, char *buffer, size_t buffer_size) {
    int ret = hashtable_get(&database, key, buffer);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int set_value(const char *key, const char *value) {
    int ret = hashtable_set(&database, key, value);
    if (ret < 0) {
        return -1;
    }
    char transaction[MAX_LINE];
    snprintf(transaction, sizeof(transaction), "SET %s %s", key, value);
    return log_transaction("wal.log", transaction);
}

int del_value(const char *key) {
    int ret = hashtable_del(&database, key);
    if (ret < 0) {
        return -1;
    }
    char transaction[MAX_LINE];
    snprintf(transaction, sizeof(transaction), "DEL %s", key);
    return log_transaction("wal.log", transaction);
}

int is_key_exist(const char *key) {
    int index = hashtable_exist_key(&database, key);

    return index;
}