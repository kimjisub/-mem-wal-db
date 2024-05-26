#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 256
#define DB_SIZE 1000

typedef struct {
    char key[50];
    char value[200];
} Record;

Record database[DB_SIZE];
int record_count = 0;

void replay_transactions(const char *wal_filename) {
    FILE *file = fopen(wal_filename, "r");
    if (file == NULL) {
        perror("Unable to open WAL file");
        return;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        char command[10], key[50], value[200];
        if (sscanf(line, "%s %s %s", command, key, value) >= 2) {
            if (strcmp(command, "SET") == 0) {
                int found = 0;
                for (int i = 0; i < record_count; ++i) {
                    if (strcmp(database[i].key, key) == 0) {
                        strcpy(database[i].value, value);
                        found = 1;
                        break;
                    }
                }
                if (!found && record_count < DB_SIZE) {
                    strcpy(database[record_count].key, key);
                    strcpy(database[record_count].value, value);
                    record_count++;
                }
            } else if (strcmp(command, "DEL") == 0) {
                for (int i = 0; i < record_count; ++i) {
                    if (strcmp(database[i].key, key) == 0) {
                        for (int j = i; j < record_count - 1; ++j) {
                            database[j] = database[j + 1];
                        }
                        record_count--;
                        break;
                    }
                }
            } else if (strcmp(command, "FLUSHALL") == 0) {
                record_count = 0;
            }
        }
    }

    fclose(file);
}

void log_transaction(const char *wal_filename, const char *transaction) {
    FILE *file = fopen(wal_filename, "a");
    if (file == NULL) {
        perror("Unable to open WAL file");
        return;
    }

    fprintf(file, "%s\n", transaction);
    fclose(file);
}

void process_transactions(int read_fd, const char *wal_filename) {
    char buffer[MAX_LINE];
    while (read(read_fd, buffer, sizeof(buffer)) > 0) {
        char command[10], key[50], value[200];
        if (sscanf(buffer, "%s %s %s", command, key, value) >= 2) {
            if (strcmp(command, "SET") == 0) {
                int found = 0;
                for (int i = 0; i < record_count; ++i) {
                    if (strcmp(database[i].key, key) == 0) {
                        strcpy(database[i].value, value);
                        found = 1;
                        break;
                    }
                }
                if (!found && record_count < DB_SIZE) {
                    strcpy(database[record_count].key, key);
                    strcpy(database[record_count].value, value);
                    record_count++;
                }
                log_transaction(wal_filename, buffer);
            } else if (strcmp(command, "DEL") == 0) {
                for (int i = 0; i < record_count; ++i) {
                    if (strcmp(database[i].key, key) == 0) {
                        for (int j = i; j < record_count - 1; ++j) {
                            database[j] = database[j + 1];
                        }
                        record_count--;
                        break;
                    }
                }
                log_transaction(wal_filename, buffer);
            } else if (strcmp(command, "FLUSHALL") == 0) {
                record_count = 0;
                log_transaction(wal_filename, buffer);
            }
        }
    }
}

void handle_read_request(const char *buffer) {
    char key[50];
    sscanf(buffer, "GET %s", key);
    int found = 0;
    for (int i = 0; i < record_count; ++i) {
        if (strcmp(database[i].key, key) == 0) {
            printf("Found: %s -> %s\n", database[i].key, database[i].value);
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("Record not found for key: %s\n", key);
    }
}

void handle_exist_request(const char *buffer) {
    char key[50];
    sscanf(buffer, "EXIST %s", key);
    int found = 0;
    for (int i = 0; i < record_count; ++i) {
        if (strcmp(database[i].key, key) == 0) {
            printf("T\n");
            found = 1;
            break;
        }
    }
    if (!found) {
        printf("F\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <db.wal>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *wal_filename = argv[1];
    replay_transactions(wal_filename);

    int fd[2];
    pipe(fd);

    pid_t pid = fork();
    if (pid == 0) {
        // 자식 프로세스 (프론트엔드)
        close(fd[0]);
        while (1) {
            char transaction[MAX_LINE];
            printf("Enter command: ");
            fgets(transaction, sizeof(transaction), stdin);
            write(fd[1], transaction, strlen(transaction) + 1);
        }
    } else {
        // 부모 프로세스 (코어)
        close(fd[1]);
        char buffer[MAX_LINE];
        while (read(fd[0], buffer, sizeof(buffer)) > 0) {
            if (strncmp(buffer, "SET", 3) == 0 ||
                strncmp(buffer, "DEL", 3) == 0 ||
                strncmp(buffer, "FLUSHALL", 8) == 0) {
                process_transactions(fd[0], wal_filename);
            } else if (strncmp(buffer, "GET", 3) == 0) {
                handle_read_request(buffer);
            } else if (strncmp(buffer, "EXIST", 5) == 0) {
                handle_exist_request(buffer);
            }
        }
        wait(NULL);
    }

    return 0;
}