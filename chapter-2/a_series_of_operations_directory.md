前面的几个小结介绍了文件的各种操作，本小结主要介绍一下关于目录的操作。具体的方面跟文件类似，有创建、打开、关闭、遍历等等。

##系统调用——mkdir
我们可以在自己的程序中使用mkdir系统调用来创建一个目录。

```
#include<sys/stat.h>

int mkdir(const char *path, mode_t mode);

成功时候返回0和新创建的目录，失败返回-1，并设置errno位。
```
第一个参数要创建的目录的名字，第二个参数是新目录的权限位。


##系统调用——getcwd/getwd/get_current_dir_name
我们可以在当前的程序中，用getcwd/getwd/get_current_dir_name系统调用来获知当前的工作目录。

```
#include <unistd.h>

char *getcwd(char *buf, size_t size);

char *getwd(char *buf);

char *get_current_dir_name(void);

执行成功则将结果复制到参数buf 所指的内存空间, 或是返回自动配置的字符串指针. 失败返回NULL,错误代码存于errno.

```
buf指的是将当前工作目录存放的地址空间，size指的是buf的空间大小。

注：
1、在调用此函数时，buf 所指的内存空间要足够大。若工作目录绝对路径的字符串长度超过参数size 大小，则返回NULL，errno 的值则为ERANGE。
2、倘若参数buf 为NULL，getcwd()会依参数size 的大小自动配置内存(使用malloc())，如果参数size 也为0，则getcwd()会依工作目录绝对路径的字符串程度来决定所配置的内存大小，进程可以在使用完次字符串后利用free()来释放此空间。


##系统调用——chdir/fchdir
我们在自己的程序中，可以使用chdir/fchdir来更改当前的工作目录。

```
#include <unistd.h>

int chdir(const char *path);
int fchdir(int fd);

执行成功则返回 0, 失败返回-1, errno 为错误代码.
```

两个系统调用的区别在于参数，path指的是路径，fd为文件描述词。

举个很简单的例子：

```
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc , char * argv[]) {
    int fd;
    fd = open("/tmp", O_RDONLY);
    fchdir(fd);
    printf("current working directory : %s \n", getcwd(NULL, NULL));
    close(fd);
    
    return EXIT_SUCCESS;
}
```


##系统调用——opendir
通过使用opendir系统调用，我们可以打开一个目录。

```
#include<sys/types.h>
#include<dirent.h>

 DIR *opendir(const char *name);
 DIR *fdopendir(int fd);

成功则返回DIR* 型态的目录流，打开失败则返回NULL。

```
opendr系统调用的参数是文件的名字，可以是绝对路径。fdopendir系统调用的参数是文件描述符。

##系统调用——readdir
我们将目录打开之后就要对它进行使用，我们可以读出目录下的所有文件的信息。

```
#include<dirent.h>

struct dirent *readdir(DIR *dirp);

成功则返回下个目录进入点。有错误发生或读取到目录文件尾则返回NULL.
```
参数就是我们使用opendir系统调用的返回值（目录流）。

我们要重点说一下dirent这个结构体：

```
struct dirent {
   ino_t d_ino;    /* 索引节点号 */
   off_t d_off;       /* 在目录文件中的偏移 */
   unsigned short d_reclen;    /* 文件名长度 */
   unsigned char  d_type;      /* 文件类型 */
   char d_name[256]; /* 文件名（最长255个字节） */
};

```
我们根据dirent结构体不难看出来，其实每个文件保存的形式都是这样保存的。我们可以使用这个结构体来遍历一个目录吧。举个例子：

```
#include<sys/types.h>
#include<dirent.h>
#include<unistd.h>

int main(int argc , char * argv[]) {
{
    DIR * dir;
    struct dirent * ptr;
    int i;
    dir =opendir(“/etc/rc.d”);
    while((ptr = readdir(dir))!=NULL) {
        printf(“d_name: %s\n”,ptr->d_name);
    }
    closedir(dir);
    
    return EXIT_SUCCESS;
}
```


##系统调用——closedir
当我们使用完打开的目录之后，我们就应该像文件一样将它关闭。

```
#include <sys/types.h>
#include <dirent.h>

int closedir(DIR *dirp);

成功返回0，失败返回-1，并设置errno位。

```
例子同上～～
