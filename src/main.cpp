#include "server/server.h"

namespace config
{
    const int DEFAULT_SERVER_PORT = 9876;                   // web端口
    const int DEFAULT_TIMEOUT_MS = 60000;                   // 超时时间

    const int DEFAULT_MYSQL_PORT = 3306;                    // MySQL端口
    const char DEFAULT_MYSQL_USER[] = "root";               // 用户名
    const char DEFAULT_MYSQL_PWD[] = "Axope_mysql_123";     // 密码
    const char DEFAULT_MYSQL_DATABASES[] = "WebServerDB";   // 数据库名

    const int connPoolNum = 10;
    // IO密集型
    const int threadNum = 8;
}; // namespace config

int main()
{
    using namespace config;
    Server(
        DEFAULT_SERVER_PORT, DEFAULT_TIMEOUT_MS,

        DEFAULT_MYSQL_PORT, DEFAULT_MYSQL_USER,
        DEFAULT_MYSQL_PWD, DEFAULT_MYSQL_DATABASES,

        connPoolNum, threadNum).Start();

    return 0;
}
