///////////////////////////////////////////////////////////////////////////////////////////////////
// File Name	: server.c						      			                                 
// Date		: 2024/05/01						     			 
// Os		: Ubuntu 20.04 64bits			                     			
// Author	: OH Nagyun					             			
// Student ID	: 2021202089						    			
// ---------------------------------------------------------------------   			
// Title	: System Programming Assignment #2-1 (proxy server)			
// Description  :										
// 		server.c => listens for incoming client connections, 			
// 		processes requests, and sends appropriate responses back to the clients.     
//		It works in conjunction with the client program to handle URL requests.       
// 											     
////////////////////////////////////////////////////////////////////////////////////////////////

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

#define BUFFSIZE	1024
#define PORT		40000
#define LOG_DIR		"/logfile"
#define LOG_FILE	"/logfile/logfile.txt"

// 함수 선언
void handle_client(int client_sock,struct sockaddr_in client);
char* getHomeDir(char* home);
char* sha1_hash(char* input_url, char* hashed_url);
void make_directory(const char* path);
void make_directory_and_file(char* hashed_url);
void create_log_directory_with_file();
int HIT_OR_MISS(char* hashed_url);
void write_log_in_file( const char* url, const char* hashed_url,const char* type);
void write_termination(int hit,int miss,double run_time);

static void handler()
{
	pid_t pid;
	int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0);
}

int main()
{
	umask(0); 

    // 필요한 변수 선언
	struct sockaddr_in server_addr,client_addr;
	int sd,cd; // server socket, client socket
	int clientlen = sizeof(client_addr);
	pid_t pid;

    // 자식 프로세스가 종료될 때 좀비프로세스가 되는것을 방지
	signal(SIGCHLD,(void *)handler);

	// 1. 소켓 생성(IPv4, TCP)
	if((sd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket");
		exit(1);
	}

	// 2. 서버 주소 설정
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family		= AF_INET;                  // IPv4
	server_addr.sin_addr.s_addr	= inet_addr("127.0.0.1");   // ip address
	server_addr.sin_port		= htons(PORT);              // host byte order  -> network byte order

	// 3. socket 과 서버 주소 bind
	if(bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr))<0) {
		perror("bind");
		exit(1);
	}

	// 4. 클라이언트 접속 대기 상태 (큐 크기는 5)
	if(listen(sd, 5)<0) {
		perror("listen");
		exit(1);
	}

    while(1){
        // 5. 클라이언트 연결 받기
        if((cd = accept(sd,(struct sockaddr*)&client_addr,&clientlen))<0) {
            perror("accept");
			exit(1); 
	    }	

	    pid = fork(); // 자식 프로세스 생성

        if(pid==0){
            close(sd); // 자식 프로세스 -> close server socket
            handle_client(cd,client_addr); // client가 입력한 URL에 대해서 Proxy 1-2 수행
        }else {
            close(cd); // 부모 프로세스 -> 클라이언트 소켓 닫고 대기
        }
    }

	close(sd); // 서버 소켓 닫기 (여기에 도달 하지는 않음)
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Function: handle_client
//
// Input:
//   - client_sock : 클라이언트 소켓
//   - client      : 클라이언트 주소 정보 (IP, 포트)
//
// Output:
//   - 없음 (void 반환, 내부에서 exit(0)로 자식 프로세스 종료)
//
// Purpose:
//   - 클라이언트 요청(URL)을 받아 처리하고, HIT/MISS 여부를 판단하여 응답
//   - 요청 결과를 로그로 남기고, "bye" 입력 시 통계와 함께 종료
///////////////////////////////////////////////////////////////////////////////
void handle_client(int client_sock,struct sockaddr_in client)
{
	char buf[BUFFSIZE];                 //  client 요청에 대한 버퍼
	char ip[INET_ADDRSTRLEN];           //  IP address 저장 버퍼
	int port = ntohs(client.sin_port);  //  클라이언트 포트번호를 올바르게 변환

	// hit miss에 대한  count 변수
	int hit_count=0;
	int miss_count=0;

    //  시작 시간 -> Terminated 문구 작성
	time_t start_time = time(NULL); 

	// 클라이언트 IP address 변환 (binary -> str)
    inet_ntop(AF_INET, &(client.sin_addr), ip, INET_ADDRSTRLEN); // ip 를 문자열로 변환

    printf("[%s : %d] client was connected\n", ip,port);

   	while (1) {
        	
        memset(buf, 0, sizeof(buf));
        int len = read(client_sock, buf, sizeof(buf)-1);
        if (len <=0)break; // error

		buf[len]='\0';
		buf[strcspn(buf, "\n")] = 0;  // fgets로 들어온 개행문자 제거

        	
        if (strncmp(buf, "bye", 3) == 0) 
        {		
            // 클라이언트 종료시 로그파일에 종료문장 작성 및 소켓 닫기       	
            double run_time = difftime(time(NULL), start_time);			
            write_termination(hit_count, miss_count, run_time);		
            close(client_sock);  	
            break;
        }

		char hashed_url[41];
		sha1_hash(buf, hashed_url);  // URL 해시

		if (HIT_OR_MISS(hashed_url)) { 
            // hit		
            write(client_sock, "HIT\n", 4);		
            write_log_in_file(buf, hashed_url, "Hit"); // log 작성	
            hit_count++;
		} else { 
            // miss
            make_directory_and_file(hashed_url);	
            write(client_sock, "MISS\n", 5);
            write_log_in_file(buf, hashed_url, "Miss"); // log 작성	
            miss_count++;
		}

 	}

	// 클라이언트 연결 종료 메시지 출력
    printf("[%s : %d] client was disconnected\n", ip,port); 	
   	exit(0); // 자식 프로세스 종료 (정상 종료)
}

/* return home directory path string */
char* getHomeDir(char* home)
{
	struct passwd* usr_info = getpwuid(getuid()); 
	strcpy(home, usr_info->pw_dir);
		
	return home;	
}

/* SHA1 */
char* sha1_hash(char* input_url, char* hashed_url)
{
	unsigned char hashed_160bits[20];  // 20byets(160bits)를 저장할 배열
	char hashed_hex[41];
	size_t  i;

    // 1. SHA1 digest 생성 (binary data)
	SHA1((unsigned char*)input_url,strlen(input_url),hashed_160bits); 

    // 2. binary data를 2자리 16진수 문자열로 인코딩
	for(i=0;i<sizeof(hashed_160bits);i++)
		sprintf(hashed_hex + i*2, "%02x", hashed_160bits[i]);  // hashed 

	strcpy(hashed_url,hashed_hex); // hashed_url 

	return hashed_url;
}


///////////////////////////////////////////////////////////////////////////////
// Function: make_directory
//
// Input:
//   - path : 생성할 전체 디렉토리 경로 (예: /home/user/cache/xx/yy)
//
// Output:
//   - 없음 (void)
//
// Purpose:
//   - 입력된 전체 경로를 기준으로 중간 디렉토리를 포함하여 모든 디렉토리를 생성
//   - 이미 존재하는 디렉토리는 무시하고, 존재하지 않는 디렉토리만 생성
///////////////////////////////////////////////////////////////////////////////
void make_directory(const char* path) 
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
// Function: make_directory_and_file
//
// Input:
//   - hashed_url : SHA1 해시된 URL 문자열 (총 40자리)
//
// Output:
//   - 없음 (void)
//
// Purpose:
//   - 해시된 URL을 기준으로 디렉토리 및 파일 경로를 구성
//   - ~/cache/abc/defg... 형태로 디렉토리를 생성하고, 해당 위치에 파일이 없으면 빈 파일 생성
///////////////////////////////////////////////////////////////////////////////
void make_directory_and_file(char* hashed_url) 
{
    char dir_path[100], file_path[200];
    char home[50];
    char* homedir = getHomeDir(home);

    // 1) 홈 디렉토리 주소 저장 
    char cache_root[50];
    snprintf(cache_root, sizeof(cache_root), "%s/cache", homedir);

    // 2) 해시화된  URL을 기반으로 디렉토리 생성
    snprintf(dir_path, sizeof(dir_path), "%s/%c%c%c", cache_root, hashed_url[0], hashed_url[1], hashed_url[2]);
    dir_path[sizeof(dir_path)-1] = '\0';

    // 3) 디렉토리가 존재하지 않으면 생성
    if (access(dir_path, F_OK) == -1) {
        make_directory(dir_path);
    }

    // 4) 파일 경로를 설정
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, &hashed_url[3]);
    file_path[sizeof(file_path)-1] = '\0'; 

    // 5) 파일이 존재하지 않으면 만들기
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
// Function: create_log_directory_with_file
//
// Input:
//   - 없음
//
// Output:
//   - 없음 (void)
//
// Purpose:
//   - 사용자의 홈 디렉토리 하위에 로그 디렉토리(`/logfile`)가 없으면 생성
//   - 로그 파일(`logfile.txt`)이 없으면 새로 생성
///////////////////////////////////////////////////////////////////////////////
void create_log_directory_with_file()
{
    char home[50];
    getHomeDir(home);

    char log_dir[100];
    char log_file[100];

    snprintf(log_dir, sizeof(log_dir), "%s%s", home,LOG_DIR); // log 디렉토리 주소
    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE); //log 파일 주소

    // log directory 생성
    if (access(log_dir, F_OK) == -1) {
        mkdir(log_dir, 0777);
    }

    // logfile.txt 생성
    if (access(log_file, F_OK) == -1) 
    {
        FILE* file = fopen(log_file, "w"); // 파일 생성
        fclose(file); // 파일 닫기
    }
}

///////////////////////////////////////////////////////////////////////////////
// Function: HIT_OR_MISS
//
// Input:
//   - hashed_url : SHA1 해시된 URL 문자열 (총 40자리)
//
// Output:
//   - int : 1이면 HIT (파일 존재), 0이면 MISS (파일 없음)
//
// Purpose:
//   - 해시된 URL이 ~/cache/xxx 디렉토리에 존재하는지 확인
//   - 디렉토리 내부에 해당 해시의 나머지 부분과 일치하는 파일명이 있으면 HIT
///////////////////////////////////////////////////////////////////////////////
int HIT_OR_MISS(char* hashed_url)
{
    char home[50];
    char cache_dir[100];
    
    snprintf(cache_dir, sizeof(cache_dir), "%s/cache/%c%c%c", getHomeDir(home), hashed_url[0], hashed_url[1], hashed_url[2]);

    DIR* dir = opendir(cache_dir); // opendir(hashed_url에 대한 directory명)

    // miss
    if (dir == NULL) return 0; 

    // Check hit
    struct dirent* dir_read;
    while ((dir_read = readdir(dir)) != NULL) {
        if (strcmp(dir_read->d_name, &hashed_url[3]) == 0){ 
            closedir(dir);
            return 1; // HIT
        }
    }

    // miss
    closedir(dir);
    return 0; 
}


///////////////////////////////////////////////////////////////////////////////
// Function: write_log_in_file
//
// Input:
//   - url         : 클라이언트가 요청한 원래 URL
//   - hashed_url  : SHA1으로 해시된 URL
//   - type        : "Hit" 또는 "Miss" 문자열
//
// Output:
//   - 없음 (로그 파일에 내용 기록)
//
// Purpose:
//   - 클라이언트 요청 처리 결과(Hit/Miss)를 로그 파일에 기록
//   - 처리 시간과 PID 정보를 포함한 로그 메시지 작성
///////////////////////////////////////////////////////////////////////////////
void write_log_in_file( const char* url, const char* hashed_url,const char* type)
{
    char home[50];
    char log_file[100];
    char time_buf[50];

    // get home directory and logfile.txt path
    getHomeDir(home); 
    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE); 

    // get time format
    time_t cur_time = time(NULL);  
    struct tm* t = localtime(&cur_time); 
    snprintf(time_buf, sizeof(time_buf), "%d/%d/%d, %02d:%02d:%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);


    FILE* file = fopen(log_file, "a"); 

    // hit
    if (strcmp(type, "Hit") == 0)
    {
        fprintf(file, "[Hit] ServerPID : %d | %c%c%c/%s - [%s]\n",getpid() ,hashed_url[0], hashed_url[1], hashed_url[2], &hashed_url[3], time_buf);
        fprintf(file, "[Hit]%s\n", url);
    }
    // miss
    else if (strcmp(type, "Miss") == 0)
    {
        fprintf(file, "[Miss] ServerPID : %d | %s - [%s]\n",getpid(), url, time_buf);
    }

    fclose(file); 
}


///////////////////////////////////////////////////////////////////////////////
// Function: write_termination
//
// Input:
//   - hit       : 처리한 HIT 요청 수
//   - miss      : 처리한 MISS 요청 수
//   - run_time  : 클라이언트와 연결된 시간 (초)
//
// Output:
//   - 없음 (종료 로그를 파일에 기록)
//
// Purpose:
//   - 클라이언트와의 연결이 종료될 때 로그 파일에 요약 정보 기록
//   - 서버 PID, 실행 시간, HIT/MISS 수를 남김
///////////////////////////////////////////////////////////////////////////////
void write_termination(int hit,int miss,double run_time)
{
    char home[50];
    char log_file[100];

    // get home directory and logfile.txt path
    getHomeDir(home); 
    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE); 

    FILE* file = fopen(log_file, "a");
    if (file != NULL){
        fprintf(file, "[Terminated] ServerPID : %d | run time: %.0f sec. #request hit : %d, miss : %d\n",getpid() ,run_time, hit, miss);
        fclose(file);
    }
}
