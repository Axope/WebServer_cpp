#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "../pool/sql_pool/sql_conn_RAII.h"
#include "../buffer/buffer.h"
#include "http_request.h"
#include "http_response.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>



class HttpConn
{
public:
    HttpConn();
    ~HttpConn();

    void init(int sockFd, const sockaddr_in &addr);
    void Close();

    ssize_t Read(int *saveErrno);
    ssize_t Write(int *saveErrno);

    int GetFd() const;
    int GetPort() const;
    const char *GetIP() const;
    sockaddr_in GetAddr() const;

    bool Process();

    int ToWriteBytes()
    {
        return iov_[0].iov_len + iov_[1].iov_len;
    }
    bool IsKeepAlive() const
    {
        return request_.IsKeepAlive();
    }

public:
    static const char *src_dir;
    static std::atomic<int> user_count;

private:
    int fd_;
    struct sockaddr_in addr_;
    bool isClose_;
    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;  // 读缓冲区
    Buffer writeBuff_; // 写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};

#endif // HTTP_CONN_H