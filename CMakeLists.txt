cmake_minimum_required(VERSION 2.8)
project(server)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(SRC_LIST 
${ROOT}/src/buffer/buffer.cpp
${ROOT}/src/http/http_conn.cpp 
${ROOT}/src/http/http_request.cpp 
${ROOT}/src/http/http_response.cpp 
${ROOT}/src/pool/sql_pool/sql_conn_pool.cpp 
${ROOT}/src/pool/sql_pool/sql_conn_RAII.cpp 
${ROOT}/src/pool/thread_pool/thread_pool.cpp 
${ROOT}/src/server/epoller.cpp 
${ROOT}/src/server/server.cpp 
${ROOT}/src/timer/timer.cpp 
${ROOT}/src/main.cpp 
)

set(CMAKE_CXX_FLAGS "-std=c++11 -W -Wall -O2")

add_library(mysqlclient SHARED IMPORTED)
set_target_properties(mysqlclient PROPERTIES IMPORTED_LOCATION /usr/lib64/mysql/libmysqlclient.so.21)

add_library(pthread SHARED IMPORTED)
set_target_properties(pthread PROPERTIES IMPORTED_LOCATION /lib64/libpthread.so.0)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} mysqlclient pthread)