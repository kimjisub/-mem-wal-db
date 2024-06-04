#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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

    int sockfd[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd);

    pid_t pid = fork();
    if (pid == 0) {
        // 자식 프로세스 (프론트엔드)
        close(sockfd[0]);
        while (1) {
            char transaction[MAX_LINE];
            printf("Enter command: ");
            fgets(transaction, sizeof(transaction), stdin);
            write(sockfd[1], transaction, strlen(transaction) + 1);

            // 코어 프로세스로부터 응답 읽기
            char response[MAX_LINE];
            read(sockfd[1], response, sizeof(response));
            printf("%s\n", response);
        }
    } else {
        // 부모 프로세스 (코어)
        close(sockfd[1]);
        char buffer[MAX_LINE];
        while (read(sockfd[0], buffer, sizeof(buffer)) > 0) {
            char *command, *key, *value;
            command = strtok(buffer, " \n");
            for (char *p = command; *p; ++p)
                *p = toupper(*p);

            if (command != NULL) {
                if (strcmp(command, "SET") == 0) {
                    key = strtok(NULL, " \n");
                    value = strtok(NULL, " \n");
                    if (key != NULL && value != NULL) {
                        set_value(key, value);
                        write(sockfd[0], "OK", 3);
                    } else {
                        write(sockfd[0], "INVALID COMMAND", 15);
                    }
                } else if (strcmp(command, "DEL") == 0) {
                    key = strtok(NULL, " \n");
                    if (key != NULL) {
                        del_value(key);
                        write(sockfd[0], "OK", 3);
                    } else {
                        write(sockfd[0], "INVALID COMMAND", 15);
                    }
                } else if (strcmp(command, "FLUSHALL") == 0) {
                    write(sockfd[0], "NOT IMPLEMENT", 3);
                } else if (strcmp(command, "GET") == 0) {
                    key = strtok(NULL, " \n");
                    if (key != NULL) {
                        char result[MAX_LINE];
                        get_value(key, result, sizeof(result));
                        write(sockfd[0], result, strlen(result) + 1);
                    } else {
                        write(sockfd[0], "INVALID COMMAND", 15);
                    }
                } else if (strcmp(command, "EXIST") == 0) {
                    key = strtok(NULL, " \n");
                    if (key != NULL) {
                        int exists = is_key_exist(key);
                        char response[2];
                        snprintf(response, sizeof(response), "%d", exists);
                        write(sockfd[0], response, strlen(response) + 1);
                    } else {
                        write(sockfd[0], "INVALID COMMAND", 15);
                    }
                } else if (strcmp(command, "HELP") == 0) {
                    write(sockfd[0], "SET <key> <value> : set value\n", 30);
                    write(sockfd[0], "GET <key> : get value\n", 22);
                    write(sockfd[0], "DEL <key> : delete value\n", 25);
                    write(sockfd[0], "FLUSHALL : delete all values\n", 29);
                    write(sockfd[0], "EXIST <key> : check if key exists\n", 34);
                    write(sockfd[0], "HELP : show this message\n", 26);
                } else {
                    write(sockfd[0], "INVALID COMMAND", 15);
                }
            }
        }
        wait(NULL);
    }

    return 0;
}