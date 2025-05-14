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

    closesocket(clientSock);
    printf("클라이언트 연결 종료\n");
}