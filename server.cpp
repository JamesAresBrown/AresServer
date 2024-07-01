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
std::mutex mutex_;

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
    string SERVER_ROOT_PATH = toString(config_map.at("SERVER.SERVER_ROOT_PATH"));
    string SERVER_DATA_PATH = toString(config_map.at("SERVER.SERVER_DATA_PATH"));
    string SERVER_MYSQL_URL = toString(config_map.at("SERVER.SERVER_MYSQL_URL"));
    string SERVER_MYSQL_USER = toString(config_map.at("SERVER.SERVER_MYSQL_USER"));
    string SERVER_MYSQL_PASSWORD = toString(config_map.at("SERVER.SERVER_MYSQL_PASSWORD"));
    string SERVER_MYSQL_DATABASENAME = toString(config_map.at("SERVER.SERVER_MYSQL_DATABASENAME"));
    int SERVER_MYSQL_PORT = toInt(config_map.at("SERVER.SERVER_MYSQL_PORT"));


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
            HttpMVS* http = new HttpMVS();
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

//    std::thread([&](){
//        while (true) {
//            std::this_thread::sleep_for(std::chrono::seconds{1});
//            {
//                std::unique_lock<std::mutex> lock(mutex_);
//                if (should_exit) {
////                    loop->quit_ = true;
//                    delete server;
//                    server = nullptr;
////                    delete loop;
//                    exit(1);
//                    break;
//                }
//            }
//
//        }
//    }).detach();

//    server->Run();
    std::thread([&](){
        while (true) {
            if (should_exit) {
                delete server;
                exit(0);
            }
        }
    }).detach();

    server->Run();

//    server = nullptr;
//    delete mysqlPool;
//    delete loop;
    return 0;
}
