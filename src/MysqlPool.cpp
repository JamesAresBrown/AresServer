/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "MysqlPool.h"
#include <mysql/mysql.h>
#include <cstdlib>
#include <list>

using namespace std;

MysqlPool::MysqlPool()
{
  cur_conn_ = 0;
  free_conn_ = 0;
}

MysqlPool *MysqlPool::GetInstance()
{
  static MysqlPool connPool;
  return &connPool;
}

//构造初始化
void MysqlPool::init(char* url, char* user, char* password, char* db, int port, int max_conn, int close_log)
{
  url_ = url;
  port_ = port;
  user_ = user;
  password_ = password;
  db_ = db;
  close_log_ = close_log;
  max_conn_ = max_conn;

  for (int i = 0; i < max_conn_; i++)
  {
    MYSQL *conn = nullptr;
    conn = mysql_init(conn);

    if (nullptr == conn) {
//            LOG_ERROR("MySQL Error");
      exit(1);
    }
    conn = mysql_real_connect(conn, url_, user_, password_, db_, port_, nullptr, 0);

    if (nullptr == conn) {
//            LOG_ERROR("MySQL Error");
        exit(1);
    }
    connList.push_back(conn);
    ++free_conn_;
  }

//    reserve = sem(free_conn_);
  condition_variable_.notify_all();

  max_conn_ = GetFreeConn();
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL* MysqlPool::GetConnection()
{
  MYSQL* con;

  if (connList.empty())
    return nullptr;

  {
    // 用条件变量等待连接队列中有可用的连接
    std::unique_lock<std::mutex> lock(connList_mutex_);
    // 获取锁并检查是否有可用的连接，如果没有则释放锁并阻塞等待
    condition_variable_.wait(lock, [this]() { return free_conn_ > 0; });
    con = connList.front();
    connList.pop_front();

    --free_conn_;
    ++cur_conn_;
  }

  return con;
}

//释放当前使用的连接
bool MysqlPool::ReleaseConnection(MYSQL* conn) {
  if (nullptr == conn)
    return false;
  {
    std::unique_lock<std::mutex> lock(connList_mutex_);
    connList.push_back(conn);
    ++free_conn_;
    --cur_conn_;
  }

  condition_variable_.notify_one();
  return true;
}

//销毁数据库连接池
void MysqlPool::DestroyPool()
{
    std::unique_lock<std::mutex> lock(connList_mutex_);
    if (!connList.empty())
    {
        list<MYSQL*>::iterator it;
        for (it = connList.begin(); it != connList.end(); ++it)
        {
            MYSQL *con = *it;
            mysql_close(con);
        }
        cur_conn_ = 0;
        free_conn_ = 0;
        connList.clear();
    }
}

//当前空闲的连接数
int MysqlPool::GetFreeConn() const
{
    return this->free_conn_;
}

MysqlPool::~MysqlPool()
{
    DestroyPool();
}
// 这里关于MYSQL额度参数采用的是指向MYSQL的地址的地址
connectionRAII::connectionRAII(MYSQL** SQL, MysqlPool *connPool){
    *SQL = connPool->GetConnection();

    connRAII_ = *SQL;
    poolRAII_ = connPool;
}

connectionRAII::~connectionRAII(){
    poolRAII_->ReleaseConnection(connRAII_);
}
