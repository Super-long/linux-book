/*************************************************************************
	> File Name: vfork_1.c
	> Author: 
	> Mail: 
	> Created Time: 2015年11月01日 星期日 12时03分49秒
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>



int globvar = 6;

int main(){

    int var;
    pid_t pid;
    var = 88;
    printf("before vfork \n ");
    if((pid = vfork()) < 0){
        printf("vfork error\n");
    }else if(pid == 0){
        globvar++;
        var++;
        _exit(0);
    }
    
    printf("pid = %d,glob = %d,var = %d\n",getpid(),globvar,var);
    
    
    
}
