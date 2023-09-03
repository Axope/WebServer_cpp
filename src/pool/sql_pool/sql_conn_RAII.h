#ifndef SQL_CONN_RAII_H
#define SQL_CONN_RAII_H
#include "sql_conn_pool.h"
#include <cassert>

/* 资源在对象构造初始化 资源在对象析构时释放*/
class SqlConnRAII
{
public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *connpool);
    ~SqlConnRAII();

private:
    MYSQL *sql_;
    SqlConnPool *connpool_;
};

#endif // SQLCONNRAII_H