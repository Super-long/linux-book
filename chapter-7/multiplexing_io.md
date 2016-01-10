#IO多路复用：
##概述：
  上面的例子我们仅监听一个socket，但实际中，大部分情况单一socket并不能满足我们的需求，比如服务器同时监听多个客户端。所以出现了IO多路复用。传统的编程模型是通过创建多个线程来管理多个数据流。IO多路复用是在单个线程通过记录每一个流的状态来管理多个流。
  举个现实生活中的例子，比如你是某个快递企业的老总，现在你要给一栋楼送10份快递。传统的编程模型是排除10个人去送快递，到达楼下后，如果对方没来就死等，直到对方来为止，这种情况很明显效率较低，10个快递就需要10个人，那么100个呢？1000个呢？IO多路复用就好比派一个人去送这10个包裹，select没有我们稍后说的epoll聪明，它会进入楼中，挨个房间询问，这是不是你的快递，直到询问完毕，将该送的快递送给对方。但这也有很明显的缺陷，如果房间很多，实际收快递的缺很少，这样效率也非常低。稍后我们来看epoll是如何解决这个问题的。
  IO多路复用使得单程序能够同时监听多个socket。
  我们具体来看看。
		
##select
```
#include <sys/select.h>

/* select 函数调用，返回时将参数中对应的集合状态进行设置 */
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

/* 以下四个函数是对状态集合的操作 */
void FD_CLR(int fd, fd_set *set);    /* 将某个 fd 从集合中删除 */
int  FD_ISSET(int fd, fd_set *set);  /* 检测状态是否变化 */
void FD_SET(int fd, fd_set *set);    /* 将 fd 加入集合中 */
void FD_ZERO(fd_set *set);           /* 将集合清零*/
```
返回值：
若成功，则设置修改集合内部的标记位，表明对应事件发生。
若失败，则返回 `-1`，并设置 `errno` 标志位。

参数：
`nfds` 表示监听描述符的最大值加一。
`readfds`，`writefds`，`exceptfds` 是监听事件的集合，我们把想要将听的事件通过 `FD_SET` 加入到集合中即可。
`timeout` 是超时值。
```
struct timeval
{
	long tv_sec;   /* 秒数 */
	long tv_usec;  /* 微秒数 */
}
```
若 `timeout` 为 `0`，表示 `select` 为阻塞状态，无事件发生，则一致阻塞。
若 `timeout` 不为 `0`，表示定时，在给定时间内没有事件发生，则返回。
若 `timeout` 为 `-1`，表示非阻塞，无事件发生立即返回。

来看个例子：

客户端：
```
#include <stdlib.h>
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
    close(sockfd);

    return 0;
}
```

服务端：
```
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BACKLOG 1
#define MAXRECVLEN 1024
#define FD_MAXSIZE 1024

int main(int argc, char *argv[])
{
    char buf[MAXRECVLEN];
    int listenfd, connectfd;   /* 套接字 */
    struct sockaddr_in server; /* 服务端地址信息 */
    struct sockaddr_in client; /* 客户端地址信息 */
    socklen_t addrlen;
    int ret;

    char* ip = argv[1];
    int port = atoi(argv[2]);

    if(argc != 3){
        printf("argument error, please input ip and port\n");
        exit(1);
    }

    /* 创建socket套接字 */
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        /* 处理错误 */
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

    
    fd_set readfds, writefds, errorfds;
    FD_ZERO(&readfds);
    FD_ZERO(&errorfds);

    struct timeval time;
    time.tv_sec = 20;
    time.tv_usec = 0;
    while(1){
        FD_SET(listenfd, &readfds);
        FD_SET(listenfd, &errorfds);

        ret = select(FD_MAXSIZE, &readfds, NULL, &errorfds, &time);
        if(ret < 1)
        {
            perror("select() error\n");
        }
        for(int fd = 0; fd < FD_MAXSIZE; ++fd)
        {
            if(FD_ISSET(fd, &readfds))
            {
                if(fd == listenfd)
                {
                    socklen_t len = sizeof(client);
                    int connfd = accept(fd, (struct sockaddr*)&client, &len);
                    if(connfd == -1)
                    {
                        perror("accept() error\n");
                        exit(1);
                    }
                    FD_SET(connfd, &readfds);
                    printf("接受客户端连入，加入到监听集合中...\n");
                }else{
                    char buffer[1024];
                    bzero(buffer, 1024);
                    recv(fd, buffer, 1024, 0);
                    printf("接受数据:%s\n", buffer);
                }
                /* 注意事件产生后，处理完毕我们要主动将状态清楚等待下一次事件产生，若未清楚则事件会一致触发 */
                FD_CLR(fd, &readfds);
            }   
            if(FD_ISSET(fd, &errorfds))
            {
                printf("异常事件产生...\n");
                FD_CLR(fd, &errorfds);
            } 
        }
    }
    
    return 0;
}
```

服务器：
$ ./server 127.0.0.1 10000
接受客户端连入，加入到监听集合中...
接受数据:hello world

客户端：
$ ./client 127.0.0.1 10000


##epoll
概述：
前面我们介绍了 `select`，在送快递的例子中，我们能明显看到 `select`这个员工虽然勤奋，但效率不高。接下来我们来看看 `epoll`。
`epoll` 是为聪明的员工，它不会跑到楼上一间房一间房的询问，而是给整个楼的人留一个自己的联系方式，当楼中的人有快递要取时，他们会给 `epoll` 发一个短信告知自己的地点。" Hi，`epoll` 大叔，我的房间是 `114`，我有快递！" 这样 `epoll` 只需要汇总短信就能知道所有需要取快递人的房间号，记录下这些房间号，然后找到房间去送即可。
这样无疑效率是非常高的，避免了盲目的寻找。
注意，`epoll` 是 `Linux` 下特有的。

```
#include <sys/epoll.h>

int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
```
`epoll` 由三个函数组成：

`epoll_create()` 表示创建一个 `epoll` 文件描述符，之后所有的操作要通过这个文件描述符来操作，`size` 表示要监听的事件数量。

`epoll_ctl()`  表示向 `epoll_create` 创建的 `epfd` 上注册我们感兴趣的事件，`op` 为表示的动作，可以为以下几种：
`EPOLL_CTL_ADD`：注册新的fd到epfd中；
`EPOLL_CTL_MOD`：修改已经注册的fd的监听事件；
`EPOLL_CTL_DEL`：从epfd中删除一个fd； 
`event` 是一个 `epoll_event` 结构体类型
```
struct epoll_event {
  __uint32_t events;  /* Epoll 事件 */
  epoll_data_t data;  /* 用户数据信息 */
};
```
`events` 是我们用来注册感兴趣的事件。可以为以下几种：
`EPOLLIN` ：表示对应的文件描述符可以读（包括对端 `SOCKET` 正常关闭）；
`EPOLLOUT`：表示对应的文件描述符可以写；
`EPOLLPRI`：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
`EPOLLERR`：表示对应的文件描述符发生错误；
`EPOLLHUP`：表示对应的文件描述符被挂断；
`EPOLLET`： 将 `EPOLL` 设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的，默认为 `LT` 水平触发。
`EPOLLONESHOT`：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个 `socket` 的话，需要再次把这个 `socket` 加入到 `EPOLL` 队列里

`epoll_wait()` 等待事件的产生，返回发生事件的数目。
`epfd` 为 `epoll` 文件描述符，`events` 为发生事件的集合，`maxevents` 为发生事件的最大数目，`timeout` 和 `select` 的类似。


我们一起来看个例子：
客户端使用 `select` 部分的客户端。
服务端如下：
```
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>

#define BACKLOG 1
#define MAXRECVLEN 1024

#define MAX_EVENT_NUMBER 1000

/* 设置非阻塞 */
int setnonblocking(int fd)
{
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
    return old_flag;
}

/* 注册监听读事件 */
void addfd(int epollfd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

int main(int argc, char *argv[])
{
    char buf[MAXRECVLEN];
    int listenfd, connectfd;   /* 套接字 */
    struct sockaddr_in server; /* 服务端地址信息 */
    struct sockaddr_in client; /* 客户端地址信息 */
    socklen_t addrlen;

    char* ip = argv[1];
    int port = atoi(argv[2]);

    if(argc != 3){
        printf("argument error, please input ip and port\n");
        exit(1);
    }

    /* 创建socket套接字 */
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        /* 处理错误 */
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

	/* 绑定端口 */
    if(bind(listenfd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("Bind() error.");
        exit(1);
    }
    
    /* 监听 socket */
    if(listen(listenfd, BACKLOG) == -1)
    {
        perror("listen() error. \n");
        exit(1);
    }

	/* 保存 epoll 发生事件的结构体 */
    struct epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    if(epollfd < 0)
    {
        perror("epoll_create() error\n");
        exit(1);
    }
    /* 将服务端 socket 加入监听事件中 */
    addfd(epollfd, listenfd);

    while(1)
    {
	    /* epoll_wait 返回发生事件的数目 */
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, 0);
        if(ret < 0)
        {
            perror("epoll_wait() error\n");
            exit(1);
        }
        for(int i = 0; i < ret; ++i)
        {
            if(events[i].data.fd == listenfd)
            {
                socklen_t len = sizeof(client);
                int connfd = accept(listenfd, (struct sockaddr*)&client, &len);
                if(connfd < 0)
                {
                    perror("accept() error\n");
                    exit(1);
                }           
                addfd(epollfd, connfd);    
            }else if(events[i].events & EPOLLIN)
            {
                char buffer[1024];
                int num;
                bzero(buffer, 1024);
                if((num = recv(events[i].data.fd, buffer, 1024, 0)) < 0)
                {
                    perror("recv() error\n");
                    exit(1);    
                }
                if(num == 0)
                {
                    close(events[i].data.fd);
                }else{
                    printf("读事件触发...\n");
                    printf("recv:%s\n", buffer);
                }
            }
        }
    }

    close(listenfd); /* close listenfd */
    return 0;
}
```
$ ./server 127.0.0.1 10000
读事件触发...
recv:hello world

$./client 127.0.0.1 10000

