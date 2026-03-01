#include <stdio.h>  // sprintf()
#include <string.h> // strcpy()
#include <openssl/sha.h> //SHA1()
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

/*
*  File Name : proxy_cache.c
*  OS		 : Ubuntu 20.04 64bits
*  Author	 : OH Nagyun
*  Title	 : System Programming Assignment #1-1 (proxy server)
*/

#define SHA1_RAW_SIZE   20      // SHA1 결과 바이너리 크기 (160 bits)
#define SHA1_HEX_SIZE   40      // 16진수 변환 후 문자열 길이 (20 * 2)

/* return home directory path string */
char* getHomeDir(char* home){

	struct passwd* usr_info = getpwuid(getuid()); 
	strcpy(home, usr_info->pw_dir);
		
	return home;	
}

/* SHA1 */
char* sha1_hash(char* input_url, char* hashed_url){

	unsigned char hashed_160bits[SHA1_RAW_SIZE];  // 20바이트를 저장할 배열
	char hashed_hex[41];
	size_t  i;

    // 1. SHA1 digest 생성 (binary data)
    SHA1((const unsigned char*)input_url, strlen(input_url), hashed_160bits));

    // 2.binary data를 2자리 16진수 문자열로 인코딩
    for (i = 0; i < sizeof(hashed_160bits); i++) {
        // hashed_url의 각 인덱스에 2자리씩 기록 (ex: 0a,1f...)
        sprintf(hashed_hex + i * 2, "%02x", hashed_160bits[i]);  
    }
	strcpy(hashed_url,hashed_hex);

	return hashed_url;
}

/* mkdir() 함수를 사용하여 0777 권한의 실제 경로를 생성하는 함수 */
void make_directory(const char* path) {

    char temp[100];
    char* p = NULL;
    int len;

    umask(0);

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    
    // 경로 마지막에 '/'가 있으면 제거
    if (temp[len - 1] == '/') {
        temp[len - 1] = '\0';
    }

    for (p = temp + 1; *p!='\0'; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(temp, 0777);  // 중간 directory 
            *p = '/';
        }
    }
   
    mkdir(temp, 0777);  // 최종 directory 
}

/* directory 와 file 생성 함수*/
void make_directory_and_file(char* hashed_url) {
    char dir_path[100], file_path[200];
    char home[256];
    char* homedir = getHomeDir(home);

    // 1) 홈 디렉토리 주소 저장
    char cache_root[50];
    snprintf(cache_root, sizeof(cache_root), "%s/cache", homedir);

    // 2) 해시화된 URL을 기반으로 디렉토리 생성
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
    if (access(file_path, F_OK) == -1) {
        FILE* file = fopen(file_path, "w");
        if (file != NULL){
            fclose(file);   
    	}
    }

}

int main(){

	char url[1024];
	char hashed_url[41];
	umask(0);

	while(1){
		printf("input url>");
		fgets(url,sizeof(url),stdin);
		url[strcspn(url,"\n")]=0;

		if(strcmp(url,"bye")==0)break;

        /* url -> hashed_url*/
		sha1_hash(url, hashed_url);     

        /* 해쉬화된 디렉토리에 대해서, directory 및 file 생성*/
        make_directory_and_file(hashed_url);
	}

}
