#include <stdio.h>  // sprintf()
#include <string.h> // strcpy()
#include <openssl/sha.h> //SHA1()
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

// File Name	: proxy_cache.c
// Date		: 2024/04/03
// Os		: Ubuntu 20.04 64bits
// Author	: OH Nagyun
// Student ID	: 2021202089
// -----------------------------------
// Title	: System Programming Assignment #1-1 (proxy server)

// return  Home directory string
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

	
int main()
{
	char url[20];
	char hashed_url[41];
	umask(0);

	while(1)
	{
		printf("input url>");
		fgets(url,sizeof(url),stdin);
		url[strcspn(url,"\n")]=0;

		if(strcmp(url,"bye")==0)break;

		sha1_hash(url, hashed_url); // 입력받은 url에 대해서 함수 호출해서 해쉬화

		Make_directory_file(hashed_url); //함수 호출하여  해쉬화된 디렉토리에 대한 디렉토리 및 파일 생성
	}


}
