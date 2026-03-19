/*
*  File Name : proxy_cache.c
*  OS		 : Ubuntu 20.04 64bits
*  Author	 : OH Nagyun
*  Title	 : System Programming Assignment #1-3 (proxy server)
*/

#include <stdio.h>  
#include <string.h> 
#include <openssl/sha.h> 
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>

#define SHA1_RAW_SIZE   20      // SHA1 결과 바이너리 크기 (160 bits)
#define SHA1_HEX_SIZE   40      // 16진수 변환 후 문자열 길이 (20 * 2)
#define LOG_DIR "/logfile"
#define LOG_FILE "/logfile/logfile.txt"


/* return home directory path string */
char* getHomeDir(char* home) {

    struct passwd* usr_info = getpwuid(getuid());
    strcpy(home, usr_info->pw_dir);

    return home;
}

/* SHA1 */
char* sha1_hash(char* input_url, char* hashed_url) {

    unsigned char hashed_160bits[20];  // 20byets(160bits)를 저장할 배열  
    char hashed_hex[41];
    size_t  i;

    // 1. SHA1 digest 생성 (binary data)
    SHA1((unsigned char*)input_url, strlen(input_url), hashed_160bits);

    // 2. binary data를 2자리 16진수 문자열로 인코딩
    for (i = 0; i < sizeof(hashed_160bits); i++) {
        // hashed_url의 각 인덱스에 2자리씩 기록 (ex: 0a,1f...)
        sprintf(hashed_hex + i * 2, "%02x", hashed_160bits[i]);
    }

    strcpy(hashed_url, hashed_hex);

    return hashed_url;
}

/* mkdir() 함수를 사용하여 0777 권한의 실제 경로를 생성하는 함수 */
void make_directory(const char* path) {

    char temp[100];
    char* p = NULL;
    int len;

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);

    // 경로 마지막에 '/'가 있으면 제거
    if (temp[len - 1] == '/') {
        temp[len - 1] = '\0';
    }

    for (p = temp + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(temp, 0777);  // 중간 directory 
            *p = '/';
        }
    }

    mkdir(temp, 0777);  // 최종 directory 
}

/*  make hashed directory and file */
void make_directory_and_file(char* hashed_url) {

    char dir_path[100], file_path[200];
    char home[50];
    char* homedir = getHomeDir(home);

    // 1) 홈 디렉토리 주소 저장 
    char cache_root[1024];
    snprintf(cache_root, sizeof(cache_root), "%s/cache", homedir);

    // 2) 해시화된  URL을 기반으로 디렉토리 생성
    snprintf(dir_path, sizeof(dir_path), "%s/%c%c%c", cache_root, hashed_url[0], hashed_url[1], hashed_url[2]);
    dir_path[sizeof(dir_path) - 1] = '\0';


    // 3) 디렉토리가 존재하지 않으면 생성
    if (access(dir_path, F_OK) == -1) {
        make_directory(dir_path);
    }

    // 4) 파일 경로를 설정
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, &hashed_url[3]);
    file_path[sizeof(file_path) - 1] = '\0';


    // 5) 파일이 존재하지 않으면 만들기
    if (access(file_path, F_OK) == -1) {

        FILE* file = fopen(file_path, "w");
        if (file != NULL) {
            fclose(file);
        }
    }
}

/*  create log directory and logfile.txt   */
void create_log_directory_with_file() {

    char home[50];
    getHomeDir(home);

    char log_dir[100];
    char log_file[100];

    snprintf(log_dir, sizeof(log_dir), "%s%s", home, LOG_DIR); // log directory
    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE); // log/logfile.txt

    // log directory 생성
    if (access(log_dir, F_OK) == -1) {
        mkdir(log_dir, 0777);
    }

    // logfile.txt 생성
    if (access(log_file, F_OK) == -1) {
        FILE* file = fopen(log_file, "w");
        fclose(file);
    }
}

/*  Check hit or miss */
int HIT_OR_MISS(char* hashed_url) {

    char home[50];
    char cache_dir[100];
    getHomeDir(home);

    snprintf(cache_dir, sizeof(cache_dir), "%s/cache/%c%c%c", home, hashed_url[0], hashed_url[1], hashed_url[2]);

    DIR* dir = opendir(cache_dir);

    // miss
    if (dir == NULL) return 0;

    // Check whether hit
    struct dirent* dir_read;
    while ((dir_read = readdir(dir)) != NULL) {
        // hit
        if (strcmp(dir_read->d_name, &hashed_url[3]) == 0) {
            closedir(dir);
            return 1;
        }
    }

    // miss
    closedir(dir);
    return 0;
}

/* write log file */
void write_log_in_file(const char* url, const char* hashed_url, const char* type) {

    char home[50];
    char log_file[100];
    char time_buf[50];

    // get home directory and logfile.txt path
    getHomeDir(home);
    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE);

    // get time format
    time_t cur_time = time(NULL);
    struct tm* t = localtime(&cur_time);
    snprintf(time_buf, sizeof(time_buf), "%d/%d/%d, %02d:%02d:%02d",
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

    FILE* file = fopen(log_file, "a");

    // hit
    if (strcmp(type, "Hit") == 0 && hashed_url != NULL) {
        fprintf(file, "[Hit]%c%c%c/%s-[%s]\n", hashed_url[0], hashed_url[1], hashed_url[2], &hashed_url[3], time_buf);
        fprintf(file, "[Hit]%s\n", url);
    }

    // miss
    else if (strcmp(type, "Miss") == 0) {
        fprintf(file, "[Miss]%s-[%s]\n", url, time_buf);
    }

    fclose(file);
}

/* write termination message in logfile.txt */
void write_termination(int hit, int miss, double run_time) {

    char home[50];
    char log_file[100];

    // get home directory and logfile.txt path
    getHomeDir(home);
    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE);

    FILE* file = fopen(log_file, "a");
    if (file != NULL) {
        fprintf(file, "[Terminated] run time: %.0f sec. #request hit : %d, miss : %d\n", run_time, hit, miss);
        fclose(file);
    }

}

void run_sub() {

    int hit = 0, miss = 0;
    char url[1024]; char hashed_url[41];
    time_t start_time = time(NULL); // 시작 시각

    create_log_directory_with_file();

    while (1) {

        // input url
        printf("[%d]input url>", getpid());
        fgets(url, sizeof(url), stdin);
        url[strcspn(url, "\n")] = 0;

        // break 
        if (strcmp(url, "bye") == 0)break;

        // url -> sha1 -> hashed_url
        sha1_hash(url, hashed_url);

        // check hit or miss
        int result = HIT_OR_MISS(hashed_url);
        if (result) {
            hit++;
            write_log_in_file(url, hashed_url, "Hit");
        }
        else {
            miss++;
            write_log_in_file(url, NULL, "Miss");
            make_directory_and_file(hashed_url);  // miss인거 기록했으니, 파일 생성
        }
    }

    time_t end_time = time(NULL); // 종료 시각

    double termination_time = difftime(end_time, start_time);
    write_termination(hit, miss, termination_time);

}

void write_termination_main(double run_time, int sub_process_count) {

    char home[50];
    char log_file[100];

    getHomeDir(home);
    snprintf(log_file, sizeof(log_file), "%s%s", home, LOG_FILE);

    FILE* file = fopen(log_file, "a");

    fprintf(file, " **SERVER** [Terminated] run time: %.0f sec. #sub process: %d\n", run_time, sub_process_count);
}


int main(){

    /* 초기 세팅 */
	umask(0);
	time_t start_time = time(NULL);  
	int sub_process_count =0; // 자식 프로세서 수
	pid_t pid;
	char input_cmd[20];

    while (1) {

        printf("[%d]input CMD> ", getpid());
        fgets(input_cmd, sizeof(input_cmd), stdin);
        input_cmd[strcspn(input_cmd, "\n")] = 0;

        /* connect */
        if (strcmp(input_cmd, "connect") == 0) {

            pid = fork();

            if (pid == 0) {
                run_sub();  // Proxy 1-2 기능 수행	
                exit(0); // 자식 프로세서 종료
            }
            else {
                sub_process_count++;
                wait(NULL); //자식 프로세서 종료까지 기다림
            }
        }
        /* quit */
        else if (strcmp(input_cmd, "quit") == 0) {
            break;
        }
    }

	time_t end_time =time(NULL); 
    double termination_time = difftime(end_time,start_time); 

	write_termination_main(termination_time,sub_process_count); 
	
    return 0;	
}



