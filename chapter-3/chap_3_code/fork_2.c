/*************************************************************************
	> File Name: fork_1.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月18日 星期日 15时47分10秒
 ************************************************************************/

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

int glob_var = 10;

int main(){
    
    pid_t chid;
    
    int parent_var = 1;

    if((chid = fork()) < 0){

        printf("create error\n");
        exit(0);

    }else if(chid == 0){

        int chid_var = 1;
        parent_var += 100;
        printf("------chid space-----\n");
        printf("the glob_var = %d \nthe parent_var = %d\nthe chid_var = %d\n",glob_var,parent_var,chid_var);
        printf("------chid end ------\n\n");

    }else{
        sleep(2);
        printf("-----parent soace----\n");
        printf("the glob_var = %d\nthe parent_var = %d\n",glob_var,parent_var);
        printf("-----parent end-----\n\n");

    }

    return 0;

}
