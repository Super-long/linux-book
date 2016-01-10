/*************************************************************************
	> File Name: fork_text.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月18日 星期日 16时19分36秒
 ************************************************************************/

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

int main(){

    pid_t pid;
    
    printf("I am head of the text!\n");

    if((pid = fork()) < 0){

        printf("crate error\n");
        exit(0);

    }else if(pid == 0){
        
        printf("I am the son_process space\n");
        printf("The son_process is end \n\n");

    }else{
        
        printf("I am the parent_process space\n");
        printf("The parent_process is end\n\n");

    }

    printf("I am the end of the text\n\n\n");

    return 0;

}
