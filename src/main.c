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
            process_command(&mwl, sockfd[0], buffer);
        }
        wait(NULL);
    }

    return 0;
}