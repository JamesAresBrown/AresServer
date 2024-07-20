/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include <csignal>

#include "Server.h"
#include "Connection.h"
#include "EventLoop.h"
#include "config.h"
#include "Socket.h"
#include "MysqlPool.h"
#include "HttpMVS.h"
#include "CacheClient.h"

bool should_exit = false;

void signal_handler(int signal) {
    std::cout << "接收到信号 " << signal << "，正在退出程序..." << std::endl;
    should_exit = true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("please load config!");
        exit(1);
    }

    // 配置系统
    std::map<std::string, std::string> config_map = loadConfig(argv[1]);
    string SERVER_ROOT_PATH = toString(config_map.at("SERVER.ROOT_PATH"));
    string SERVER_DATA_PATH = toString(config_map.at("SERVER.DATA_PATH"));
    string SERVER_MYSQL_URL = toString(config_map.at("MYSQL.URL"));
    string SERVER_MYSQL_USER = toString(config_map.at("MYSQL.USER"));
    string SERVER_MYSQL_PASSWORD = toString(config_map.at("MYSQL.PASSWORD"));
    string SERVER_MYSQL_DATABASENAME = toString(config_map.at("MYSQL.DATABASENAME"));
    int SERVER_MYSQL_PORT = toInt(config_map.at("MYSQL.PORT"));


    signal(SIGINT, signal_handler); // Ctrl+C
    signal(SIGTERM, signal_handler); // 终止信号


    // 创建数据库连接池
    MysqlPool *mysqlPool;
    mysqlPool = MysqlPool::GetInstance();
    mysqlPool->init(const_cast<char *>(SERVER_MYSQL_URL.c_str()), const_cast<char *>(SERVER_MYSQL_USER.c_str()),
                    const_cast<char *>(SERVER_MYSQL_PASSWORD.c_str()),
                    const_cast<char *>(SERVER_MYSQL_DATABASENAME.c_str()), SERVER_MYSQL_PORT, 8, 0);
    // 线程池和数据库连接池是的生命周期和服务器的一样
    // Redis
    CacheClient *cacheClient;
    cacheClient = CacheClient::GetInstance();
    cacheClient->init(toString(config_map.at("REDIS.URL")));
    // 创建服务器，与事件循环绑定
    Server *server;
    server = new Server();
    printf("begin!\n");

    server->ReadConnect([&](Connection *conn) {
        // 这是个非常恶心的臭虫，初始化操作不可以放在读取中，不然没当读一次，绑定的应用对象就被重新创建一次
        if (nullptr == conn->application_) {
            auto* http = new HttpMVS();
            conn->application_ = http;
            // 采用虚函数 以下是一些自定义的初始化操作
            http->Init(const_cast<char *>(SERVER_ROOT_PATH.c_str()), const_cast<char *>(SERVER_DATA_PATH.c_str()), conn->GetChannel());
            http->SetMysqlPool(mysqlPool);
            http->SetCacheClient(cacheClient);
        }

        if (conn->GetChannel()->read_callback_ == nullptr) {
            conn->Close();
        }
        // 应用层拿不到socket，在应用层缓冲区读写
        conn->application_->Read();

        if (!conn->application_->Process()) {
            conn->Close();
        }
    });

    server->WriteConnect([](Connection *conn){
        if (!conn->application_->Write()) {
//            delete static_cast<HttpConnection*>(conn->application_);
            conn->Close();
        }
    });

    auto t = std::thread([&](){
        while (true) {
            this_thread::sleep_for(2s);
            if (should_exit) {
                server->Stop();
                delete server;
                server = nullptr;
                return;
            }
        }
    });

    server->Run();

    t.join();


//    server = nullptr;
//    delete mysqlPool;
//    delete loop;
    return 0;
}
