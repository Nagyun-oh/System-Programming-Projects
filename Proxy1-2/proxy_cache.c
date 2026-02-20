#include <stdio.h>  // sprintf()
#include <string.h> // strcpy()
#include <openssl/sha.h> //SHA1()
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#define LOG_DIR "/logfile"
#define LOG_FILE "/logfile/logfile.txt"

// File Name	: proxy_cache.c
// Date		: 2024/04/10
// Os		: Ubuntu 20.04 64bits
// Author	: OH Nagyun
// Student ID	: 2021202089
// -----------------------------------
// Title	: System Programming Assignment #1-2 (proxy server)

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

// mkdir() 함수를 사용하여 0777 권한의  실제 경로를 생성하는 함수
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



// directory 와  파일 생성 함수
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


// 로그 디렉토리/파일 생성
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


// url 이 HIT 인지 MISS인지 판별하는 함수
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

// 로그 작성 함수
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
        fprintf(file, "[Hit]%c%c%c/%s-[%s]\n", hashed_url[0], hashed_url[1], hashed_url[2], &hashed_url[3], time_buf);
        fprintf(file, "[Hit]%s\n", url);
    }
    // MISS
    else if (strcmp(type, "Miss") == 0)
    {
        fprintf(file, "[Miss]%s-[%s]\n", url, time_buf);
    }

    fclose(file); // 파일 닫기
}

// 종료 로그 작성
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
        fprintf(file, "[Terminated] run time: %.0f sec. #request hit : %d, miss : %d\n", run_time, hit, miss);
        fclose(file);
    }

}

	
int main()
{
	char url[20];   // 입력받는 url
	char hashed_url[41]; //hashed_url
	umask(0);

	int hit =0; //hit count
	int miss =0; // miss count
	time_t start_time = time(NULL); // 시작 시간

	Create_log_directory_with_file(); // 함수호출 =>  /log/logfile 생성

	while(1)
	{
		printf("input url>");
		fgets(url,sizeof(url),stdin); // url 입력
		url[strcspn(url,"\n")]=0; // 개행문자 처리

		if(strcmp(url,"bye")==0)break; // bye 면 반복문 탈출해서 프로그램 종료하게끔 함

		sha1_hash(url, hashed_url); // 입력받은 url에 대해서 함수 호출해서 해쉬화

		int result = HIT_OR_MISS(hashed_url);

		 if (result)
		 {
                      hit++;
           	      Write_log_in_file(url,hashed_url,"Hit");
                 }
		 else
		 {
                      miss++; 
                      Write_log_in_file(url,NULL,"Miss"); // miss 이면 로그파일에 miss인거 기록
                      Make_directory_file(hashed_url);  // miss인거 기록했으니, 파일 생성
       		 }

	}

	// Terminated time 작성
	time_t end_time =time(NULL); // 종료시간 얻기
	double termination_time = difftime(end_time,start_time);
	Write_termination(hit,miss,termination_time);

	return 0;
}
