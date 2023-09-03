#include "server.h"

Server::Server(
    int port, int timeoutMS,
    int sqlPort, const char *sqlUser, const char *sqlPwd,
    const char *dbName, int connPoolNum, int threadNum) : 
        port_(port), timeoutMS_(timeoutMS), isClose_(false),
        timer_(new Timer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    HttpConn::user_count = 0;
    HttpConn::src_dir = srcDir_;
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

    InitEventMode_();
    if (!InitSocket_())
    {
        isClose_ = true;
    }
}

Server::~Server()
{
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

void Server::InitEventMode_()
{
    listenEvent_ = EPOLLRDHUP | EPOLLET;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP | EPOLLET;
}

void Server::Start()
{
    int timeMS = -1; /* epoll wait timeout == -1 无事件将阻塞 */
    if (!isClose_)
    {
        printf("Server start...\n\n\n");
    }
    while (!isClose_)
    {
        if (timeoutMS_ > 0)
        {
            timeMS = timer_->GetNextTick();
        }
        int eventCnt = epoller_->Wait(timeMS);
        for (int i = 0; i < eventCnt; i++)
        {
            /* 处理事件 */
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if (fd == listenFd_)
            {
                DealListen_();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }
            else if (events & EPOLLIN)
            {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }
            else if (events & EPOLLOUT)
            {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            }
            else
            {
                printf("Unexpected event\n");
            }
        }
    }
}

void Server::SendError_(int fd, const char *info)
{
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0)
    {
        printf("send error to client[%d] error!\n", fd);
    }
    close(fd);
}

void Server::CloseConn_(HttpConn *client)
{
    assert(client);
    printf("Client[%d] quit!\n", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void Server::AddClient_(int fd, sockaddr_in addr)
{
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if (timeoutMS_ > 0)
    {
        timer_->Add(fd, timeoutMS_, std::bind(&Server::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetNonblocking_(fd);
    printf("Client[%d] in!\n", users_[fd].GetFd());
}

void Server::DealListen_()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do
    {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if (fd <= 0)
        {
            return;
        }
        else if (HttpConn::user_count >= MAX_FD)
        {
            SendError_(fd, "Server busy!");
            printf("Clients is full!\n");
            return;
        }
        AddClient_(fd, addr);
    } while (listenEvent_ & EPOLLET);
}

void Server::DealRead_(HttpConn *client)
{
    assert(client);
    ExtentTime_(client);
    auto fuc = std::bind(&Server::OnRead_, this, client);
    threadpool_->AddTask(std::move(fuc));
}

void Server::DealWrite_(HttpConn *client)
{
    assert(client);
    ExtentTime_(client);
    auto fuc = std::bind(&Server::OnWrite_, this, client);
    threadpool_->AddTask(std::move(fuc));
}

void Server::ExtentTime_(HttpConn *client)
{
    assert(client);
    if (timeoutMS_ > 0)
    {
        timer_->Adjust(client->GetFd(), timeoutMS_);
    }
}

void Server::OnRead_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->Read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN)
    {
        CloseConn_(client);
        return;
    }
    OnProcess_(client);
}

void Server::OnProcess_(HttpConn *client)
{
    if (client->Process())
    {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    }
    else
    {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void Server::OnWrite_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->Write(&writeErrno);
    if (client->ToWriteBytes() == 0)
    {
        /* 传输完成 */
        if (client->IsKeepAlive())
        {
            OnProcess_(client);
            return;
        }
    }
    else if (ret < 0)
    {
        if (writeErrno == EAGAIN)
        {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

/* Create listenFd */
bool Server::InitSocket_()
{
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024)
    {
        printf("Port:%d error!\n", port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0)
    {
        printf("Create socket error!\n", port_);
        return false;
    }

    if (ret < 0)
    {
        close(listenFd_);
        printf("Init linger error!\n", port_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    if (ret == -1)
    {
        printf("set socket setsockopt error !\n");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        printf("Bind Port:%d error!\n", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if (ret < 0)
    {
        printf("Listen port:%d error!\n", port_);
        close(listenFd_);
        return false;
    }
    ret = epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if (ret == 0)
    {
        printf("Add listen error!\n");
        close(listenFd_);
        return false;
    }
    SetNonblocking_(listenFd_);
    printf("Server port:%d\n", port_);
    return true;
}

int Server::SetNonblocking_(int fd)
{
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
