/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_MYSQLPOOL_H
#define MYSERVER_MYSQLPOOL_H

#include <cstdio>
#include <list>
#include <mysql/mysql.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

class MysqlPool {
public:
    MYSQL* GetConnection();				 //获取数据库连接
    bool ReleaseConnection(MYSQL *conn); //释放连接
    int GetFreeConn() const;					 //获取连接
    void DestroyPool();					 //销毁所有连接

    //单例模式
    static MysqlPool *GetInstance();

    void init(char* url, char* User, char* PassWord, char* DataBaseName, int Port, int MaxConn, int close_log);

private:
    MysqlPool();
    ~MysqlPool();

    int max_conn_{};  //最大连接数
    int cur_conn_;  //当前已使用的连接数
    int free_conn_; //当前空闲的连接数
    list<MYSQL*> connList; //连接池
    std::mutex connList_mutex_;
    std::condition_variable condition_variable_;

public:
    char* url_{};			 //主机地址
    unsigned int port_{};		 //数据库端口号
    char* user_{};		 //登陆数据库用户名
    char* password_{};	 //登陆数据库密码
    char* db_{}; //使用数据库名
    int close_log_{0};	//日志开关
};
// 利用类的生命周期来主动获取数据库池中的连接，并且在对象消亡的时候主动释放。
class connectionRAII{

public:
    connectionRAII(MYSQL** con, MysqlPool *connPool);
    ~connectionRAII();

private:
    MYSQL* connRAII_;
    MysqlPool* poolRAII_;
};

class QueryRAII {
public:
    QueryRAII(MYSQL* mysql, const char *format, ...) {
        mysql_ = mysql;
        va_list args;
        va_start(args, format);

        char query[1000];
        vsprintf(query, format, args);

//        connectionRAII mysqlConn(&mysql, mysqlPool_);
        ret_ = mysql_query(mysql_, query);
//  if (res != nullptr)
        res_ = mysql_store_result(mysql_);

        if (ret_ != 0 || res_ == nullptr) {
//            std::cerr << "Error retrieving result set: " << mysql_error(mysql_) << std::endl;
        } else {
            MYSQL_ROW row;
//            while ((row = mysql_fetch_row(res_))) {
//                // 提取每一行的数据
////                unsigned long *lengths = mysql_fetch_lengths(res_);
//                for (int i = 0; i < mysql_num_fields(res_); ++i) {
//                    std::cout << "Column " << i << ": " << (row[i] ? row[i] : "NULL") << std::endl;
//                }
//            }
        }
        va_end(args);
    }
    ~QueryRAII() {
        if (res_ != nullptr) {
            mysql_free_result(res_);
        }
    }
//private:
    MYSQL* mysql_;
    MYSQL_RES* res_;
    int ret_ = -1;

};

#endif //MYSERVER_MYSQLPOOL_H
