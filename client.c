// LUMIS client.c
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUF_SIZE 1024

unsigned WINAPI RecvMsg(void* arg);

int main() {
    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN serv_adr;
    char msg[BUF_SIZE];

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_adr.sin_port = htons(PORT);

    if (connect(sock, (SOCKADDR*)&serv_adr, sizeof(serv_adr)) == -1) {
        puts("서버 연결 실패");
        return 1;
    }

    puts("서버에 연결됨. quit 입력 시 종료");
    // 로그인 정보 입력
    char id[50], pw[50];
    printf("ID 입력: ");
    fgets(id, sizeof(id), stdin);
    id[strcspn(id, "\n")] = 0;

    printf("PW 입력: ");
    fgets(pw, sizeof(pw), stdin);
    pw[strcspn(pw, "\n")] = 0;

    // 로그인 메시지 전송: ID//PW\n 형태
    sprintf(msg, "%s//%s\n", id, pw);
    send(sock, msg, strlen(msg), 0);

    // 로그인 응답 수신
    int recv_len = recv(sock, msg, BUF_SIZE - 1, 0);
    msg[recv_len] = 0;

    if (strcmp(msg, "LOGIN_SUCCESS\n") == 0) {
        printf("로그인 성공!\n");
    } else {
        printf("로그인 실패. 종료합니다.\n");
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    _beginthreadex(NULL, 0, RecvMsg, (void*)sock, 0, NULL);

    while (1) {
        fgets(msg, BUF_SIZE, stdin);
        if (strcmp(msg, "quit\n") == 0) break;

        send(sock, msg, strlen(msg), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}

unsigned WINAPI RecvMsg(void* arg) {
    SOCKET sock = (SOCKET)arg;
    char msg[BUF_SIZE];
    int str_len;

    while ((str_len = recv(sock, msg, BUF_SIZE - 1, 0)) > 0) {
        msg[str_len] = 0;
        fputs(msg, stdout);
    }

    // 예외 처리: recv()가 0 이하로 리턴됐을 때
    if (str_len == 0) {
        printf("서버 연결이 종료되었습니다.\n");
    } else if (str_len == SOCKET_ERROR) {
        printf("데이터 수신 중 오류 발생!\n");
    }

    return 0;
}