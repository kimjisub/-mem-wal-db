#include "core.h"

int init_db(MemWalDB *db, const char *wal_filename) {
    hashtable_init(&db->db);

    FILE *wal_file = fopen(wal_filename, "a+");
    if (wal_file == NULL) {
        fprintf(stderr, "Unable to open WAL file: %s\n", wal_filename);
        return -1;
    }
    db->wal_file = wal_file;
    print_wal_info(db);
    return replay_transactions(db, wal_filename);
}

int close_db(MemWalDB *db) {
    fclose(db->wal_file);
    return 0;
}

void print_wal_info(MemWalDB *db) {
    // !평가기준 - 파일 정보
    struct stat st;
    if (fstat(fileno(db->wal_file), &st) != 0) {
        fprintf(stderr, "Unable to get WAL file size\n");
        return;
    }

    printf("WAL file size: %lld bytes\n", st.st_size);
    printf("WAL file last accessed: %s", ctime(&st.st_atime));
    printf("WAL file last modified: %s", ctime(&st.st_mtime));
    printf("WAL file last status change: %s", ctime(&st.st_ctime));

    rewind(db->wal_file);

    char line[MAX_LINE];
    int line_count = 0;
    while (fgets(line, MAX_LINE, db->wal_file) != NULL) {
        line_count++;
    }

    rewind(db->wal_file);

    printf("WAL file line count: %d\n", line_count);
    printf("\n\n");
}

// !평가기준 - 파일 입/출력
int replay_transactions(MemWalDB *db, const char *wal_filename) {
    FILE *wal_read = fopen(wal_filename, "r");
    if (wal_read == NULL) {
        fprintf(stderr, "Unable to open WAL file: %s\n", wal_filename);
        return -1;
    }

    db->replaying = 1;

    char line[MAX_LINE];
    int fd = open("/dev/null", O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open /dev/null\n");
        db->replaying = 0;
        return -1;
    }

    printf("Replaying transactions");

    while (fgets(line, MAX_LINE, wal_read) != NULL) {
        printf(".");
        if (process_command(db, fd, line) < 0) {
            db->replaying = 0;
            close(fd);
            return -1;
        }
    }
    printf("\n");
    close(fd);

    db->replaying = 0;

    fprintf(stderr, "Replayed all transactions.\n\n");
    return 0;
}

// !평가기준 - 파일 입/출력
int log_transaction(MemWalDB *db, const char *transaction) {
    // replay 중에는 로그를 남기지 않습니다.
    if (db->replaying == 1) {
        return 0;
    }

    if (db->wal_file == NULL) {
        fprintf(stderr, "WAL file is NULL, cannot replay transactions.\n");
        return -1;
    }

    printf("Logging transaction: %s\n", transaction);
    fprintf(db->wal_file, "%s\n", transaction);
    fflush(db->wal_file);
    return 0;
}

/**
 * 단순히 명령어를 파싱하고, 해당 명령어에 해당하는 함수를 호출하는 함수입니다.
 */
int process_command(MemWalDB *db, int fd, char *line) {
    char *command = strtok(line, " \n");
    if (command == NULL) {
        write(fd, "INVALID COMMAND", 15);
        return -1;
    }

    // 명령어를 대문자로 변환
    for (char *p = command; *p != '\0'; p++) {
        *p = toupper(*p);
    }

    if (strcmp(command, "SET") == 0) {
        char *key = strtok(NULL, " \n");
        char *value = strtok(NULL, " \n");
        if (key == NULL || value == NULL) {
            write(fd, "INVALID COMMAND", 15);
            return -1;
        }

        if (set_value(db, key, value) < 0) {
            char *error = "FAILED TO SET VALUE";
            write(fd, error, strlen(error) + 1);
            return -1;
        }

        write(fd, "OK", 3);
        return 0;

    } else if (strcmp(command, "GET") == 0) {
        char *key = strtok(NULL, " \n");
        if (key == NULL) {
            write(fd, "INVALID COMMAND", 15);
            return -1;
        }

        char result[MAX_LINE];
        if (get_value(db, key, result, sizeof(result)) < 0) {
            char *error = "NOT FOUND";
            write(fd, error, strlen(error) + 1);
            return -1;
        }

        write(fd, result, strlen(result) + 1);
        return 0;
    } else if (strcmp(command, "DEL") == 0) {
        char *key = strtok(NULL, " \n");
        if (key == NULL) {
            write(fd, "INVALID COMMAND", 15);
            return -1;
        }
        if (del_value(db, key) < 0) {
            char *error = "FAILED TO DELETE VALUE";
            write(fd, error, strlen(error) + 1);
            return -1;
        }
        write(fd, "OK", 3);
        return 0;

    } else if (strcmp(command, "FLUSHALL") == 0) {
        char *error = "NOT IMPLEMENT";
        write(fd, error, strlen(error) + 1);
        return -1;
    } else if (strcmp(command, "EXIST") == 0) {
        char *key = strtok(NULL, " \n");
        if (key == NULL) {
            write(fd, "INVALID COMMAND", 15);
            return -1;
        }
        int index = is_key_exist(db, key);
        char response[MAX_LINE];

        if (index < 0) {
            write(fd, "NOT EXIST", 10);
            return -1;
        }

        snprintf(response, sizeof(response), "EXIST at %d", index);
        write(fd, response, strlen(response));
        return 0;
    } else if (strcmp(command, "HELP") == 0) {
        write(fd, "SET <key> <value> : set value\n", 30);
        write(fd, "GET <key> : get value\n", 22);
        write(fd, "DEL <key> : delete value\n", 25);
        write(fd, "FLUSHALL : delete all values\n", 29);
        write(fd, "EXIST <key> : check if key exists\n", 34);
        write(fd, "EXIT : graceful shutdown\n", 26);
        write(fd, "HELP : show this message\n", 26);
        return 0;
    } else {
        write(fd, "INVALID COMMAND", 15);
        return -1;
    }
}

int get_value(MemWalDB *db, const char *key, char *buffer, size_t buffer_size) {
    int ret = hashtable_get(&database, key, buffer, buffer_size);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

int set_value(MemWalDB *db, const char *key, const char *value) {
    int ret = hashtable_set(&database, key, value);
    if (ret < 0) {
        return -1;
    }
    char transaction[MAX_LINE];
    snprintf(transaction, sizeof(transaction), "SET %s %s", key, value);
    return log_transaction(db, transaction);
}

int del_value(MemWalDB *db, const char *key) {
    int ret = hashtable_del(&database, key);
    if (ret < 0) {
        return -1;
    }
    char transaction[MAX_LINE];
    snprintf(transaction, sizeof(transaction), "DEL %s", key);
    return log_transaction(db, transaction);
}

int is_key_exist(MemWalDB *db, const char *key) {
    int index = hashtable_exist_key(&database, key);

    return index;
}