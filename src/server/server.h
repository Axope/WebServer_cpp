#ifndef Server_H
#define Server_H

#include "epoller.h"
#include "../timer/timer.h"
#include "../pool/sql_pool/sql_conn_pool.h"
#include "../pool/sql_pool/sql_conn_RAII.h"
#include "../pool/thread_pool/thread_pool.h"
#include "../http/http_conn.h"

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Server
{
public:
    Server(
        int port, int timeoutMS,
        int sqlPort, const char *sqlUser, const char *sqlPwd,
        const char *dbName, int connPoolNum, int threadNum);

    ~Server();
    void Start();

private:
    bool InitSocket_();
    void InitEventMode_();
    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();
    void DealWrite_(HttpConn *client);
    void DealRead_(HttpConn *client);

    void SendError_(int fd, const char *info);
    void ExtentTime_(HttpConn *client);
    void CloseConn_(HttpConn *client);

    void OnRead_(HttpConn *client);
    void OnWrite_(HttpConn *client);
    void OnProcess_(HttpConn *client);

    static int SetNonblocking_(int fd);

private:
    static const int MAX_FD = 65535;

    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClose_;
    int listenFd_;
    char *srcDir_;

    // 基础属性
    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<Timer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};

#endif // Server_H