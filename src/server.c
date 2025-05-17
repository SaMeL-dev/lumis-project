// LUMIS server.c
#define _CRT_SECURE_NO_WARNINGS
#define USERS_FILE "src/users.txt"
#define BOOKLIST_FILE "src/booklist2-2.txt"
#include <stdio.h>
#include <winsock2.h>
#include <process.h>
#include <string.h>

#define PORT 12345
#define BUF_SIZE 1024

void SendMenu(SOCKET clientSock);
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

        // 클라이언트 접속 안내 및 IP 출력
        printf("클라이언트 접속됨, IP: %s\n", inet_ntoa(clnt_adr.sin_addr));

        _beginthread(HandleClient, 0, (void*)clientSock);
    }
    closesocket(serv_sock);
    WSACleanup();
    return 0;
}

// 메뉴 창 불러오기 함수
void SendMenu(SOCKET clientSock) {
    const char* menu = 
        "===== 메뉴 =====\n"
        "1. 도서 검색\n"
        "2. 도서 추가\n"
        "3. 도서 삭제\n"
        "4. 도서 랭킹\n"
        "5. 도서 정보 수정\n"
        "6. 도서 목록 개수\n"
        "7. 수동 저장\n"
        "8. 종료\n"
        "선택: ";

    send(clientSock, menu, strlen(menu), 0);
}

// 클라이언트 요청 처리 스레드 함수
void HandleClient(void* arg) {
    SOCKET clientSock = (SOCKET)arg;
    char buf[BUF_SIZE];
    int str_len;
    int login_success = 0;

    while (!login_success) {
        recv(clientSock, buf, BUF_SIZE - 1, 0);
        buf[strcspn(buf, "\n")] = 0;  // 개행 제거

        FILE* fp = fopen(USERS_FILE, "r");
        char line[BUF_SIZE];

        if (fp != NULL) {
            while (fgets(line, sizeof(line), fp)) {
                line[strcspn(line, "\n")] = 0;  // 개행 제거

                if (strcmp(line, buf) == 0) {
                    login_success = 1;
                    break;
                }
            }
            fclose(fp);
        }

        if (login_success) {
            send(clientSock, "LOGIN_SUCCESS\n", strlen("LOGIN_SUCCESS\n"), 0);
        } else {
            send(clientSock, "LOGIN_FAIL\n", strlen("LOGIN_FAIL\n"), 0);
        }
    }

    while ((str_len = recv(clientSock, buf, BUF_SIZE -1, 0)) != 0) {
        buf[str_len] = '\0';
        printf("[수신] %s\n", buf);

        buf[strcspn(buf, "\n")] = 0; // 개행 제거

        char* command = strtok(buf, "\t");
        SendMenu(clientSock);

        // 1번 도서 검색 기능
        if (strcmp(command, "SEARCH") == 0) {
            char* keyword = strtok(NULL, "\t");
            FILE* fp = fopen(BOOKLIST_FILE, "r");
            char line[BUF_SIZE];
            char result[BUF_SIZE * 5] = "";  // 결과 누적 버퍼

            if (fp != NULL) {
                while (fgets(line, sizeof(line), fp)) {
                    if (strstr(line, keyword)) {  // 키워드 포함 여부 검사
                        strcat(result, line);
                    }
                }
                fclose(fp);
            }

            if (strlen(result) == 0) {
                strcpy(result, "검색 결과가 없습니다.\n");
            }

            send(clientSock, result, strlen(result), 0);

            SendMenu(clientSock);
        }

        // 2번 도서 추가 기능
        else if (strcmp(command, "ADD") == 0) {
            char* title = strtok(NULL, "\t");
            char* author = strtok(NULL, "\t");
            char* rating = strtok(NULL, "\t");

            if (title && author && rating) {
                // 책 개수 파악
                FILE* fp_count = fopen(BOOKLIST_FILE, "r");
                int count = 0;
                char line[BUF_SIZE];
                if (fp_count != NULL) {
                    while (fgets(line, sizeof(line), fp_count)) {
                        count++;
                    }
                    fclose(fp_count);
                }

                int new_id = count + 1;  // 다음 순번

                // 파일에 추가
                FILE* fp = fopen(BOOKLIST_FILE, "a");
                if (fp != NULL) {
                    fprintf(fp, "%d\t%s\t%s\t%s\n", new_id, title, author, rating);
                    fclose(fp);
                    send(clientSock, "도서 추가 성공\n", 17, 0);
                } else {
                    send(clientSock, "파일 열기 실패\n", 16, 0);
                }
            } else {
                send(clientSock, "입력값 부족\n", 13, 0);
            }

            SendMenu(clientSock);  // 메뉴 재출력
        }

        // 3번 도서 삭제 기능
        else if (strcmp(command, "DELETE") == 0) {
            char* title = strtok(NULL, "\t");

            FILE* fp = fopen(BOOKLIST_FILE, "r");
            FILE* temp_fp = fopen("src/temp.txt", "w");
            char line[BUF_SIZE];
            int found = 0;

            if (fp != NULL && temp_fp != NULL) {
                while (fgets(line, sizeof(line), fp)) {
                    if (strstr(line, title) == NULL) {  // 도서명이 포함되지 않은 줄만 복사
                        fputs(line, temp_fp);
                    } else {
                        found = 1; // 삭제할 도서 발견
                    }
                }
                fclose(fp);
                fclose(temp_fp);

                remove(BOOKLIST_FILE);
                rename("src/temp.txt", BOOKLIST_FILE);

                if (found) {
                    send(clientSock, "도서 삭제 성공\n", 15, 0);
                    SendMenu(clientSock);
                } else {
                    send(clientSock, "도서 삭제 실패. 해당 도서가 존재하는지 확인하세요.\n", 27, 0);
                    SendMenu(clientSock);
                }
            } else {
                send(clientSock, "파일 열기 실패\n", 16, 0);
                SendMenu(clientSock);
            }
        }

        // 4번 도서 랭킹 기능
        else if (strcmp(command, "RANKING") == 0) {
            FILE* fp = fopen(BOOKLIST_FILE, "r");
            char lines[700][BUF_SIZE];
            float ratings[700];
            int count = 0;

            if (fp != NULL) {
                while (fgets(lines[count], sizeof(lines[count]), fp)) {
                    char temp[BUF_SIZE];
                    strcpy(temp, lines[count]);

                    strtok(temp, "\t");
                    strtok(NULL, "\t");
                    char* rating_str = strtok(NULL, "\n"); // 평점 추출

                    if (rating_str != NULL) {
                        ratings[count] = atof(rating_str);
                        count++;
                    }
                }
                fclose(fp);

                // 선택 정렬로 평점 내림차순 정렬
                for (int i = 0; i < count - 1; i++) {
                    int max_idx = i;
                    for (int j = i + 1; j < count; j++) {
                        if (ratings[j] > ratings[max_idx]) {
                            max_idx = j;
                        }
                    }
                    if (max_idx != i) {
                        // Swap 평점
                        float temp_rating = ratings[i];
                        ratings[i] = ratings[max_idx];
                        ratings[max_idx] = temp_rating;

                        // Swap 도서 정보
                        char temp_line[BUF_SIZE];
                        strcpy(temp_line, lines[i]);
                        strcpy(lines[i], lines[max_idx]);
                        strcpy(lines[max_idx], temp_line);
                    }
                }

                char result[BUF_SIZE * 5] = "";
                for (int i = 0; i < count; i++) {
                    strcat(result, lines[i]);
                }

                if (count == 0) {
                    strcpy(result, "등록된 도서가 없습니다.\n");
                }

                send(clientSock, result, strlen(result), 0);
                SendMenu(clientSock);
            } else {
                send(clientSock, "파일 열기 실패\n", 16, 0);
                SendMenu(clientSock);
            }
        }

        // 5번 도서 정보 수정 기능
        else if (strcmp(command, "MODIFY") == 0) {
            char* old_title = strtok(NULL, "\t");
            char* new_title = strtok(NULL, "\t");
            char* new_author = strtok(NULL, "\t");
            char* new_rating = strtok(NULL, "\t");

            FILE* fp = fopen(BOOKLIST_FILE, "r");
            FILE* temp_fp = fopen("src/temp.txt", "w");
            char line[BUF_SIZE];
            int found = 0;

            if (fp != NULL && temp_fp != NULL) {
                while (fgets(line, sizeof(line), fp)) {
                    char temp_line[BUF_SIZE];
                    strcpy(temp_line, line);

                    char* title = strtok(temp_line, "\t");

                    if (title && strcmp(title, old_title) == 0) {
                        // 도서 정보 수정
                        fprintf(temp_fp, "%s\t%s\t%s\n", new_title, new_author, new_rating);
                        found = 1;
                    } else {
                        fputs(line, temp_fp);  // 기존 정보 그대로 복사
                    }
                }
                fclose(fp);
                fclose(temp_fp);

                remove(BOOKLIST_FILE);
                rename("src/temp.txt", BOOKLIST_FILE);

                if (found) {
                    send(clientSock, "도서 정보 수정 성공\n", 23, 0);
                    SendMenu(clientSock);
                } else {
                    send(clientSock, "도서 정보 수정 실패. 해당 도서가 존재하는지 확인하세요.\n", 36, 0);
                    SendMenu(clientSock);
                }
            } else {
                send(clientSock, "파일 열기 실패\n", 16, 0);
                SendMenu(clientSock);
            }
        }

        // 6번 도서 목록 개수 출력 기능
        else if (strcmp(command, "COUNT") == 0) {
            FILE* fp = fopen(BOOKLIST_FILE, "r");
            char line[BUF_SIZE];
            int count = 0;

            if (fp != NULL) {
                while (fgets(line, sizeof(line), fp)) {
                    count++;
                }
                fclose(fp);

                char result[10];
                sprintf(result, "%d", count);  // 개수 문자열로 변환
                send(clientSock, result, strlen(result), 0);
                SendMenu(clientSock);
            } else {
                send(clientSock, "파일 열기 실패\n", 16, 0);
                SendMenu(clientSock);
            }
        }

        // 7번 수동 저장 기능
        else if (strcmp(command, "SAVE") == 0) {
            // 현재 모든 변경이 즉시 저장되고 있으므로, 별도 동작 없이 메시지 출력
            send(clientSock, "현재 상태가 파일에 저장되었습니다.\n", 36, 0);
            SendMenu(clientSock);
        }

        // 8번 종료 기능
        else if (strcmp(command, "LOGOUT") == 0) {
            send(clientSock, "LOGOUT_SUCCESS\n", 16, 0);
            break;  // 클라이언트와의 연결 종료
        }

        // 유효하지 않은 입력값 예외 처리
        else {
            send(clientSock, "유효하지 않은 명령입니다.\n", 30, 0);
            SendMenu(clientSock);
        }
    }
    closesocket(clientSock);
    _endthread();
}