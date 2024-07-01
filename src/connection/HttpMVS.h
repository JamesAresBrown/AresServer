/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_HTTPMVS_H
#define MYSERVER_HTTPMVS_H

#include <iostream>
#include <sys/stat.h>
#include <map>
#include <sys/uio.h>
#include "MysqlPool.h"
#include "Channel.h"
#include "Application.h"
#include "CacheClient.h"

class HttpMVS : public Application {
//    static std::map<std::string, std::string> users_;
//    static std::mutex mutex_;
    //定义http响应的一些状态信息
    const char *ok_200_title = "OK";
    const char *error_400_title = "Bad Request";
    const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
    const char *error_403_title = "Forbidden";
    const char *error_403_form = "You do not have permission to get file form this server.\n";
    const char *error_404_title = "Not Found";
    const char *error_404_form = "The requested file was not found on this server.\n";
    const char *error_500_title = "Internal Error";
    const char *error_500_form = "There was an unusual problem serving the request file.\n";


public:
    static const int FILENAME_LEN = 200;
    static const unsigned long READ_BUFFER_SIZE = 2048;
    // 事实上写缓冲用来写回报的响应行与头，这个长度满足这两部分就行
    static const unsigned long WRITE_BUFFER_SIZE = 2048;
    enum METHOD
    {
        GET = 0,
        POST,
//        HEAD,
//        PUT,
//        DELETE,
//        TRACE,
//        OPTIONS,
//        CONNECT,
//        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST, // 并不像字面意思一样，如果想要继续读，就设置改状态
        GET_REQUEST, // 获取响应
        BAD_REQUEST, // 反馈失败.
        NO_RESOURCE, // 没有文件
        FORBIDDEN_REQUEST, // 没有权限访问.
        FILE_REQUEST, //.
        INTERNAL_ERROR, // 服务器错误.
        CLOSED_CONNECTION,
        LOAD_FILE
    };
    enum LINE_STATE
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    enum PROCESS_WRITE_STATE {
        TO_WRITE = 1,
        NO_WRITE = 0,
        PROCESS_WRITE_ERROR = -1
    };

public:
    HttpMVS();
    ~HttpMVS() override;
    HttpMVS(const HttpMVS&) = delete; /* NOLINT */
    HttpMVS& operator=(const HttpMVS&) = delete; /* NOLINT */
    HttpMVS(HttpMVS&&) = delete; /* NOLINT */
    HttpMVS& operator=(HttpMVS&&) = delete; /* NOLINT */

    bool Process() override;
    bool Read() override;
//    char* GetWriteBuf() const { return m_write_buf; }
    bool Write() override; // 0失败 1发送成功 -1阻塞
    void Init(char* doc_root, char* data_path, Channel* channel);
    void SetMysqlPool(MysqlPool* mysqlPool);
    void SetCacheClient(CacheClient* cacheClient);

private:
//    void init();
    char* get_line() { return read_buf_ + start_line_; };
    HTTP_CODE ProcessRead();
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    std::string generateUUID();
//    void ClearReadBuf(int sec);
//    void ClearWriteBuf(int sec);
    void Reset(int cho);

    HTTP_CODE DoRequest();

    HttpMVS::PROCESS_WRITE_STATE ProcessWrite(HTTP_CODE ret);
    LINE_STATE ParseLine();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type(const char* content_type = "");
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
    bool add_token();
    std::string generateToken(int length);

private:
    MysqlPool* mysqlPool_{};
    CacheClient* cacheClient_{};
    char* doc_root_{};
    int TRIGMode_{0};
    char real_file_[FILENAME_LEN]{};
    char data_path_[1024]{};
//    enum CLIENT_STATE {
//        ZOMBIE = -1,
//        NO_LOGGED = 0,
//        ACTIVATE = 1
//    };

private:
//    struct keep_alive_data { // keep alive data
//        char name[100];
//        char password[100];
//        int state = NO_LOGGED;
//        char res_filename[17] = "rrrrrrrrrrrr.ply";
//    }kad_;

    std::string token_;

private:
    int fd_{};
    Channel* channel_{};

    char read_buf_[READ_BUFFER_SIZE]{}; // 读缓冲区
    unsigned int read_buf_idx_{}; // 指向读缓冲的最新字节
    long checked_idx_{}; // 在缓冲区中已经检查的字节数量，必须永远在追随read_buf_idx_
    unsigned int start_line_{};

    char write_buf_[WRITE_BUFFER_SIZE]{}; // 写缓冲区
    unsigned int m_write_idx{}; // 写字节数
    long bytes_to_send{};
    unsigned long bytes_have_send{};

    CHECK_STATE check_state_;
    METHOD m_method;
    char *url_{};
    char *version_{};
    char *host_{};
    unsigned long read_idx_{}; // 读取的总字节数
    unsigned int head_length_{0}; // 头部长度，实际上是整个http包长度-content_length_，并不只是头部部分
    unsigned long content_length_{0};
    bool keep_alive_{};
    char *file_address_{};
    struct stat m_file_stat{};
    struct iovec m_iv[2]{};
    int m_iv_count{};
    int cgi_{}; //是否启用的POST
    char *string_{}; //存储请求头数据

    char* boundary_{nullptr};
    char* filename_{nullptr};
    unsigned long file_size_{0};
    unsigned int to_download_idx_{0};
    unsigned long download_idx_{0};
    enum DOWNLOADING_STATE {
        LOAD_NO = -1,
        LOAD_ING = 0,
        LOAD_DONE = 1,
        LOAD_SIZE_HEAD = 2,
        LOAD_SIZE = 3,
        LOAD_DATA_HEAD = 4,
        LOAD_DATA = 5,
        LOAD_DATA_DONE = 6,
    };
    int download_status_ = LOAD_NO;
    bool have_file_to_download_ = false;
    unsigned short reread_times_ = 0;
    std::string load_path_;
    std::string fileID_;

//    int process_state_ = -1;

    std::map<std::string, std::map<std::string, std::string>> data_ {
        {"users", {}},
        {"material", {}},
        {"results", {}}
    };
    std::map<std::string, std::string> PRIMARY_KEY {
            {"users", "name"},
            {"material", "fileID"},
            {"results", "resID"}
    };


    int LoadUserData(const string& token); // by token
    int LoadUserData(const string& name, const string& password);
    int SetUserData(const std::string& table, const std::string& rkey, const string& key = "*");
    int LoadMaterial();

//    map<string, string> m_users;

//    int m_close_log{};

//    char sql_user[100];
//    char sql_passwd[100];
//    char sql_name[100];

};


#endif //MYSERVER_HTTPMVS_H
