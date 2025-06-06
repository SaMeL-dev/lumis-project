// LUMIS client.c
/* 과제 요구사항 상 스레드 동기화 및 핸들 제어가 불필요하여, MinGW 컴파일 환경에서
   사용할 수 있는 _beginthread 사용이 호환성 및 리소스 관리 측면에서 적합하다고 판단했습니다. */
#define _CRT_SECURE_NO_WARNINGS
#define USERS_FILE "src/users.txt"
#define BOOKLIST_FILE "src/booklist2-2.txt"
#include <stdio.h>
#include <winsock2.h>
#include <process.h>
#include <string.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUF_SIZE 1024

// 메뉴항목 출력 함수
void PrintMenu() {
    printf("\n===== 메뉴 =====\n");
    printf("1. 도서 검색\n");
    printf("2. 도서 추가\n");
    printf("3. 도서 삭제\n");
    printf("4. 도서 랭킹\n");
    printf("5. 도서 정보 수정\n");
    printf("6. 도서 목록 개수\n");
    printf("7. 수동 저장\n");
    printf("8. 종료\n");
    printf("선택: ");
}

int main() {
    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN serv_adr;
    char msg[BUF_SIZE];
    int recv_len;

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_adr.sin_port = htons(PORT);

    if (connect(sock, (SOCKADDR*)&serv_adr, sizeof(serv_adr)) == -1) {
        puts("서버 연결 실패");
        return 1;
    }

    puts("서버에 연결되었습니다. 8 입력 시 종료");
    // 로그인 정보 입력
    char id[50], pw[50];

    while (1) {
        // ID 입력
        printf("ID 입력: ");
        fgets(id, sizeof(id), stdin);
        id[strcspn(id, "\n")] = 0;  // 개행 문자 제거

        // PW 입력
        printf("PW 입력: ");
        fgets(pw, sizeof(pw), stdin);
        pw[strcspn(pw, "\n")] = 0;  // 개행 문자 제거

        // 로그인 요청 메시지 전송: "ID//PW" 형식
        sprintf(msg, "%s//%s", id, pw);
        send(sock, msg, strlen(msg), 0);

        // 로그인 응답 수신
        recv_len = recv(sock, msg, BUF_SIZE - 1, 0);
        msg[recv_len] = 0;  // 문자열 종료 처리

        if (strcmp(msg, "LOGIN_SUCCESS\n") == 0) {
            printf("로그인 성공!\n");
            break;  // 로그인 성공 → 메뉴로 이동
        } else {
            // 로그인 실패 시 다시 입력
            printf("로그인 실패. 다시 시도하세요.\n\n");
        }
    }

    while (1) {
        // 메뉴항목 출력 함수 호출
        PrintMenu();
        // 메뉴 번호 입력받기
        fgets(msg, 4, stdin);

        // 1번 도서 검색 기능
        if (strcmp(msg, "1\n") == 0) {
            // 도서 검색
            printf("검색할 키워드 입력: ");
            fgets(msg, BUF_SIZE, stdin);
            msg[strcspn(msg, "\n")] = 0; // 개행 제거

            // 서버에 SEARCH 명령어 전송
            char send_buf[BUF_SIZE];
            sprintf(send_buf, "SEARCH\t%s\n", msg);
            send(sock, send_buf, strlen(send_buf), 0);

            // 서버 응답 수신
            int recv_len = recv(sock, msg, BUF_SIZE - 1, 0);
            msg[recv_len] = 0;
            printf("검색 결과:%s\n", msg);
        }

        // 2번 도서 추가 기능
        else if (strcmp(msg, "2\n") == 0) {
            // 도서 추가
            char title[100], author[100], rating[10];

            printf("도서명 입력: ");
            fgets(title, sizeof(title), stdin);
            title[strcspn(title, "\n")] = 0;

            printf("저자명 입력: ");
            fgets(author, sizeof(author), stdin);
            author[strcspn(author, "\n")] = 0;

            printf("평점 입력 (숫자): ");
            fgets(rating, sizeof(rating), stdin);
            rating[strcspn(rating, "\n")] = 0;

            char send_buf[BUF_SIZE];
            sprintf(send_buf, "ADD\t%s\t%s\t%s\n", title, author, rating);
            send(sock, send_buf, strlen(send_buf), 0);

            // 서버 응답 수신
            int recv_len = recv(sock, msg, BUF_SIZE - 1, 0);
            msg[recv_len] = 0;
            printf("서버 응답: %s\n", msg);
        }

        // 3번 도서 삭제 기능
        else if (strcmp(msg, "3\n") == 0) {
            char title[100];
            printf("삭제할 도서명 입력: ");
            fgets(title, sizeof(title), stdin);
            title[strcspn(title, "\n")] = 0;

            char send_buf[BUF_SIZE];
            sprintf(send_buf, "DELETE\t%s\n", title);
            send(sock, send_buf, strlen(send_buf), 0);

            // 서버 응답 수신 (구분자 "##END##\n"가 나타날 때까지)
            char recv_buf[BUF_SIZE * 2] = {0};  // 응답을 모을 버퍼
            int total_received = 0;
            while (1) {
                int recv_len = recv(sock, recv_buf + total_received, BUF_SIZE - 1, 0);
                if (recv_len <= 0) break;
                total_received += recv_len;
                recv_buf[total_received] = '\0';
                if (strstr(recv_buf, "##END##\n") != NULL) {  // 구분자 체크
                    break;
                }
            }

            // 구분자 이후의 내용을 제거
            char* delimiter = strstr(recv_buf, "##END##\n");
            if(delimiter) {
                *delimiter = '\0';
            }
            printf("서버 응답:%s\n", recv_buf);
        }

        // 4번 도서 랭킹 기능
        else if (strcmp(msg, "4\n") == 0) {     
            // 도서 랭킹 요청 전송
            send(sock, "RANKING\n", strlen("RANKING\n"), 0);

            // 서버 응답 수신: "##END##\n" 구분자까지 읽어들임
            char totalResponse[BUF_SIZE * 10] = "";  // 충분히 큰 버퍼 (상황에 맞게 조정)
            char buffer[BUF_SIZE] = {0};
            int bytes;
            
            while (1) {
                bytes = recv(sock, buffer, BUF_SIZE - 1, 0);
                if (bytes == SOCKET_ERROR) {
                    printf("서버 응답 수신 오류\n");
                    break;
                }
                if (bytes == 0) {
                    printf("서버 연결 종료됨\n");
                    break;
                }
                buffer[bytes] = '\0';
                strcat(totalResponse, buffer);
                // "##END##\n" 구분자가 포함되어 있으면 종료
                if (strstr(totalResponse, "##END##\n") != NULL) {
                    break;
                }
                memset(buffer, 0, BUF_SIZE);
            }
            
            // 구분자 "##END##\n" 제거
            char* delimiter = strstr(totalResponse, "##END##\n");
            if (delimiter) {
                *delimiter = '\0';
            }
            printf("도서 랭킹:\n%s\n", totalResponse);
        }

        // 5번 도서 정보 수정 기능
        else if (strcmp(msg, "5\n") == 0) {
            // 도서 정보 수정
            char old_title[100], new_title[100], new_author[100], new_rating[10];

            printf("수정할 도서명 입력: ");
            fgets(old_title, sizeof(old_title), stdin);
            old_title[strcspn(old_title, "\n")] = 0;

            printf("새 도서명 입력: ");
            fgets(new_title, sizeof(new_title), stdin);
            new_title[strcspn(new_title, "\n")] = 0;

            printf("새 저자명 입력: ");
            fgets(new_author, sizeof(new_author), stdin);
            new_author[strcspn(new_author, "\n")] = 0;

            printf("새 평점 입력 (숫자): ");
            fgets(new_rating, sizeof(new_rating), stdin);
            new_rating[strcspn(new_rating, "\n")] = 0;

            char send_buf[BUF_SIZE];
            sprintf(send_buf, "MODIFY\t%s\t%s\t%s\t%s\n", old_title, new_title, new_author, new_rating);
            send(sock, send_buf, strlen(send_buf), 0);

            // 서버 응답 수신
            int recv_len = recv(sock, msg, BUF_SIZE - 1, 0);
            msg[recv_len] = 0;
            printf("서버 응답: %s\n", msg);
        }

        // 6번 도서 목록 개수 출력 기능
        else if (strcmp(msg, "6\n") == 0) {
            // 도서 목록 개수 요청
            send(sock, "COUNT\n", strlen("COUNT\n"), 0);

            // 서버 응답 수신
            int recv_len = recv(sock, msg, BUF_SIZE - 1, 0);
            msg[recv_len] = 0;
            printf("현재 등록된 도서 개수: %s권\n", msg);
        }

        // 7번 수동 저장 기능
        else if (strcmp(msg, "7\n") == 0) {
            // 수동 저장 요청
            send(sock, "SAVE\n", strlen("SAVE\n"), 0);

            // 서버 응답 수신
            int recv_len = recv(sock, msg, BUF_SIZE - 1, 0);
            msg[recv_len] = 0;
            printf("서버 응답: %s\n", msg);
        }

        // 8번 종료 기능
        else if (strcmp(msg, "8\n") == 0) {
            // 로그아웃 요청
            send(sock, "LOGOUT\n", strlen("LOGOUT\n"), 0);

            // 서버 응답 수신
            int recv_len = recv(sock, msg, BUF_SIZE - 1, 0);
            msg[recv_len] = 0;
            printf("서버 응답: %s\n", msg);

            break;  // 종료
        }

        else {
            // 유효하지 않은 입력 처리
            printf("잘못된 입력입니다. 메뉴 번호를 다시 선택하세요.\n");
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}