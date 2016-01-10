/*************************************************************************
	> File Name: fork_id.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月18日 星期日 16时04分33秒
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<stdlib.h>

int main(){
    
    pid_t pid;
    if((pid = fork()) < 0){
        printf("create error \n");
        exit(0);
    }else if(pid == 0){
        printf("i am son_process ID is:%d\n",getpid());
    }else{
        printf("i am father_process ID is:%d\n",getpid());
    }

    return 0;
}
