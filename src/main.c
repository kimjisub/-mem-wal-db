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

MemWalDB mwl;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <db.wal>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *wal_filename = argv[1];
    init_db(&mwl, wal_filename);

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
                    if (key == NULL || value == NULL) {
                        write(sockfd[0], "INVALID COMMAND", 15);
                        continue;
                    }

                    if (set_value(&mwl, key, value) < 0) {
                        char *error = "FAILED TO SET VALUE";
                        write(sockfd[0], error, strlen(error) + 1);
                        continue;
                    }

                    write(sockfd[0], "OK", 3);

                } else if (strcmp(command, "GET") == 0) {
                    key = strtok(NULL, " \n");
                    if (key == NULL) {
                        write(sockfd[0], "INVALID COMMAND", 15);
                        continue;
                    }

                    char result[MAX_LINE];
                    if (get_value(&mwl, key, result, sizeof(result)) < 0) {
                        char *error = "NOT FOUND";
                        write(sockfd[0], error, strlen(error) + 1);
                        continue;
                    }

                    write(sockfd[0], result, strlen(result) + 1);
                } else if (strcmp(command, "DEL") == 0) {
                    key = strtok(NULL, " \n");
                    if (key == NULL) {
                        write(sockfd[0], "INVALID COMMAND", 15);
                        continue;
                    }
                    if (del_value(&mwl, key) < 0) {
                        char *error = "FAILED TO DELETE VALUE";
                        write(sockfd[0], error, strlen(error) + 1);
                        continue;
                    }
                    write(sockfd[0], "OK", 3);

                } else if (strcmp(command, "FLUSHALL") == 0) {
                    char *error = "NOT IMPLEMENT";
                    write(sockfd[0], error, strlen(error) + 1);
                    continue;
                } else if (strcmp(command, "GET") == 0) {
                    key = strtok(NULL, " \n");
                    if (key == NULL) {
                        write(sockfd[0], "INVALID COMMAND", 15);
                        continue;
                    }
                    char result[MAX_LINE];
                    if (get_value(&mwl, key, result, sizeof(result)) < 0) {
                        char *error = "NOT FOUND";
                        write(sockfd[0], error, strlen(error) + 1);
                        continue;
                    }
                    write(sockfd[0], result, strlen(result) + 1);

                } else if (strcmp(command, "EXIST") == 0) {
                    key = strtok(NULL, " \n");
                    if (key == NULL) {
                        write(sockfd[0], "INVALID COMMAND", 15);
                        continue;
                    }
                    int index = is_key_exist(&mwl, key);
                    char response[MAX_LINE];

                    if (index < 0) {
                        write(sockfd[0], "NOT EXIST", 6);
                        continue;
                    }

                    snprintf(response, sizeof(response), "EXIST at (%d)",
                             index);
                    write(sockfd[0], response, strlen(response) + 1);
                    continue;
                } else if (strcmp(command, "EXIT") == 0) {
                    close_db(&mwl);
                    exit(0);
                    break;
                } else if (strcmp(command, "HELP") == 0) {
                    write(sockfd[0], "SET <key> <value> : set value\n", 30);
                    write(sockfd[0], "GET <key> : get value\n", 22);
                    write(sockfd[0], "DEL <key> : delete value\n", 25);
                    write(sockfd[0], "FLUSHALL : delete all values\n", 29);
                    write(sockfd[0], "EXIST <key> : check if key exists\n", 34);
                    write(sockfd[0], "EXIT : graceful shutdown\n", 34);
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