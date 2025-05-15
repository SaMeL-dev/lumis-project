// LUMIS server.c
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#define PORT 12345
#define BUF_SIZE 1024

void HandleClient(void* arg);

int main() {
    WSADATA wsaData;
    SOCKET serv_sock, clientSock;
    SOCKADDR_IN serv_adr, clnt_adr;
    int clnt_adr_sz;

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(PORT);

    bind(serv_sock, (SOCKADDR*)&serv_adr, sizeof(serv_adr));
    listen(serv_sock, 5);
    printf("서버 시작: 포트 %d에서 대기 중...\n", PORT);

    while (1) {
        clnt_adr_sz = sizeof(clnt_adr);
        clientSock = accept(serv_sock, (SOCKADDR*)&clnt_adr, &clnt_adr_sz);
        printf("클라이언트 접속됨\n");

        _beginthreadex(NULL, 0, HandleClient, (void*)clientSock, 0, NULL);
    }

    closesocket(serv_sock);
    WSACleanup();
    return 0;
}

// 클라이언트 요청 처리 스레드 함수
void HandleClient(void* arg) {
    SOCKET clientSock = (SOCKET)arg;
    char buf[BUF_SIZE];
    int str_len;

    while ((str_len = recv(clientSock, buf, BUF_SIZE -1, 0)) != 0) {
        buf[str_len] = '\0';
        printf("[수신] %s\n", buf);

        if (strcmp(buf, "quit") == 0) break;

        send(clientSock, "서버로부터 응답\n", 18, 0);
    }

    // 로그인 처리
    recv(clientSock, buf, BUF_SIZE - 1, 0);
    buf[strcspn(buf, "\n")] = 0;  // 개행 문자 제거

    // ID와 PW 파싱
    char* input_id = strtok(buf, "//");
    char* input_pw = strtok(NULL, "//");

    // 파일 열고 인증
    FILE* fp = fopen("users.txt", "r");
    char line[BUF_SIZE];
    int login_success = 0;

    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\n")] = 0;  // 개행 제거
            char* file_id = strtok(line, "//");
            char* file_pw = strtok(NULL, "//");
            
            if (file_id && file_pw && strcmp(file_id, input_id) == 0 && strcmp(file_pw, input_pw) == 0) {
                login_success = 1;
                break;
            }
        }
        fclose(fp);
    }

    if (login_success) {
        send(clientSock, "LOGIN_SUCCESS\n", 14, 0);
    } else {
        send(clientSock, "LOGIN_FAIL\n", 11, 0);
    }

    closesocket(clientSock);
    printf("클라이언트 연결 종료\n");
}