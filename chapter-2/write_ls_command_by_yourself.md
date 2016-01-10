本章的最后呢，我们来尝试写一个ls系列的（参数： -a,-l,-al）命令。

我的代码：
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <errno.h>

#define    PARAM_NONE	0	//无参数
#define    PARAM_A      1	//-a：显示所有文件
#define    PARAM_L	2	//—l：一行只显示一个文件的详细信息
#define    MAXROWLEN	80	//一行显示的最多字符数

int 	   g_leave_len = MAXROWLEN;	//一行剩余长度，用于输出对齐
int 	   g_maxlen;			//存放某目录下最长文件名的长度


/*错误处理函数，打印出错误所在行数及其出错信息*/
void my_err(const char *err_string, int line)
{
	fprintf(stderr, "line: %d", line);
	perror(err_string);
	exit(1);
}

/*获取文件属性再打印*/
void display_attribute(struct stat buf, char *name)
{
	char		buf_time[32];		
	struct	passwd	*psd;		//从该结构体中获取文件所有者的用户名
	struct group	*grp;		//从该结构体中获取文件所有者所属组的组名
  
	/*获取并打印文件类型*/
	if(S_ISLNK(buf.st_mode)) {		//符号链接
		printf("l");
	} else if(S_ISREG(buf.st_mode)) {	//一般文件
		printf("-");
	} else if(S_ISDIR(buf.st_mode)) {	//目录文件
 		printf("d");
	} else if(S_ISCHR(buf.st_mode)) {	//字符设备文件
		printf("c");	
	} else if(S_ISBLK(buf.st_mode)) {	//块设备
		printf("b");
	} else if(S_ISFIFO(buf.st_mode)) {	//先进先出FIFO
		printf("f");
	}else if(S_ISSOCK(buf.st_mode)) {	//SOCK
		printf("s");
	}

	/*获取并打印文件所有者权限*/
	if(buf.st_mode & S_IRUSR) {
		printf("r");
	} else {
		printf("-");
	}
	if(buf.st_mode & S_IWUSR) {
		printf("w");
	} else {
		printf("-");
	}
	if(buf.st_mode & S_IXUSR) {
		printf("x");
	} else {
		printf("-");
	}


	/*获取并打印与文件所有者同组的用户组对该文件的操作权限*/

	if(buf.st_mode & S_IRGRP) {
		printf("r");
	} else {
		printf("-");
	}
	if(buf.st_mode & S_IWGRP) {
		printf("w");
	} else {
		printf("-");
	}
	if(buf.st_mode & S_IXGRP) {
		printf("x");
	} else {
		printf("-");
	}


	/*获取其他用户对该文件的操作权限*/

	if(buf.st_mode & S_IROTH) {
		printf("r");
	} else {
		printf("-");
	}
	if(buf.st_mode & S_IWOTH) {
		printf("w");
	} else {
		printf("-");
	}
	if(buf.st_mode & S_IXOTH) {
		printf("x");
	} else {
		printf("-");
	}

	printf("	");

	
	/*根据uid或者gid获取文件所有者的用户名与组名*/
	psd = getpwuid(buf.st_uid);   	//getpwuid是通过用户的 uid 查找用户的passwd数据，出错时返回NULL
	grp = getgrgid(buf.st_gid); 	//getgrgid（）用来依参数gid指定的组识别码逐一搜索组文件，找到时便将该组的数据以group结构返回。
	printf("%4d", (int)buf.st_nlink);	//打印文件的链接
	printf("%-8s", psd->pw_name);	//打印用户名 
	printf("%-8s",grp->gr_name);  	//打印组名

	printf("%6d", (int)buf.st_size);	//打印文件大小
	strcpy(buf_time, ctime(&buf.st_mtime));
	buf_time[strlen(buf_time) - 1] = '\0';      //去掉时间字符串的换行符
	printf("      %s",buf_time);		    //打印文件的时间信息
}


/*在没有使用-l选项时，打印一个文件名，打印时上下对齐*/
void display_single(char *name)
{
	int i, len;

	if(g_leave_len < g_maxlen) {		  	//g_leave_len b本行剩余长度   g_mxlen文件名中最长的文件名的长度
		printf("\n");
		g_leave_len = MAXROWLEN;
	}

	len = strlen(name);				//计算比最长的文件名短出来的长度
	len = g_maxlen - len;
	printf("%-s", name);

	for(i=0; i<len; i++) {				//将剩余位置输出空格
		printf(" ");
	}
	printf("  ");

	g_leave_len -= (g_maxlen + 2);
	
}

    	/*
	 *根据命令行参数和完整路径名显示目标文件
	 *参数flag: 命令行参数
	 *参数pathname: 包含了文件按名和路径名
	*/

void display(int flag, char *pathname)
{
	int		 i, j;
	struct stat 	 buf;
	char 		name[NAME_MAX + 1];

	/*从路径中解析出文件名*/
	for(i = 0, j = 0; i < strlen(pathname); i++) {
		if(pathname[i] == '/') {
			j=0;
			continue;
		}
		name[j++] = pathname[i];	
	}
	name[j] = '\0';


	/*用lstat而不是stat以方便解析链接文件*/
	if(lstat(pathname, &buf)  == -1) {		//将pathname指定的相关文件的信息放到buf里
		my_err("lstat", __LINE__);
	}

	switch(flag) {
		case PARAM_NONE: 	//没有-l和-a选项
			if(name[0] != '.') {
				display_single(name);
			}
			break;

		case PARAM_A:		//-a：显示包含隐藏文件在内的所有文件
			display_single(name);
			break;

		case PARAM_L:		//-l:每个文件单独占一行，显示文件的详细附属信息
			if(name[0] != '.') {
				display_attribute(buf, name);    //获取name指定的文件信息，保存到buf 中
				printf("%-s\n",name);
			}
			break;
		case PARAM_A + PARAM_L:   //-l ,-a同时存在
			display_attribute(buf,name);
			printf("%-s\n",name);
			break;

		default:
			break;
	}
}

void display_dir(int flag_param, char *path)
{
	DIR		*dir;
	struct dirent	*ptr;
	int 		count = 0;
	char  		filenames[256][PATH_MAX + 1], temp[PATH_MAX + 1];

	//获取该目录下文件总数和最长的文件名
puts(path);
	dir = opendir(path);     	//打开参数path指定的目录，返回DIR*形态的目录流
	if(dir == NULL) {
		my_err("opendir", __LINE__);
	}
	while((ptr = readdir(dir)) != NULL) {   		//找到文件名长度最大的，将其值赋给g_maxlen
		if(g_maxlen <strlen(ptr->d_name))
			g_maxlen = strlen(ptr->d_name);
		count++;	
	}
	closedir(dir);

	if(count > 256)                                      
	my_err("too many files under this dir", __LINE__);


	int i,j, len = strlen(path);
	//获取该目录下所有文件名
	dir = opendir(path);				//打开path指定的目录，返回DIR*形式的目录流
	for(i = 0; i < count; i++ ) {
		ptr = readdir(dir);			//从dir中读取目录项信息，返回一个struct dirent 指针
		if(ptr == NULL) {
			my_err("readdir", __LINE__);
		}
	strncpy(filenames[i], path, len);
	filenames[i][len] = '\0';
	strcat(filenames[i], ptr->d_name);
	filenames[i][len+strlen(ptr->d_name)] = '\0';
	}

//使用冒泡对文件名进行排序，排序后的存于filename中
	for(i = 0; i < count-1; i++)
		for(j = 0; j < count-1-i; j++) {
			if(strcmp(filenames[j],filenames[j+1]) > 0) {		//判断若前大于后，则交换两字符串
				strcpy(temp, filenames[j+1]);
				temp[strlen(filenames[j+1])] = '\0';
				strcpy(filenames[j+1], filenames[j]);
				filenames[j+1][strlen(filenames[j])]!= '\0';
				strcpy(filenames[j], temp);
				filenames[j][strlen(temp)] = '\0';
			}
		}
	for(i = 0; i < count; i++)
		display(flag_param,filenames[i]);    //调用上面一个函数
	closedir(dir);

	if( (flag_param & PARAM_L) == 0)
		printf("\n");
}


int main(int argc, char **argv)
{
	int		i,j,k,num;
	char		path[PATH_MAX + 1];
	char		param[32];				//保存命令行参数
	int		flag_param = PARAM_NONE;			//参数种类，即是否有-l和-a选项
	struct stat	buf;

	/*命令行参数的解析，分析-l, -a,-al,-la选项*/
	j = 0;
	num = 0;
	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			for(k = 1; k < strlen(argv[i]); k++, j++) {
				param[j] = argv[i][k];
			}
		num++;
		}
	}

	//只支持参数a和l,如果含有区其他选项就报错
	for(i=0; i<j; i++) {
		if(param[i] == 'a') {
			flag_param |= PARAM_A;
			continue;
		} else if(param[i] == 'l') {
			flag_param |= PARAM_L;
			continue;
		}else {
			printf("my_ls:invlid option -%c\n", param[i]);
			exit(1);
		}
	}
	param[j] = '\0';

	//如果没有输入文件名或目录 ，就显示当前目录
	if((num+1) == argc) {
		strcpy(path, "./");
		path[2]= '\0';
		display_dir(flag_param, path);
		return 0;
	}

	i=1;
	do{
	   // 如果不是目标文件名或目录，解析下一个命令行参数
	 	if(argv[i][0] == '-') {
			i++;
			continue;
		} else {
			strcpy(path, argv[i]);			//如果目标文件或者目录不存在，报错退出程序
		if(stat(path, &buf) == -1)
			my_err("stat", __LINE__);

		if(S_ISDIR(buf.st_mode) ) { 			//argv[i]是一个目录，如果目录的左最后一个字符不是‘/’，就加上
			if( (path[strlen(argv[i])-1])-1 != '/') {
				path[strlen(argv[i])] = '/';
				path[strlen(argv[i])+1] = '\0';
			}
		else 
			path[strlen(argv[i])] = '\0';
			
			display_dir(flag_param, path);
			i++;
		}
		else {
			display(flag_param, path);
			i++;
		}
	}
	}while(i<argc);
	
	return 0;
}
```