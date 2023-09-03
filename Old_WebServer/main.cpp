#include "locker.h"
#include "thread_pool.h"
#include "http_conn.h"

constexpr int MAX_EVENT_NUMBER = 100000;
constexpr int MAX_FD = 100000;

// 创建epoll对象添加事件
epoll_event events[MAX_EVENT_NUMBER];

extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);

// 修改与信号相关联的动作
void addsig(int sig, void(handler)(int))
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        std::cout << "Please enter the port number" << std::endl;
        exit(-1);
    }

    int port = atoi(argv[1]);
    addsig(SIGPIPE, SIG_IGN);  // SIGPIPE默认会终止进程, 改为SIG_ING忽略信号

    thread_pool<http_conn>* poll = nullptr;
    try
    {
        poll = new thread_pool<http_conn>;
    }
    catch (...)
    {
        exit(-1);
    }

    // 主线程：监听+将数据读到缓冲区
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        perror("socket");
        exit(-1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    // 设置端口复用
    int reuse = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
    {
        perror("setsockopt");
        exit(-1);
    }
    // 绑定
    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        exit(-1);
    }
    // 监听
    if (listen(listenfd, 10) == -1)
    {
        perror("listen");
        exit(-1);
    }

    // 向epoll中添加需要监听的文件描述符
    int epollfd = epoll_create(5);
    addfd(epollfd, listenfd, false);
    http_conn::m_epollfd = epollfd;

    // 表示客户端的连接套接字
    http_conn* users = new http_conn[MAX_FD];

    while (1)
    {
        // epoll监听事件(阻塞)
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

        // 错误
        if (num < 0 && errno != EINTR)
        {
            std::cout << "EPOLL FAILED" << std::endl;
            break;
        }
        // 遍历事件
        for (int i = 0; i < num; i++)
        {
            int sockfd = events[i].data.fd;
            // 监听套接字
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int clientfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
                if (clientfd < 0)
                {
                    std::cout << "prrno is : " << errno << std::endl;
                    continue;
                }

                if (http_conn::m_user_num >= MAX_FD)
                {
                    close(clientfd);
                    continue;
                }
                users[clientfd].init(clientfd, client_addr);
            }
            // 错误, 应及时关闭连接
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].close_conn();
            }
            // 读数据
            else if (events[i].events & EPOLLIN)
            {
                if (users[sockfd].read())
                {
                    poll->append(users + sockfd);
                }
                else {
                    users[sockfd].close_conn();
                }
            }
            // 写数据
            else if (events[i].events & EPOLLOUT)
            {
                if (!users[sockfd].write())
                {
                    users[sockfd].close_conn();
                }
            }
        }

    }

    // 善后处理
    close(epollfd);
    close(listenfd);
    delete[] users;
    delete poll;

    return 0;
}