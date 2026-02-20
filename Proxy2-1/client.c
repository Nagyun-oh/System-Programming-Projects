///////////////////////////////////////////////////////////////////////////////////////////////
// File Name	: client.c								     //
// Date		: 2024/05/01								     //
// Os		: Ubuntu 20.04 64bits							     //
// Author	: OH Nagyun								    //
// Student ID	: 2021202089								    //
// ----------------------------------------------------------------------		    //
// Title	: System Programming Assignment #2-1 (proxy server)			   //
// Description  : A client program that sends a URL to a server and displays the response. // 
// 		  The client exits when "bye" is entered.				  //
////////////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFSIZE 1024
#define PORT 40000
int main()
{
    int cd;
    struct sockaddr_in server_addr;
    char buf[BUFFSIZE], response[BUFFSIZE];

   // 소켓 생성 
   if((cd = socket(AF_INET, SOCK_STREAM, 0))<0)
   {
	perror("socket"); // 소켓 생성 실패시 에러 메시지
	exit(1); // 프로그램 종료
   }
    
    // 서버 주소 설정 (127.0.0.1:PORT)
    server_addr.sin_family = AF_INET;  // IPv4
    server_addr.sin_port = htons(PORT); // 포트 번호 -> 네트워크 바이트로 변환
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); // 문자열 ip -> 이진수 ip

    // 서버에 연결 요청
    if (connect(cd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); //연결 실패시 에러 메시지 출력
        exit(1); // 프로그램 종료
    }

    //사용자로부터 URL을 입력받고, 서버에 전송
    while (1) 
    {
	// URL 입력처리
        printf("input URL> ");
        fgets(buf, sizeof(buf), stdin);
	buf[strcspn(buf,"\n")] = 0;

	// 입력받은 URL을 서버에 전송
        write(cd, buf, strlen(buf));

	// bye 입력시 종료
        if (strncmp(buf, "bye", 3) == 0) {
            break;
        }

        memset(response, 0, sizeof(response)); // 응답버퍼 초기화
        read(cd, response,sizeof(response) - 1); // 서버로부터 응답 읽기
        printf("%s", response);  // 응답 출력
    }

    close(cd); //소켓 닫기
    return 0;
}


