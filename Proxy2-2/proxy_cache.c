///////////////////////////////////////////////////////////////////////////////////////////////////
// File Name	: proxy_cache.c						      			                              //
// Date		: 2024/05/08						     			                                  //
// Os		: Ubuntu 20.04 64bits			                     		                    	  //
// Author	: OH Nagyun					             		                                	  //
// Student ID	: 2021202089						    		                            	  //
// ---------------------------------------------------------------------   		            	  //
// Title	: System Programming Assignment #2-2 (proxy server)				                      //
// Description  :										                                          //
// 	- 웹 브라우저로부터 URL을 입력받고, HTTP request를 받음 			                                  //
// 	- 자식 프로세서는 웹 브라우저의 요청에 응답후 종료	                                                    //
//	- HTTP request header로 부터 host정보 (url)	 추출                                                //
// 	- Assignment 1-2 수행										                                  //
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <openssl/sha.h> //SHA1()
#include <dirent.h>
#include <pwd.h> 
#include <sys/types.h> //mkdir umask
#include <sys/stat.h> // mkdir umask
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define BUFFSIZE    1024
#define PORT        39999
#define LOG_DIR		"/logfile"
#define LOG_FILE	"/logfile/logfile.txt"

// 함수 선언
int is_already_logged(const char* url);
char* getHomeDir(char* home);
char* sha1_hash(char* input_url, char* hashed_url);
void Make_directory_for_real(const char* path);
void Make_directory_file(char* hashed_url);
void Create_log_directory_with_file();
int HIT_OR_MISS(char* hashed_url);
void Write_log_in_file( const char* url, const char* hashed_url,const char* type);
void Write_termination(int hit,int miss,double run_time);

static void handler()
{
	pid_t pid;
	int status;
	while((pid=waitpid(-1,&status,WNOHANG)>0));
}

int main()
{
    // hit, miss count
    int hit_count = 0;
    int miss_count = 0;

    struct sockaddr_in server_addr,client_addr;
    int socekt_fd,client_fd;
    socklen_t len;
    
    signal(SIGCHLD,handler) ; // 좀비 프로세서 방지

    //소켓 생성
    if((socekt_fd=socket(PF_INET,SOCK_STREAM,0))<0)
    {
        printf("Server : Can't open stream socket\n");
        return 0;
    }

    //서버 주소 정보 초기화
    bzero((char*)&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 모든 IP로부터 연결 허용
    server_addr.sin_port = htons(PORT); // 지정 포트로 바인딩

    // 소켓에 주소 바인딩
    if(bind(socekt_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        printf("Server : Can't bind local address\n");
        return 0;
    }

    //클라이언트 연결 대기
    listen(socekt_fd,5); 

    // 메인 루프 : 웹 요청을 계속해서 처리
    while(1)
    {
        struct in_addr inet_client_address;

        char buf[BUFFSIZE]={0,}; // 클라이언트 요청 저장
        char response_header[BUFFSIZE] = {0,}; //header
        char response_message[2048]={0,};  // message
        char tmp[BUFFSIZE] = {0,};
        char method[20] = {0,};
        char url[BUFFSIZE] = {0,}; // 요청 URL 저장
        
        char* tok = NULL;

        len = sizeof(client_addr);
        //클라이언트 접속 대기 및 수락
        client_fd = accept(socekt_fd,(struct sockaddr*)&client_addr,&len);
        if(client_fd<0)
        {
            printf("Server : accept failed\n");
            return 0; // 에러 발생시 서버 종료
        }

        // 접속한 클라이언트 주소 정보 출력
        inet_client_address.s_addr = client_addr.sin_addr.s_addr;
        printf("[%s : %d] client was connected\n",inet_ntoa(inet_client_address),client_addr.sin_port);

        // 헤더 및 메시지 초기화
        memset(response_header,0,sizeof(response_header));
        memset(response_message,0,sizeof(response_message));
        
        read(client_fd,buf,BUFFSIZE); // 클라이언트로부터 요청 메시지 읽기
        strcpy(tmp,buf); // 원본 복사

        // 요청 로그 출력
        puts("===============================================");
        printf("Request from [%s : %d]\n",inet_ntoa(inet_client_address),client_addr.sin_port);
        puts(buf);
        puts("===============================================\n");

        // 요청 메시지에서 GET,URL 추출
        tok = strtok(tmp," ");
        strcpy(method,tok);

        // url 추출
        if(strcmp(method,"GET") == 0)
        {
            tok = strtok(NULL," ");
            strcpy(url,tok);
        }

        // fork()를 이용하여, 자식 프로세서에서 요청 처리
        pid_t pid =fork();
        if(pid==0) // child process
        { 
        // 요청 URL 해쉬화
        char hashed_url[41];
        sha1_hash(url,hashed_url);

        //캐시 여부 확인
        int cache_state = HIT_OR_MISS(hashed_url);

        if(cache_state){
            // HIT
            if((strcmp(method, "GET") == 0) && strstr(buf, "Upgrade-Insecure-Requests: 1"))
            {
            if(!is_already_logged(url))
            {
            Write_log_in_file(url, hashed_url, "Hit"); // log 작성
            hit_count++;
            }
            }   
            // 응답 메시지 구성
            snprintf(response_message,sizeof(response_message),
                "<h1>HIT</h1><br>"
                "%s:%d<br>"
                "%s<br>"
                "kw:2021202089",inet_ntoa(inet_client_address),client_addr.sin_port,url);
        }
        else{
            // MISS
            Make_directory_file(hashed_url);
            if((strcmp(method, "GET") == 0) && strstr(buf, "Upgrade-Insecure-Requests: 1"))
            {
            if(!is_already_logged(url)){
            Write_log_in_file(url, hashed_url, "Miss"); // log 작성
            miss_count++;
            }
            }
            // 응답 메시지 구성
            snprintf(response_message,sizeof(response_message),
                "<h1>MISS</h1><br>"
                "%s:%d<br>"
                "%s<br>"
                "kw:2021202089",inet_ntoa(inet_client_address),client_addr.sin_port,url);
        }
        //응답 헤더 생성
        sprintf(response_header,
        "HTTP/1.0 200 OK\r\n"
        "Server:2018 simple web_server\r\n"
        "Content-length:%lu\r\n"
        "Content-type:text/html\r\n\r\n",strlen(response_message));

        // 응답 전송
        write(client_fd,response_header,strlen(response_header));
        write(client_fd,response_message,strlen(response_message));

        printf("[%s : %d] client was disconnected\n",inet_ntoa(inet_client_address),client_addr.sin_port);
        close(client_fd); // end child process
        exit(0); // end child process
       }
       else if(pid>0) 
       {
        // 부모 프로세서는 다음 요청 받기 위해 client_fd만 닫고 반복
        close(client_fd);
       }
}
    close(socekt_fd); // 서버 종료시 소켓을 닫고 리소스를 정리
   
    return 0;
}
///////////////////////////////////////////////////////////////////////////////
// Function: is_already_logged
// Input:
//   - url : 클라이언트로부터 받은 url
// Output:
//  - 1 : 로그 파일에 해당 URL이 이미 존재한다.
//  - 0 : 로그 파일에 해당 URL 존재 하지 않음
// Purpose:
//   - 로그 파일을 열어 각 줄을 확인하며 해당 URL이 이미 기록되어 있는지 검사
//   - URL이 로그 파일에 이미 존재할 경우 중복 로그 작성을 방지하기 위해 사용
///////////////////////////////////////////////////////////////////////////////
int is_already_logged(const char* url)
{
    FILE* file = fopen(LOG_FILE,"r");
    if(!file)return 0;

    char line[BUFFSIZE];
    while(fgets(line,sizeof(line),file))
    {
        if(strstr(line,url))
        {
            fclose(file);
            return 1; // already exits
        }
    }
    fclose(file);
    return 0; // not exits
}

// 홈 디렉토리 경로 반환
char* getHomeDir(char* home)
{
	struct passwd* usr_info = getpwuid(getuid()); 
	strcpy(home, usr_info->pw_dir);
		
	return home;	
}
// SHA1 
char* sha1_hash(char* input_url, char* hashed_url)
{
	unsigned char hashed_160bits[20];  // 20byets(160bits)를 저장할 배열
	char hashed_hex[41];
	size_t  i;

	SHA1((unsigned char*)input_url,strlen(input_url),hashed_160bits); // SHA1()함수를 호출하여 input_url에 대해 해싱된 20byte 데이터를 hashed_160bits 에 저장

	for(i=0;i<sizeof(hashed_160bits);i++)
		sprintf(hashed_hex + i*2, "%02x", hashed_160bits[i]);  // hashed 

	strcpy(hashed_url,hashed_hex); // hashed_url 

	return hashed_url;
}

///////////////////////////////////////////////////////////////////////////////
// Function: Make_directory_for_real
// Input:
//   - path : 생성할 전체 디렉토리 경로 (예: /home/user/cache/xx/yy)
// Output:
//   - 없음 (void)
// Purpose:
//   - 입력된 전체 경로를 기준으로 중간 디렉토리를 포함하여 모든 디렉토리를 생성
//   - 이미 존재하는 디렉토리는 무시하고, 존재하지 않는 디렉토리만 생성
///////////////////////////////////////////////////////////////////////////////
void Make_directory_for_real(const char* path) 
{
    char temp[100];
    char* p = NULL;
    int len;

    umask(0);
    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    
    if (temp[len - 1] == '/')  // 경로 마지막에 '/'가 있으면 제거
        temp[len - 1] = '\0';

    for (p = temp + 1; *p!='\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(temp, 0777);  // 중간 directory 
            *p = '/';
        }
    }
   
    mkdir(temp, 0777);  // 최종 directory 
}

///////////////////////////////////////////////////////////////////////////////
// Function: Make_directory_file
// Input:
//   - hashed_url : SHA1 해시된 URL 문자열 (총 40자리)
// Output:
//   - 없음 (void)
// Purpose:
//   - 해시된 URL을 기준으로 디렉토리 및 파일 경로를 구성
//   - ~/cache/abc/defg... 형태로 디렉토리를 생성하고, 해당 위치에 파일이 없으면 빈 파일 생성
///////////////////////////////////////////////////////////////////////////////
void Make_directory_file(char* hashed_url) {
    char dir_path[100], file_path[200];
    char home[50];
    char* homedir = getHomeDir(home);

    // 홈 디렉토리 주소 저장
    char cache_root[50];
    snprintf(cache_root, sizeof(cache_root), "%s/cache", homedir);

    // 해시화된  URL을 기반으로 디렉토리 생성
    snprintf(dir_path, sizeof(dir_path), "%s/%c%c%c", cache_root, hashed_url[0], hashed_url[1], hashed_url[2]);
    dir_path[sizeof(dir_path)-1] = '\0';

    // 디렉토리가 존재하지 않으면 생성
    if (access(dir_path, F_OK) == -1) {
        Make_directory_for_real(dir_path);
    }
    // 파일 경로를 설정
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, &hashed_url[3]);
    file_path[sizeof(file_path)-1] = '\0'; 

    // 파일이 존재하지 않으면 만들기
    if (access(file_path, F_OK) == -1) 
    {
        FILE* file = fopen(file_path, "w");
        if (file != NULL)
       	{
            fclose(file);   
    	}
    }

}

///////////////////////////////////////////////////////////////////////////////
// Function: Create_log_directory_with_file
// Input:
//   - 없음
// Output:
//   - 없음 (void)
// Purpose:
//   - 사용자의 홈 디렉토리 하위에 로그 디렉토리(`/logfile`)가 없으면 생성
//   - 로그 파일(`logfile.txt`)이 없으면 새로 생성
///////////////////////////////////////////////////////////////////////////////
void Create_log_directory_with_file()
{
    char home[50];
    getHomeDir(home);

    char log_dir[100];
    char log_file[100];

    snprintf(log_dir, sizeof(log_dir), "%s%s", home,LOG_DIR); // log 디렉토리 주소
    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE); //log 파일 주소

    if (access(log_dir, F_OK) == -1) mkdir(log_dir, 0777); //log 디렉토리에  0777권한 부여 하여 생성

    if (access(log_file, F_OK) == -1) // log 파일이 존재하지 않으면
    {
        FILE* file = fopen(log_file, "w"); // 파일 생성
        fclose(file); // 파일 닫기
    }
}

///////////////////////////////////////////////////////////////////////////////
// Function: HIT_OR_MISS
// Input:
//   - hashed_url : SHA1 해시된 URL 문자열 (총 40자리)
// Output:
//   - int : 1이면 HIT (파일 존재), 0이면 MISS (파일 없음)
// Purpose:
//   - 해시된 URL이 ~/cache/xxx 디렉토리에 존재하는지 확인
//   - 디렉토리 내부에 해당 해시의 나머지 부분과 일치하는 파일명이 있으면 HIT
///////////////////////////////////////////////////////////////////////////////
int HIT_OR_MISS(char* hashed_url)
{
    char home[50];
    char cache_dir[100];
    
    // hashed_url 에 대한 directory경로  구하기
    snprintf(cache_dir, sizeof(cache_dir), "%s/cache/%c%c%c", getHomeDir(home), hashed_url[0], hashed_url[1], hashed_url[2]);

    DIR* dir = opendir(cache_dir); // opendir(hashed_url에 대한 directory명)

    if (dir == NULL) return 0;  // MISS

    struct dirent* dir_read;
    while ((dir_read = readdir(dir)) != NULL) 
    {
	 // 디렉토리 내의 파일명이 hashed_url과 동일하면 HIT
        if (strcmp(dir_read->d_name, &hashed_url[3]) == 0)
       	{ 
            closedir(dir);
            return 1; // HIT
        }
    }

    // 위의 while문에서 값을 return 하지 않았다면 MISS임
    closedir(dir);
    return 0; // MISS
}

///////////////////////////////////////////////////////////////////////////////
// Function: Write_log_in_file
// Input:
//   - url         : 클라이언트가 요청한 원래 URL
//   - hashed_url  : SHA1으로 해시된 URL
//   - type        : "Hit" 또는 "Miss" 문자열
// Output:
//   - 없음 (로그 파일에 내용 기록)
// Purpose:
//   - 클라이언트 요청 처리 결과(Hit/Miss)를 로그 파일에 기록
//   - 처리 시간과 PID 정보를 포함한 로그 메시지 작성
///////////////////////////////////////////////////////////////////////////////
void Write_log_in_file( const char* url, const char* hashed_url,const char* type)
{
    char home[50];
    char log_file[100];
    char time_buf[50];

    getHomeDir(home); // 홈 디렉토리 경로

    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE); // 로그파일명  주소 얻기

    time_t cur_time = time(NULL);  // 현재 시간
    struct tm* t = localtime(&cur_time); // broken_time 변수 얻기
    // "year/month/day hour/min/sec" 시간 형식 얻기
    snprintf(time_buf, sizeof(time_buf), "%d/%d/%d, %02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);


    FILE* file = fopen(log_file, "a"); // 파일 append 모드로 열기
    // HIT
    if (strcmp(type, "Hit") == 0)
    {
        fprintf(file, "[Hit] ServerPID : %d | %c%c%c/%s - [%s]\n",getpid() ,hashed_url[0], hashed_url[1], hashed_url[2], &hashed_url[3], time_buf);
        fprintf(file, "[Hit]%s\n", url);
    }
    // MISS
    else if (strcmp(type, "Miss") == 0)
    {
        fprintf(file, "[Miss] ServerPID : %d | %s - [%s]\n",getpid(), url, time_buf);
    }

    fclose(file); // 파일 닫기
}
///////////////////////////////////////////////////////////////////////////////
// Function: Write_termination
// Input:
//   - hit       : 처리한 HIT 요청 수
//   - miss      : 처리한 MISS 요청 수
//   - run_time  : 클라이언트와 연결된 시간 (초)
// Output:
//   - 없음 (종료 로그를 파일에 기록)
// Purpose:
//   - 클라이언트와의 연결이 종료될 때 로그 파일에 요약 정보 기록
//   - 서버 PID, 실행 시간, HIT/MISS 수를 남김
///////////////////////////////////////////////////////////////////////////////
void Write_termination(int hit,int miss,double run_time)
{
    char home[50];
    char log_file[100];
    getHomeDir(home); // 홈 디렉토리 얻기

    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE); // 로그파일에 대한 주소명 얻기

    FILE* file = fopen(log_file, "a");

    if (file != NULL)
    {
         // 로그파일에 종료 로그 적기
        fprintf(file, "[Terminated] ServerPID : %d | run time: %.0f sec. #request hit : %d, miss : %d\n",getpid() ,run_time, hit, miss);
        fclose(file);
    }

}
