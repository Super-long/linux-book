#跨机器的hello world
		这一小节，我们通过实现一个跨机器传输hello world例子，来学习socket的基本使用。

##客户端
```c
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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
        printf("argument error, please input ip and port\n");
        exit(1);
    }
        
    if((sockfd=socket(AF_INET,SOCK_STREAM, 0)) == -1)
    {
        printf("socket() error\n");
        exit(1);
    }
    
    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);
    
    if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        printf("connect() error\n");
        exit(1);
    }
    
    char* str = "hello world\n";
    
    if((num = send(sockfd, str, strlen(str), 0)) == -1)
    {
        printf("send() error\n");
        exit(1);
    }
    if((num = recv(sockfd, buf, MAXDATASIZE, 0)) == -1)
    {
        printf("recv() error\n");
        exit(1);
    }
    buf[num-1]='\0';
    printf("recv message: %s\n",buf);
    close(sockfd);

    return 0;
}
```

##服务端
```c
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BACKLOG 1
#define MAXRECVLEN 1024

int main(int argc, char *argv[])
{
    char buf[MAXRECVLEN];
    int listenfd, connectfd;   /* 套接字 */
    struct sockaddr_in server; /* 服务端地址信息 */
    struct sockaddr_in client; /* 客户端地址信息 */
    socklen_t addrlen;

    char* ip = argv[1];
    int port = atoi(argv[2]);

    if(argc != 3)
    {
        printf("argument error, please input ip and port\n");
        exit(1);
    }

    /* 创建socket套接字 */
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error. Failed to initiate a socket");
        exit(1);
    }
 
    /* 设置套接字参数 */
    int opt = SO_REUSEADDR;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);

    if(bind(listenfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("Bind() error.");
        exit(1);
    }
    
    if(listen(listenfd, BACKLOG) == -1)
    {
        perror("listen() error. \n");
        exit(1);
    }

    addrlen = sizeof(client);
    if((connectfd=accept(listenfd,(struct sockaddr *)&client, &addrlen))==-1)
    {
	    perror("accept() error. \n");
        exit(1);
    }

    bzero(buf, MAXRECVLEN);
    int iret = recv(connectfd, buf, MAXRECVLEN, 0);
    if(iret > 0)
    {
        printf("%s", buf);
    }else{
        close(connectfd);
    }
    send(connectfd, buf, iret, 0);   /* 回复客户端消息 */
    close(listenfd);                 /* 关闭socket套接字 */
    
    return 0;
}
```
##示例流程图
![enter image description here](https://github.com/wangweihao/linux-book/blob/master/system-call/chapter-7/image/c-s.png)


##描述客户端：
客户端通过 `socket` 函数创建一个 `socket` 套接字，`AF_INET` 表示套接字类型为 `ipv4`， `SOCK_STREAM` 表示套接字通信协议为 `TCP`，接着我们调用 `connect` 函数向服务器发起一个连接，参数为 `socket` 套接字以及服务器的地址信息，该函数将 `socket` 套接字转换为连接套接字。`connect` 成功后我们就可以进行通信啦，`send` 函数向服务器发送 `str` 数组里的内容 `hello world`，`recv` 函数接受服务器回复的内容。


##描述服务端：
服务端通过 `socket` 函数创建一个 `socket` 套接字，`AF_INET` 表示套接字类型为 `ipv4`， `SOCK_STREAM` 表示套接字通信协议为 `TCP`。接着我们调用 `bind` 函数将 `socket` 套接字和服务器地址进行绑定后，再调用 `listen` 函数将普通套接字转化为监听套接字，监听套接字可以监听客户端发来的连接请求。最后我们调用 `accept` 来接受一个连接并返回连接套接字，若成功返回连接套接字则表明双方为可以进行通信，`recv` 函数接受客户端发来的消息，处理后 `send` 发送回客户端。 
##相关socket函数及结构：
###地址相关结构体
```c
struct sockaddr_in
{
	short sin_family;              /* 协议 一般来说 AF_INET（地址族）PF_INET（协议族）*/

	unsigned short sin_port;       /* 端口号 (必须要采用网络数据格式,普通数字可以用htons()函数转换成网络数据格式的数字) */

	struct in_addr sin_addr;       /* ip 地址 */

	unsigned char sin_zero[8];     /* 没有实际意义,只是为了跟 SOCKADDR 结构在内存中对齐 */
};
```
上面我们演示了 socket 套接字通信，你肯定会有疑问，一个简单的整型 socket 为什么能进行通信，其实 socket 套接字仅是外部的表现而已，它通过 socket API 将一些信息封装后生成 socket 套接字以便于我们使用。socket 内部实际上是由 ip，端口以及协议组成。从上面的地址结构体信息我们就能看出来。

常见的协议有：
       
       AF_UNIX, AF_LOCAL   Local communication              unix(7)
       AF_INET             IPv4 Internet protocols          ip(7)
       AF_INET6            IPv6 Internet protocols          ipv6(7)

###地址相关API

###socket()
```c
#include <sys/types.h>
#include <sys/socket.h>

int socket(int domain, int type, int protocol);
```
返回值：
如果成功返回 `socket` 套接子，失败返回 `-1`。

socket() 函数创建一个通信端点并且返回一个 socket 描述符。一个网络 socket 套接字至少由以下部分组成：
 - domain：协议域，又称协议族（family）。常用的协议族有AF_INET、AF_INET6、AF_LOCAL（或称AF_UNIX，Unix域Socket）、AF_ROUTE等。协议族决定了socket的地址类型，在通信中必须采用对应的地址，如AF_INET决定了要用ipv4地址（32位的）与端口号（16位的）的组合、AF_UNIX决定了要用一个绝对路径名作为地址。
 - type：
 数据报 `socket`，一种无连接状态的 `socket`，使用 `UDP` 传输协议。
 传输流 `socket`，一种面向连接的 `socket`，使用 `TCP`。
 原生 `socket`，通常同在路由器等其他网络设备。
 - protocol：
指定协议。常用协议有IPPROTO_TCP、IPPROTO_UDP、IPPROTO_SCTP、IPPROTO_TIPC等，分别对应TCP传输协议、UDP传输协议、STCP传输协议、TIPC传输协议。

###connect()
```
#include <sys/types.h>         
#include <sys/socket.h>

int connect(int sockfd, const struct sockaddr *addr,                						socklen_t addrlen);

```
返回值：
连接成功，返回 `0`，失败返回 `-1`。并设置 `error` 错误码。
参数：
`sockfd` 为一个未连接的流或数据包套接字。
`addr` 为描述服务端信息的地址结构体。
`addrlen` 为 `addr` 的长度。
描述：
`connect` 函数用于创建一个连接套接字。
如果是 `Tcp` 连接，`connect` 函数在内部进行 `Tcp` 三次握手过程。成功返回则表示 `Tcp` 连接建立成功。
如果是 `Udp`，通常不调用 `connect` 函数，每次发送消息指明对端的地址信息。不过 `Udp`协议也能调用 `connect` 函数，这样每次发送消息时我们不用明确指出对端的地址信息 ，并且仅仅调用一次连接过程，可以提高效率。

###bind()
```
#include <sys/types.h>          
#include <sys/socket.h>

int bind(int sockfd, const struct sockaddr *addr,                 socklen_t addrlen);
```
返回值：
若成功返回 `0`，失败返回 `-1`，并设置 `errno` 错误码。
参数：
`sockfd` 为原生套接字。
`addr` 为地址信息结构。
`addrlen` 为地址信息结构的长度。
描述：
在 `Tcp` 流和 `Udp` 数据报中，具名套接字包含 `3` 部分，协议、`ip`和区分应用的端口号，`bind` 函数将本地的原生套接字和具体的地址信息绑定。

###listen()
```
#include <sys/types.h>          
#include <sys/socket.h>

int listen(int sockfd, int backlog);
```
返回值：
若成功返回 `0`，失败返回 `-1`，并设置错误码 `errno` 错误码。
参数：
`sockfd` 为绑定地址信息后的套接字。
`backlog` 为“已连接队列的最大长度”（也有系统表示为未连接队列和已连接队列的和值），在 `Tcp` 连接进行完毕三次握手时，也就是客户端调用 `connect` 函数后，服务端调用 `accept` 函数前，操作系统内部会为 `Tcp` 连接维持一个已连接队列，将未进行 `accpet` 调用的连接放入已连接队列。当已连接队列满时，新连接会被拒绝，所以我们应根据程序的需要来设定 `backlog` 参数的大小。
描述：
`listen` 函数接受一个绑定地址信息未接受连接的套接字，并将该套接字转化为“监听套接字”，这样，调用 `listen` 的函数就相当于服务器，该套接字也可以接受连接。


###accept()
```
#include <sys/types.h>          
#include <sys/socket.h>

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```
返回值：
若成功则返回一个连接套接字，此后该套接字可以进行收发信息。若失败则返回 `-1`，并设置 `errno` 错误码。

参数：
`sockfd` 为监听套接字。
`addr` 为地址信息结构体，用来保存客户端的地址信息结构（前面我们说过一个收发数据的连接套接字需要客户端`ip`和服务端`ip`，客户端端口和服务端端口以及协议）。
`addrlen` 为地址信息结构长度。

描述：
`accept` 函数从“已连接队列”中取出一个成功建立的连接并返回一个连接套接字，该套接字可以进行通信。
在调用该函数时，若 `socket` 套接字是阻塞的且已连接队列已满，则 `accept` 函数会一致等待直到已连接队列有空位置，若 `socket` 套接字为非阻塞，如果已连接队列有空位则 `accpet` 函数调用成功并返回连接套接字，否则返回失败。

注意阻塞和非阻塞的区别：
阻塞：阻塞是你等待事件时为挂起状态，此时你什么也不能干，只能在等待事件。 
非阻塞：非阻塞是等待事件时，如果事件没发生，不挂起自己 。

		在旧版本内核中，设置非阻塞 socket 我们必须调用 fcntl 函数。函数如下：
		#include <unistd.h>
        #include <fcntl.h>
        int fcntl(int fd, int cmd, ... /* arg */ );
	
		新版本内核中，我们可以直接调用 accept4 函数在接受连接时直接将该 socket 套接字设置为非阻塞。该函数如下：
       #define _GNU_SOURCE            
       #include <sys/socket.h>
       int accept4(int sockfd, struct sockaddr *addr,
                   socklen_t *addrlen, int flags);

        
###send()
```
#include <sys/types.h>
#include <sys/socket.h>

ssize_t send(int sockfd, const void *buf, size_t len, int flags);
```
返回值：
若成功则返回发送的字节数，失败则返回 `-1` 并设置 `errno` 错误码。

参数：
`sockfd` 为已连接套接字，`buf` 为发送缓冲区，`len` 为发送缓冲区长度，`flags` 为 `send` 函数调用执行的方式。

描述：
`send` 函数适用于 `Tcp` 连接发送数据。注意 `send` 函数返回成功并不意味着数据传送到达对端。仅仅说明数据被拷贝到内核的发送缓冲区中。
 
###recv()
```
#include <sys/types.h>
#include <sys/socket.h>

ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```
返回值：
若成功则返回发送的字节数，失败则返回 `-1` 并设置 `errno` 错误码。

参数：
`sockfd` 为已连接套接字，`buf` 为接受缓冲区，`len` 为接受缓冲区的长度，`flags` 为 `recv` 函数调用执行的方式。

描述：
`recv` 函数适用于 `Tcp` 连接接受数据。注意 `recv` 函数仅仅是从内核的接受缓冲区将数据拷贝到设置的 `buf` 中，真正的接受数据由协议来完成。
 
###close()
```
#include <unistd.h>

int close(int fd);
```
返回值：
若成功返回 `0`，失败返回 `-1`，并设置错误码 `errno` 错误码。

参数：
`fd` 为文件描述符，包括且不限于 `socket` 套接字。

描述：
`close` 函数用来关闭一个 `socket` 套接字，关闭后该 `socket` 套接字归还给系统，不能再用来通信。
注意：
一个系统所设置的 `socket` 套接字数量是有限的，不用的 `socket` 套接字我们要及时归还给系统，避免文件描述符不足而导致不能创建 `socket` 套接字。
`Linux` 下可以通过修改 `fs.file-max` 来修改文件描述符的最大数量。`fd.file-max` 在目录 `/proc/sys/fs/file-max` 下。
