#更多socket相关函数：
##概述：
	除了上面描述过的一些基本socket API，Linux还提供给我们许多高级I/O函数，或许它们不是那么的常用，但在需要时它们会显示出卓越的性能。
##收发数据
###sendv()和writev()
```
#include <sys/uio.h>

ssize_t readv(int fd, const struct iovec* vector, int count);
ssize_t writev(int fd, const struct iovec* vector, int count);
```
返回值：
如果所有的内存块被读取或写入成功的话，该函数返回传输的所有字节。失败时返回 `-1` 并设置 `errno` 错误码。

参数：
`fd` 为被操作的文件描述符，`vector` 是 `iovec` 类型的数组，`count` 是 `vector` 数组的大小。
`iovec` 定义如下：
```
struct iovec{
	void *iov_base;        /* 内存起始地址 */
	size_t iov_len;        /* 内存的长度   */
}; 
```

描述：
`readv` 函数将数据从文件描述符读入到分散的内存块中。
`write` 函数将多块分散的内存块中的数据写入到文件描述符中。

来看个例子：

客户端
```
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>

#define MAXDATASIZE 100

int main(int argc, char *argv[])
{
    int sockfd;                 /* 套接字 */
    int num;                    /* 接受发送数据的大小 */
    char buf[MAXDATASIZE];      /* 缓冲区 */
    struct sockaddr_in server;  /* 存储ip和端口的结构体 */
    
    char* ip = argv[1];
    int port = atoi(argv[2]);

    if (argc != 3)
    {
        perror("argument error, please input ip and port\n");
        exit(1);
    }
        
    if((sockfd=socket(AF_INET,SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error\n");
        exit(1);
    }
    
    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);
    
    if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("connect() error\n");
        exit(1);
    }
    
    /* 我们定义了 iovec 结构体数组，将2份缓冲区数据放入其中
	 * 接着调用 writev 发送
	 */
	/* 注意 c99 标准后允许中途定义变量，在编译选项后加 -std=c99 即可*/
    char* buffer1 = "hello ";
    char* buffer2 = "world\n";

    struct iovec vector[2];
    vector[0].iov_len = strlen(buffer1);
    vector[1].iov_len = strlen(buffer2);
    vector[0].iov_base = buffer1;
    vector[1].iov_base = buffer2;

    if((num = writev(sockfd, vector, 2)) == -1)
    {
        perror("send() error\n");
        exit(1);
    }else{
        printf("sendv %d byte\n", num);
    }
	
	
    if((num = recv(sockfd, buf, MAXDATASIZE, 0)) == -1)
    {
        perror("recv() error\n");
        exit(1);
    }
    buf[num-1]='\0';
    printf("recv message: %s\n",buf);
    
    close(sockfd);

    return 0;
}
```

为了解决篇幅，服务端我们依旧使用第一小节的例子。
读者可以自己尝试用 `readv` 来实现服务端，`readv` 是将读入的数据写到多个缓冲区中，它总是写满一块才开始写下一块。

客户端:

**sendv 12 byte**

**recv message: hello world**

服务端:

**hello world**

这时，你可能会问，`writev` 和 `sendv` 与调用两次 `write` 或 `send` 有什么区别，`writev` 和 `sendv` 只需要一次就可以在多个缓冲区之间传送数据，避免了多次系统调用和缓冲区的拷贝。 
还有重要的一点，`writev` 能够避免 `Nagle` 和延迟 `Ack` 的影响，这个在我们后面的 `Tcp` 协议探究会说到。 

###sendmsg()和recvmsg()
```
#include <sys/types.h>
#include <sys/socket.h>
       
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
```
返回值：
成功返回发送或读取字节的数量。失败则返回 `-1` 并设置 `errno` 错误码。

参数：
`sockfd` 为已连接的套接字
`msg` 为 `const struct msghdr` 类型的结构体
结构体定义如下：
```
struct msghdr {
	void          *msg_name;       /* 可选择的地址，适用于无连接状态如 Udp */
    socklen_t     msg_namelen;     /* 地址长度 */
    struct iovec  *msg_iov;        /* 输入输出缓冲区 */
    size_t        msg_iovlen;      /* 输入输出缓冲区个数 */
    void          *msg_control;    /* 辅助数据 */
    size_t        msg_controllen;  /* 辅助数据大小 */
    int           msg_flags;       /* 标志变量 */
};
```
 
描述：
`recvmsg` 和 `sendmsg` 是最通用的 `I/O` 函数。就拿 `recvmsg` 来说，我们基本上可以把所有的 `read`、 `readv`、`recvfrom` 调用换成 `recvmsg`。
`recvmsg` 和 `sendmsg` 支持一般数据和多缓冲区数据的发送和接收，并且可以携带辅助数据（`msg_control`）。 

##零拷贝
###sendfile()
```
#include <sys/sendfile.h>

ssize_t  sendfile(int  out_fd, int in_fd, off_t *offset, size_t count);
```
返回值：
如果成功，返回写入 `out_fd` 的字节数。发送失败则返回 `-1` 并设置 `errno` 错误码。

参数：
`out_fd` 是写文件描述符。
`in_fd` 是读文件描述符。
`offset` 是文件偏移量，如果不为 `NULL` ，将从文件的 `offset` 处开始发送，若为 `NULL` ，则从文件的起始开始发送。
`count` 是发送的字节数。

描述：
`sendfile` 是专门用来发送大文件的 `socket API`，它属于零拷贝中的一种。平时我们发送文件时若调用 `write` 或 `send` 等 `API`，它们首先会将文件中的数据读入到用户态缓冲区中，再将用户态缓冲区数据拷贝到内核态，然后发送给对方。对方调用 `read` 或 `recv` 等 `API`，接受数据到用户态缓冲区后再写入文件中，这样白白进行了多次不必要的拷贝，效率较低。
`Linux` 内核在 `2.6.33` 版本后引入了 `sendfile` 零拷贝这个 `API`，它发送大文件时不会先将数据读入到用户态缓冲区后再拷贝到内核态缓冲区发送，而是直接将文件中的数据读入到内核态缓冲区中然后发送，这样就减少了拷贝，大大提高了效率。

`sendfile` 基本使用

客户端
```
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("argument error\n");
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int fd;
    int in_fd;
    int num;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() error\n");
        exit(1);
    }

    if(connect(fd, (struct sockaddr*)&server, sizeof(server)) == -1)
    {
        perror("connect() error\n");
        exit(1);
    }

    char *name = argv[3];
    struct stat st;
    if((in_fd = open(name, O_RDONLY)) == -1)
    {
        perror("open() error\n");
        exit(1);
    }

    long int size = stat(name, &st);
    if(size < 0)
    {
        perror("stat() error\n");
        exit(1);
    }

    off_t pos = lseek(in_fd, 0, SEEK_SET);
    printf("file size:%ld\n", st.st_size);
    if(pos < 0)
    {
        printf("lseek() error\n");
        exit(1);
    }

    if((num = sendfile(fd, in_fd, &pos, st.st_size)) == -1)
    {
        perror("send file error\n");
        exit(1);
    }else{
        printf("sendfile success,size:%d\n", num);
    }

    return EXIT_SUCCESS;
}
```

服务端
```
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("argument error\n");
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int out_fd;
    int num;
    int fd, sockfd;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() error\n");
        exit(1);
    }
    if(bind(fd, (struct sockaddr*)&server, sizeof(server)) == -1)
    {
        perror("bind() error\n");
        exit(1);
    }
    if(listen(fd, 64) == -1)
    {
        perror("listen() error\n");
        exit(1);
    }
    
    socklen_t len = sizeof(server);
    if((sockfd = accept(fd, (struct sockaddr*)&server, &len)) == -1)
    {
        perror("accept() error\n");
        exit(1);
    }else{
        printf("connect success\n");
    }

    char *name = "newfile";
    char buffer[2048];
    
    if((out_fd = open(name, O_WRONLY | O_CREAT, 777)) == -1)
    {
        perror("open() error\n");
        exit(1);
    }

    if(lseek(out_fd, 0, SEEK_CUR) < 0)
    {
        perror("lseek() error\n");
        exit(1);
    }

    while(1){
        bzero(buffer, 2048);
        if((num = recv(sockfd, buffer, 2048, 0)) < 0)
        {
            printf("recv() error\n");
            exit(1);
        }else if(num == 0){
            break;
        }
        if(write(out_fd, buffer, num) < 0)
        {
            printf("write() error\n");
            exit(1);
        }
    }
    printf("recv file success!!!\n");


    return EXIT_SUCCESS;
}
```

我们尝试运行下，发送客户端代码：
```
$ gcc server.c -o server
$ gcc client.c -o client

$ ./server 127.0.0.1 10000
$ ./client 127.0.0.1 10000 client.c

$ ./client 127.0.0.1 10000 client.c
file size:1482
sendfile success,size:1482

$ ./server 127.0.0.1 10000
connect success
recv file success!!!
```

###splice()

```
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <fcntl.h>

ssize_t splice(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags);
```

返回值：
若成功返回流入到 `fd_out` 中的字节数，若返回 `0` 意味着没有数据可以传输。出错返回 `-1`，并设置 `errno` 错误码。

参数：
`fd_in` 是输入数据的文件描述符。如果 `fd_in` 是管道文件描述符，`off_in` 必须被设置为 `NULL`。如果 `fd_in` 不是管道文件描述符，`off_in` 表示偏移量。`fd_out` 与 `fd_in` 类似，表示的是输出流。
`len` 表示数据的长度。
`flag` 表示控制数据如何移动。

`flag`：


| 标识值 | 表示含义 |
| :-: | :-: |
|SPLICE_F_MOVE |提示内核如果可以以页为单位移动数据 |
|SPLICE_F_NONBLOCK | 非阻塞的 splice |
|SPLICE_F_MORE | 告诉内核后面还有更多的数据 |

描述：
`splice` 用于在两个文件描述符之间零拷贝移动数据，`fd_in` 和 `fd_out` 必须至少有一个是管道文件描述符。

来看个例子：

我们使用 `sendfile` 例子的客户端。

服务端代码如下：
```
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("argument error\n");
        exit(1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int out_fd;
    int num;
    int fd, sockfd;

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() error\n");
        exit(1);
    }
    if(bind(fd, (struct sockaddr*)&server, sizeof(server)) == -1)
    {
        perror("bind() error\n");
        exit(1);
    }
    if(listen(fd, 64) == -1)
    {
        perror("listen() error\n");
        exit(1);
    }
    
    socklen_t len = sizeof(server);
    if((sockfd = accept(fd, (struct sockaddr*)&server, &len)) == -1)
    {
        perror("accept() error\n");
        exit(1);
    }else{
        printf("connect success\n");
    }

    const char *name = "newfile";
    char buffer[2048];
    
    if((out_fd = open(name, O_WRONLY | O_CREAT, 777)) == -1)
    {
        perror("open() error\n");
        exit(1);
    }

    if(lseek(out_fd, 0, SEEK_CUR) < 0)
    {
        perror("lseek() error\n");
        exit(1);
    }

    int pipefd[2];
    if(pipe(pipefd) < 0){
        perror("pipe() error\n");
        exit(1);
    }
    int ret = 0;
    /* 使用 splice */
    while(1){
        if((ret = splice(sockfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE)) == -1){
            perror("splice() error\n");
            exit(1);
        }else if(ret == 0){
            break;
        }
        if((ret = splice(pipefd[0], NULL, out_fd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE)) == -1){
            perror("splice() error\n");
            exit(1);
        }
              
    }
    printf("recv file success!!!\n");


    return EXIT_SUCCESS;
}
```

读者可以尝试的运行下。
感兴趣可以和 `send` 和 `recv` 版本对比下性能。

##设置属性
###fcntl()
```
#include <unistd.h>
#include <fcntl.h>

int fcntl(int fd, int cmd, ... /* arg */ );
```
`fcntl` 函数提供对文件描述符各种各样的操作，操作不同返回值不同。出错返回 `-1` 并且设置 `errno` 错误码。

这里列举一些常用的。

|操作|含义|返回值|
|:-|:-|:-|
|F_DUPFD | 创建一个新的文件描述符 | 新创建的文件描述符号 | 
|F_GETFD | 获得 fd 的标志 | fd 的标志 |
|F_SETFD | 设置 fd 的标志 | 0 |
|F_GETFL | 获取 fd 的标志状态位 | fd 的标志状态 |
|F_SETFD | 设置 fd 的标志状态位 | 0 |

需要强调的是 `F_GETFL` 和 `F_SETFL`，它们可以设置的标志位有 `APPEND`、`O_CREATE`、`O_RDONLY`、`O_NONBLOCK`等等

看个例子，将文件描述符设置为非阻塞：
```
int setnonblocking(int fd){
	int old_flag = fcntl(fd, F_GETFL);
	int new_flag = old_flag | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flag);
	return old_flag;
}
```

###setsockopt()
```
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
```
返回值：
成功返回 `0`，失败返回 `-1`，并设置 `errno` 错误码。

参数：
`sockfd` socket 文件描述符。
`level` 选项定义的层次。
`optname` 需要设置的选项
`optval` 存放选项值的缓冲区
`optlen` 缓冲区的长度

返回值：
成功返回 `0`，失败返回 `-1`，并且设置 `errno` 错误码。

描述：
`setsockopt` 是为任意类型，任意状态的套接字设置选项。

这里仅列举一些重要的选项，更多选项请参考手册：
`SO_KEEPALIVE`    设置“保活”选项，设置该选项后，`Tcp` 连接会定时给对端发送心跳包以检测对端是否存活。
`SO_RCVBUF`          设定接受缓冲区的大小。
`SO_SNDBUF`          设定发送缓冲区的大小。
`SO_REUSEADDR`    允许套接字绑定一个已被占用的端口。
`TCP_NODELAY`      关闭 `Tcp` 的 Nagle 算法（Tcp协议探究部分会说到）。
`SO_DONTROUTE `  禁止进行路由选径，直接进行发送。
`SO_DONTLINER`    如果缓冲区有残留数据，尝试发送给对端。
