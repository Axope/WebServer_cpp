#include "http_conn.h"

const char *HttpConn::src_dir;
std::atomic<int> HttpConn::user_count;

HttpConn::HttpConn()
{
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
};

HttpConn::~HttpConn()
{
    Close();
};

void HttpConn::init(int fd, const sockaddr_in &addr)
{
    assert(fd > 0);
    user_count++;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.ResetPtr();
    readBuff_.ResetPtr();
    isClose_ = false;
    printf("Client[%d](%s:%d) in, user_count:%d\n", fd_, GetIP(), GetPort(), (int)user_count);
}

void HttpConn::Close()
{
    response_.UnmapFile();
    if (isClose_ == false)
    {
        isClose_ = true;
        user_count--;
        close(fd_);
        printf("Client[%d](%s:%d) quit, user_count:%d\n", fd_, GetIP(), GetPort(), (int)user_count);
    }
}

int HttpConn::GetFd() const
{
    return fd_;
};

struct sockaddr_in HttpConn::GetAddr() const
{
    return addr_;
}

const char *HttpConn::GetIP() const
{
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const
{
    return addr_.sin_port;
}

ssize_t HttpConn::Read(int *saveErrno)
{
    ssize_t len = -1;
     while (true)
    {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0)
        {
            break;
        }
    }
    return len;
}

ssize_t HttpConn::Write(int *saveErrno)
{
    ssize_t len = -1;
    while(true)
    {
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0)
        {
            *saveErrno = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0)
        {
            break;
        } /* 传输结束 */
        else if (static_cast<size_t>(len) > iov_[0].iov_len)
        {
            iov_[1].iov_base = (uint8_t *)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len)
            {
                writeBuff_.ResetPtr();
                iov_[0].iov_len = 0;
            }
        }
        else
        {
            iov_[0].iov_base = (uint8_t *)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.MoveReadPtr(len);
        }
    }
    return len;
}

bool HttpConn::Process()
{
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0)
    {
        return false;
    }
    else if (request_.parse(readBuff_))
    {
        printf("%s\n", request_.path().c_str());
        response_.Init(src_dir, request_.path(), request_.IsKeepAlive(), 200);
    }
    else
    {
        response_.Init(src_dir, request_.path(), false, 400);
    }

    response_.MakeResponse(writeBuff_);
    /* 响应头 */
    iov_[0].iov_base = const_cast<char *>(writeBuff_.Peek());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if (response_.FileLen() > 0 && response_.File())
    {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    printf("filesize:%d, %d  to %d\n", response_.FileLen(), iovCnt_, ToWriteBytes());
    return true;
}
