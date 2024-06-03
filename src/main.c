#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "core.h"

void replay_transactions(const char *wal_filename) {
    // FILE *file = fopen(wal_filename, "r");
    // if (file == NULL) {
    //     perror("Unable to open WAL file");
    //     return;
    // }

    // char line[MAX_LINE];
    // while (fgets(line, sizeof(line), file)) {
    //     char command[10], key[50], value[200];
    //     if (sscanf(line, "%s %s %s", command, key, value) >= 2) {
    //         if (strcmp(command, "SET") == 0) {
    //             set_value(key, value);
    //         } else if (strcmp(command, "DEL") == 0) {
    //             del_value(key);
    //         } else if (strcmp(command, "FLUSHALL") == 0) {
    //             // todo
    //         }
    //     }
    // }

    // fclose(file);
}

// void process_transactions(int read_fd, const char *wal_filename) {
//     char buffer[MAX_LINE];
//     while (read(read_fd, buffer, sizeof(buffer)) > 0) {
//         char command[10], key[50], value[200];
//         if (sscanf(buffer, "%s %s %s", command, key, value) >= 2) {
//             if (strcmp(command, "SET") == 0) {
//                 set_value(key, value);
//                 log_transaction(wal_filename, buffer);
//             } else if (strcmp(command, "GET") == 0) {
//                 get_value(key, value, sizeof(value));
//                 printf("Found: %s -> %s\n", key, value);
//             } else if (strcmp(command, "EXIST") == 0) {
//                 int exists = is_key_exist(key, value, sizeof(value));
//                 if (exists) {
//                     printf("T\n");
//                 } else {
//                     printf("F\n");
//                 }
//             } else if (strcmp(command, "DEL") == 0) {
//                 del_value(key);
//                 log_transaction(wal_filename, buffer);
//             } else if (strcmp(command, "FLUSHALL") == 0) {
//                 // todo
//             }
//         }
//     }
// }

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <db.wal>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *wal_filename = argv[1];
    replay_transactions(wal_filename);

    int fd_front_to_core[2];
    int fd_core_to_front[2];
    pipe(fd_front_to_core);
    pipe(fd_core_to_front);

    pid_t pid = fork();
    if (pid == 0) {
        // 자식 프로세스 (프론트엔드)
        close(fd_front_to_core[0]);
        close(fd_core_to_front[1]);
        while (1) {
            char transaction[MAX_LINE];
            printf("Enter command: ");
            fgets(transaction, sizeof(transaction), stdin);
            write(fd_front_to_core[1], transaction, strlen(transaction) + 1);

            // 코어 프로세스로부터 응답 읽기
            char response[MAX_LINE];
            read(fd_core_to_front[0], response, sizeof(response));
            printf("%s\n", response);
        }
    } else {
        // 부모 프로세스 (코어)
        close(fd_front_to_core[1]);
        close(fd_core_to_front[0]);
        char buffer[MAX_LINE];
        while (read(fd_front_to_core[0], buffer, sizeof(buffer)) > 0) {
            char *command, *key, *value;
            command = strtok(buffer, " ");
            key = strtok(NULL, " ");
            value = strtok(NULL, " ");

            if (command != NULL) {
                if (strcmp(command, "SET") == 0 && key != NULL &&
                    value != NULL) {
                    set_value(key, value);
                } else if (strcmp(command, "DEL") == 0 && key != NULL) {
                    del_value(key);
                } else if (strcmp(command, "FLUSHALL") == 0) {
                    // FLUSHALL 명령어 처리
                } else if (strcmp(command, "GET") == 0 && key != NULL) {
                    char result[MAX_LINE];
                    get_value(key, result, sizeof(result));
                    write(fd_core_to_front[1], result, strlen(result) + 1);
                } else if (strcmp(command, "EXIST") == 0 && key != NULL) {
                    int exists = is_key_exist(key);
                    char response[2];
                    snprintf(response, sizeof(response), "%d", exists);
                    write(fd_core_to_front[1], response, strlen(response) + 1);
                }
                // 완료되면 OK 응답 보내기
                write(fd_core_to_front[1], "OK", 3);
            }
        }
        wait(NULL);
    }

    return 0;
}