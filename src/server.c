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

void HandleClient(void* arg);
void ReorderBooks();

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

// 도서 목록을 불러와 순번 재조정하는 함수
void ReorderBooks() {
    FILE* fp = fopen(BOOKLIST_FILE, "r");
    char lines[700][BUF_SIZE];
    int count = 0;

    if (fp != NULL) {
        while (fgets(lines[count], sizeof(lines[count]), fp)) {
            char temp[BUF_SIZE];
            strcpy(temp, lines[count]);

            strtok(temp, "\t"); // 기존 번호 제거
            char* title = strtok(NULL, "\t");
            char* author = strtok(NULL, "\t");
            char* rating = strtok(NULL, "\n");

            if (title != NULL && author != NULL && rating != NULL) {
                sprintf(lines[count], "%d\t%s\t%s\t%s\n", count + 1, title, author, rating);
                count++;
            }
        }
        fclose(fp);

        // 새로 번호가 정렬된 리스트를 파일에 다시 저장
        fp = fopen(BOOKLIST_FILE, "w");
        for (int i = 0; i < count; i++) {
            fputs(lines[i], fp);
        }
        fclose(fp);
    }
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
                    send(clientSock, "도서 추가 성공\n", 22, 0);
                } else {
                    send(clientSock, "파일 열기 실패\n", 22, 0);
                }
            } else {
                send(clientSock, "입력값 부족\n", 18, 0);
            }
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

                // 번호 재정렬 함수 호출
                ReorderBooks();

                if (found) {
                    send(clientSock, "도서 삭제 완료\n", strlen("도서 삭제 완료\n"), 0);
                } else {
                    send(clientSock, "도서 삭제 실패. 해당 도서가 존재하는지 확인하세요.\n", strlen("도서 삭제 실패. 해당 도서가 존재하는지 확인하세요.\n"), 0);
                }
            } else {
                send(clientSock, "파일 열기 실패\n", strlen("파일 열기 실패\n"), 0);
            }
            // 응답의 끝을 알리는 구분자 전송
            send(clientSock, "##END##\n", strlen("##END##\n"), 0);
        }

        // 4번 도서 랭킹 기능
        else if (strcmp(command, "RANKING") == 0) {
            FILE* fp = fopen(BOOKLIST_FILE, "r");
            if (fp == NULL) {
                send(clientSock, "파일 열기 실패\n", strlen("파일 열기 실패\n"), 0);
            } else {
                char lines[700][BUF_SIZE];
                float ratings[700];
                int count = 0;

                // 파일에서 각 줄 불러오기
                while (count < 700 && fgets(lines[count], BUF_SIZE, fp) != NULL) {
                    char temp[BUF_SIZE];
                    strcpy(temp, lines[count]);

                    // 파일 형식: 번호\t도서명\t저자명\t평점\n
                    strtok(temp, "\t");                   // 번호 토큰
                    strtok(NULL, "\t");                   // 도서명 토큰
                    strtok(NULL, "\t");                   // 저자명 토큰
                    char* rating_str = strtok(NULL, "\n");  // 평점 토큰 (네 번째)
                    if (rating_str != NULL) {
                        ratings[count] = atof(rating_str);
                        count++;
                    }
                }
                fclose(fp);

                // 내림차순 정렬 (선택 정렬)
                for (int i = 0; i < count - 1; i++) {
                    for (int j = i + 1; j < count; j++) {
                        if (ratings[j] > ratings[i]) {
                            // 평점 스왑
                            float tmp_rating = ratings[i];
                            ratings[i] = ratings[j];
                            ratings[j] = tmp_rating;
                            
                            // 해당 줄의 스왑
                            char tmp_line[BUF_SIZE];
                            strcpy(tmp_line, lines[i]);
                            strcpy(lines[i], lines[j]);
                            strcpy(lines[j], tmp_line);
                        }
                    }
                }

                // 결과 문자열 누적
                char result[BUF_SIZE * 5] = "";
                for (int i = 0; i < count; i++) {
                    strcat(result, lines[i]);
                }
                if (count == 0) {
                    strcpy(result, "등록된 도서가 없습니다.\n");
                }
                // 정렬 결과 전송
                send(clientSock, result, strlen(result), 0);
                // 응답의 끝을 알리는 구분자 전송
                send(clientSock, "##END##\n", strlen("##END##\n"), 0);
            }
        }

        // 5번 도서 정보 수정 기능
        else if (strcmp(command, "MODIFY") == 0) {
            char* old_title = strtok(NULL, "\t");       // 수정할 기존 도서명
            char* new_title = strtok(NULL, "\t");       // 새 도서명
            char* new_author = strtok(NULL, "\t");      // 새 저자명
            char* new_rating = strtok(NULL, "\n");      // 새 평점

            FILE* fp = fopen(BOOKLIST_FILE, "r");
            FILE* temp_fp = fopen("src/temp.txt", "w");
            char line[BUF_SIZE];
            int found = 0;

            if (fp != NULL && temp_fp != NULL) {
                while (fgets(line, sizeof(line), fp)) {
                    char temp_line[BUF_SIZE];
                    strcpy(temp_line, line);

                    // 원래 저장된 형식: 번호\t도서명\t저자명\t평점\n
                    // 번호 토큰 추출
                    char* record_num = strtok(temp_line, "\t");
                    // 실제 도서명은 두 번째 토큰
                    char* curr_title = strtok(NULL, "\t");

                    // 기존 도서명이 일치하면 새 정보로 수정
                    if (curr_title != NULL && strcmp(curr_title, old_title) == 0) {
                        // 기존 번호(record_num)는 그대로 유지하면서 새 도서명, 새 저자명, 새 평점을 저장
                        fprintf(temp_fp, "%s\t%s\t%s\t%s\n", record_num, new_title, new_author, new_rating);
                        found = 1;
                    } else {
                        // 그 외의 경우는 원래 줄을 그대로 복사
                        fputs(line, temp_fp);
                    }
                }
                fclose(fp);
                fclose(temp_fp);

                // 기존 파일 삭제 후 임시 파일 이름 변경
                remove(BOOKLIST_FILE);
                rename("src/temp.txt", BOOKLIST_FILE);

                if (found) {
                    send(clientSock, "도서 정보 수정 성공\n", 29, 0);
                } else {
                    send(clientSock, "도서 정보 수정 실패. 해당 도서가 존재하는지 확인하세요.\n", 80, 0);
                }
            } else {
                send(clientSock, "파일 열기 실패\n", 22, 0);
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
            } else {
                send(clientSock, "파일 열기 실패\n", 16, 0);
            }
        }

        // 7번 수동 저장 기능
        else if (strcmp(command, "SAVE") == 0) {
            // 현재 모든 변경이 즉시 저장되고 있으므로, 별도 동작 없이 메시지 출력
            send(clientSock, "현재 상태가 파일에 저장되었습니다.\n", 51, 0);
        }

        // 8번 종료 기능
        else if (strcmp(command, "LOGOUT") == 0) {
            send(clientSock, "LOGOUT_SUCCESS\n", 16, 0);
            break;  // 클라이언트와의 연결 종료
        }

        // 유효하지 않은 입력값 예외 처리
        else {
            send(clientSock, "유효하지 않은 명령입니다.\n", 30, 0);
        }
    }
    closesocket(clientSock);
    _endthread();
}