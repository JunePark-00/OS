#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include "shell_header.h"
#define BUFF 1024

void shell(){
    char str[BUFF];
    char *user = getenv("USER");
    char *path = getenv("PATH");
    char *home = getenv("HOME");
    //printf("HOME: %s\n",home);
    printf("echo$%s\n",path);
    pid_t pid;
    pid_t status;
    
    while(1){
        char* pwd; //pwd : 현재 어떤 디렉토리에 있는가?
        pwd = getcwd(NULL,BUFF);
        printf("(shell) %s $ ",pwd);

        fgets(str,BUFF,stdin); // 문자열 가져오기
        str[strlen(str)-1] = '\0'; // null 문자 추가

        strtok(str," ");
        char* arg1=strtok(NULL," "); // 입력 공백 단위로 끊기

        if(strcmp(str,"quit") == 0){
            break;
        }

        // path
        if(strcmp(str,"path") == 0){
            path = getenv("PATH");
            printf("echo$%s\n",path);
            continue;
        }

        // user
        if(strcmp(str,"user") == 0){
            printf("[User] %s\n",user);
            continue;
        }

        //pwd
        if(strcmp(str,"pwd") == 0){
            printf("[Directory] %s\n",pwd);
            continue;
        }

        //cd : 작업 디렉토리 위치 변경
        if(strcmp(str,"cd") == 0){
            if (arg1 == NULL || strcmp(arg1,"/") == 0){
                chdir(home);
            } else {
                chdir(arg1);
            }
            //printf("Directory is changed.\n");
            continue;
        }

        //mkdir : 디렉토리 생성
        if(strcmp(str,"mkdir") == 0){
            mkdir(arg1, 0755);
            //printf("%s is created.\n",arg1);
            continue;
        }

        //rmdir : 디렉토리 삭제
        if(strcmp(str,"rmdir") == 0){
            rmdir(arg1);
            //printf("%s is removed.\n",arg1);
            continue;
        }

        //time
        if(strcmp(str,"time") == 0){
            time_t get_time;
            time(&get_time); // 현재 시간
            struct tm* c_time; 
            c_time = localtime(&get_time);
            printf("[Time] %s",asctime(c_time));
            continue;
        }

        pid = fork();

	    if(pid == -1) {
            perror("fork error");
	        exit(1);
	    } else if(pid == 0) {// child
            execlp(str,str,arg1,(char*)0);
	        exit(0); 
	    } else {// parent
	        waitpid(pid,&status,0); // child가 죽을 때까지 기다림
		}

	}

	return;

}