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

void frontend(int sockfd[2]) {
    while (1) {
        char transaction[MAX_LINE];
        printf("> ");
        fgets(transaction, sizeof(transaction), stdin);
        write(sockfd[1], transaction, strlen(transaction) + 1);

        // 코어 프로세스로부터 응답 읽기
        char response[MAX_LINE];
        read(sockfd[1], response, sizeof(response));
        printf("%s\n", response);

        if (strcmp(response, "GOODBYE") == 0) {
            break;
        }
    }
}

void core(int sockfd[2]) {
    char buffer[MAX_LINE];
    while (read(sockfd[0], buffer, sizeof(buffer)) > 0) {
        process_command(&mwl, sockfd[0], buffer);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <db.wal>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *wal_filename = argv[1];
    init_db(&mwl, wal_filename);

    // !평가기준 5 - IPC/시스템V 통신 (Pipe, Socket)
    // 수업시간에 socketpair에 대해서 다루진 않았지만,
    // socketpair는 두 프로세스 간 통신을 위한 양방향 파이프를 생성하는
    // 함수입니다.
    int sockfd[2];
    socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);

    // !평가기준 3 - 프로세스 생성
    pid_t pid;

    switch (pid = fork()) {
    case -1: /* fork failed */
        perror("fork");
        exit(1);
        break;
    case 0:               /* child process */
        close(sockfd[0]); // frontend 프로세스에서는 0번 파이스 사용 안함.
        frontend(sockfd);
        printf("Fronted END\n");
        break;
    default: /* parent process */
        printf("PID\n");
        printf("Parent:           %d\n", (int)getppid());
        printf("Core (Parent):    %d\n", (int)getpid());
        printf("Frontend (Child): %d\n", (int)pid);
        printf("\n");

        close(sockfd[1]); // core 프로세스에서는 1번 파이스 사용 안함.
        core(sockfd);
        printf("Core END\n");

        // !평가기준 4 - exec 함수군
        // 코어 종료 이후 wal 파일 정보 출력
        if (execlp("stat", "stat", wal_filename, (char *)NULL) == -1) {
            perror("execve");
            exit(1);
        }

        break;
    }

    return 0;
}
