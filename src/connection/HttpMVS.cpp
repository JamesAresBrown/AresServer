/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "HttpMVS.h"

#include <fstream>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdarg>
#include <map>
#include <sys/epoll.h>
#include <sys/socket.h>
#include "DPMVS5.h"

void HttpMVS::SetMysqlPool(MysqlPool* mysqlPool) {
    mysqlPool_ = mysqlPool;
}

void HttpMVS::SetCacheClient(CacheClient *cacheClient) {
    cacheClient_ = cacheClient;
}

HttpMVS::HttpMVS(){
//    memset(kad_.name, '\0', 100);
//    memset(kad_.password, '\0', 100);
//    kad_.state = NO_LOGGED;
}


HttpMVS::~HttpMVS() {
    Reset(1);
    Reset(2);
    Reset(3);
//    Reset(4);
    Reset(5);
//    Reset(6);
    Reset(7);
}

void HttpMVS::Init(char* doc_root, char* data_path, Channel* channel) {
    doc_root_ = new char[strlen(doc_root)];
    strcpy(doc_root_,doc_root);
    strcpy(data_path_, data_path);
    channel_ = channel;

    fd_ = channel_->GetFd();
    //    read_buf_ = new char[READ_BUFFER_SIZE];
    read_buf_idx_ = 0;

//    write_buf_ = new char[WRITE_BUFFER_SIZE];
    m_write_idx = 0;

    check_state_ = CHECK_STATE_REQUESTLINE;
    m_method = GET;

//    real_file_ = new char[FILENAME_LEN];

    memset(read_buf_, '\0', READ_BUFFER_SIZE);
    memset(write_buf_, '\0', WRITE_BUFFER_SIZE);
    memset(real_file_, '\0', FILENAME_LEN);

}

//从状态机，用于分析出一行内容
//返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
// 结束后 LINE_OK：   checked_idx_指向下一句的开头
//       LINE_OPEN： checked_idx_指向整个buf的最后一个字节
//       LINE_BAD：  checked_idx_指向发生错误的字符地址
HttpMVS::LINE_STATE HttpMVS::ParseLine() {
    char temp;
    // checked_idx_是扫描到的字节数
    if (checked_idx_ > read_buf_idx_) {
        checked_idx_ = 0;
    }
    for (; checked_idx_ < read_buf_idx_; ++checked_idx_) {
        temp = read_buf_[checked_idx_];
        if (temp == '\r') {
            if ((checked_idx_ + 1) == read_buf_idx_)
                return LINE_OPEN;
            else if (read_buf_[checked_idx_ + 1] == '\n') {
                read_buf_[checked_idx_++] = '\0'; // 把'\r'置'\0'
                read_buf_[checked_idx_++] = '\0'; // 把'\n'置'\0'
                return LINE_OK; // 解析出一行 并 截断
            }
            return LINE_BAD;
        } else if (temp == '\n') {
            if (checked_idx_ > 1 && read_buf_[checked_idx_ - 1] == '\r') {
                read_buf_[checked_idx_ - 1] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

//循环读取客户数据，直到无数据可读或对方关闭连接
//非阻塞ET工作模式下，需要一次性将数据读完
bool HttpMVS::Read() {
    if (read_buf_idx_ >= READ_BUFFER_SIZE) { // 读数据大小大于最大缓冲区
        return false;
    }

    int bytes_read;
    // LT读取数据
    if (0 == TRIGMode_) {
        bytes_read = recv(fd_, read_buf_ + read_buf_idx_, READ_BUFFER_SIZE - read_buf_idx_, 0);
        read_buf_idx_ += bytes_read;
        read_idx_ += bytes_read;
//        cout << "port: " << this->fd_ << "    bytes_read: " << bytes_read << "    read_buf_idx_: " << read_buf_idx_ <<endl;
//        cout << string(read_buf_, read_buf_idx_) << endl;
//        cout << endl;
//        read_buf_idx_ = 0;

        if (bytes_read <= 0) {
            return false;
        }

        return true;
    } else { // ET读数据
        while (true) {
            bytes_read = recv(fd_, read_buf_ + read_buf_idx_, READ_BUFFER_SIZE - read_buf_idx_, 0);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (bytes_read == 0) {
                return false;
            }
            read_buf_idx_ += bytes_read;
        }
        return true;
    }
}

// 解析http请求行
HttpMVS::HTTP_CODE HttpMVS::parse_request_line(char *text) {
    char* c = strpbrk(text, " \t"); // 找到字符集合" \t"，返回第一个匹配到的字符的指针，http报文中用【空格】或者【\t】隔开
    if (nullptr != c) { *c = '\0'; }
    char* method = text;
    if (strcasecmp(method, "GET") == 0) { // 不分大小写的比较
        m_method = GET;
    } else if (strcasecmp(method, "POST") == 0) {
        m_method = POST;
        cgi_ = 1; // 是post则使用CGI
    } else {
        return BAD_REQUEST; // 目前值支持这两种，其他认为是错误http报文
    }

    if (nullptr != c) ++c;
    if (nullptr != c) c += strspn(c, " \t"); // 返回size_t，识别连续空格或制表符，就是用来分割，url_定位到url部分的第一个字符位置
    char* url = c;
    if (!url) {
        return BAD_REQUEST;
    }
    c = strpbrk(c, " \t");
    if (nullptr != c) { *c = '\0'; } // 设置 方法 后的一个字符为\0 截断出方法部分
    url_ = new char[strlen(url)];
    strcpy(url_, url);

    if (nullptr != c) ++c;
    if (nullptr != c) c += strspn(c, " \t"); // 返回size_t，识别连续空格或制表符，就是用来分割，url_定位到url部分的第一个字符位置
    char* version = c;
    if (!version)
        return BAD_REQUEST;
    c = strpbrk(c, " \t"); // version_移动到version部分的后一个字符 注意是否存在！
    if (nullptr != c) { *c = '\0'; }
    version_ = new char[strlen(version)];
    strcpy(version_, version);
    // 至此c已经不安全了！

    // 进一步解析url
    if (strcasecmp(version_, "HTTP/1.1") != 0 && strcasecmp(version_, "HTTP/1.0") != 0) {
        return BAD_REQUEST;
    }
    if (strncasecmp(url_, "http://", 7) == 0) {
        url_ += 7;
        url_ = strchr(url_, '/'); // url_指向了第一个/，也就是路径的开端
    }

    if (strncasecmp(url_, "https://", 8) == 0) {
        url_ += 8;
        url_ = strchr(url_, '/'); // url_指向了第一个/，也就是路径的开端
    }

    if (!url_ || url_[0] != '/') {
        return BAD_REQUEST;
    }

    if (strlen(url_) == 1) { //当url为/时，显示初始界面
        strcat(url_, "do.html");
    }

    check_state_ = CHECK_STATE_HEADER; // 将状态设置为检查 头部
    return NO_REQUEST;
}

//解析http头部
HttpMVS::HTTP_CODE HttpMVS::parse_headers(char *text) {
    if (text[0] == '\0') { // 头部和主体间有一个单独的空行，当识别到这个空行说明头部解析完毕了
        if (0 == head_length_) {
            head_length_ = checked_idx_;
        }
        if (content_length_ != 0) {
            check_state_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            keep_alive_ = true;
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        content_length_ = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        host_ = new char[strlen(text)];
        strcpy(host_, text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        host_ = text;
    } else if (strncasecmp(text, "Authorization:", 14) == 0) {
        text += 14;
        text += strspn(text, " \t");
        token_ = text;
    } else if (strncasecmp(text, "Content-Type:", 13) == 0) {
        text += 13;
        text += strspn(text, " \t");
        if (strncasecmp(text, "multipart/form-data", 19) == 0) {
            text += 19;
            text += strspn(text, "; \t");
            text += strspn(text, "boundary=");
            text += strspn(text, "-");
            // 在数据载入前，确认存储单元初始化，删除旧信息
            Reset(5);

            boundary_= new char[strlen(text)];
            strcpy(boundary_, text);
            // 此时判定有数据需要载入，开启下载模式
            have_file_to_download_ = true;
        } else if (strncasecmp(text, "text/plain", 10) == 0) {

        }
    }
    else {
//        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

std::string HttpMVS::generateUUID() {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd()); // Mersenne Twister 19937-bit
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2); // 注意: 使用 sizeof 时减去 '\0' 的大小
    std::string uuid(16, 0); // 16个0初始化字符串

    for (int i = 0; i < 16; ++i) {
        uuid[i] = charset[dis(gen)];
    }

    return std::move(uuid);
}

//判断http请求是否被完整读入
HttpMVS::HTTP_CODE HttpMVS::parse_content(char *text) {
    // 如果内容中包含大文件，很有可能一次读缓冲是读不完的，必须有另外的处理方法

    if (have_file_to_download_) {
        // 此时 read_buf_ + checked_idx_ 指向头部后的一个空行的第一个字符
        if (data_["users"].empty()) {
            if (0 != LoadUserData(token_)) {
                return FORBIDDEN_REQUEST;
            }
        }

        // 如果数据没有准备好 则进行数据头的判断
        switch (download_status_) {
            case LOAD_NO: { // 解析内容
                // 解析到了边界
                if (nullptr != strstr(text, boundary_)) {
                    // 如果数据大小还没有给定，进入载入 fileSize 头部阶段。
                    if (0 == file_size_) {
                        download_status_ = LOAD_SIZE_HEAD;
                    }
                }
                break;
            }
            case LOAD_SIZE_HEAD: {
                // 识别到了 fileSize 的小头部
                if (nullptr != strstr(text, "fileSize")) {
                // download_status_ = LOAD_SIZE_HEAD;
                }
                if (text[0] == '\0') {
                    // fileSize的小头部结束，准备读取
                    download_status_ = LOAD_SIZE;
                }
                break;
            }
            case LOAD_SIZE: { // 读入数据
                if (0 == file_size_) { // 如果大小还没读取，则读取
                    file_size_ = stol(text);
                    // download_status_ = LOAD_SIZE;
                }
                // 如果读取了，那么识别边界
                if (nullptr != strstr(text, boundary_)) {
                    if (0 < file_size_) { // 正确载入大小，成功读取到边界，准备数据读取
                        download_status_ = LOAD_DATA_HEAD;
                    } else { // 读取到边界，但是大小异常，返回异常
                        return BAD_REQUEST;
                    }
                }
                break;
            }
            case LOAD_DATA_HEAD: {
                if (nullptr != strstr(text, "Content-Disposition:")) {
                    char* filename = strstr(text, "filename=\"");
                    filename += strlen("filename=\"");
                    char* t = strpbrk(filename, "\"");
                    *t = '\0';
                    filename_ = new char[strlen(filename)];
                    strcpy(filename_, filename);
                    break;
                }
                if (nullptr != strstr(text, "Content-Type:")) {
                    break;
                }
                if (text[0] == '\0') {
                    // data 的小头部结束，准备读取数据
                    download_status_ = LOAD_DATA;
                    // 此处没有break！！！！！下滑到 LOAD_DATA
                    // 此处必须接管 ProcessRead() 读取数据
                    cout << "Load Data! name: " << filename_ << " size: " << file_size_ << endl;
                }
            }
            case LOAD_DATA: {
                char *file_buf;
                // 已经在读缓存中的数据长度：
                to_download_idx_ = read_buf_idx_ - checked_idx_;
                file_buf = read_buf_ + start_line_;
//                file_buf = text;
                // 超大文件会导致int型别移除，要小心处理。
                if (file_size_ - download_idx_ < long(READ_BUFFER_SIZE)) {
                    if (to_download_idx_ > file_size_ - download_idx_ && file_size_ - download_idx_ >= 0) {
                        to_download_idx_ = int(file_size_ - download_idx_);
                    }
                }

                checked_idx_ += to_download_idx_;
                fileID_ = fileID_.empty() ? generateUUID() : fileID_;
                load_path_ = string(data_path_) + "material/" + fileID_ + ".zip";
                std::ofstream file(load_path_, std::ios::app | std::ios::binary);
                if (!file.is_open()) {
                    return INTERNAL_ERROR;
                }
                file.write(file_buf, to_download_idx_);
                file.close();
                download_idx_ += to_download_idx_;

                // 渴望继续读取
                if (download_idx_ < file_size_) {
                    if (read_buf_idx_ >= READ_BUFFER_SIZE) {
                        read_buf_idx_ = 0;
                        checked_idx_ = 0;
                        memset(read_buf_, '\0', READ_BUFFER_SIZE);
                    }
                    return NO_REQUEST;
                } else { // 已经读完
                    download_status_ = LOAD_DATA_DONE;
                    reread_times_ = 0;
                }
                break;
            }
            case LOAD_DATA_DONE: { // 负责读完后的收尾工作
                // !!!!对于边界，除了第一次出现的边界，后面的边界前后都带有/r/n！！！！！
                // 此处还会读取一行 要么是完整的后边界， 要么是不完整的后边界
                // 此处要判别是否读完了边界，残余的边界会留存在读缓存中, 必须把残余的边界读出来
                // 已经包括最后一个边界读取完了
                // ！！！！不知道为什么有时候总是差那几个比特，但是数据是读完的。
                if (read_idx_ - head_length_ >= content_length_ || reread_times_ > 100) {
                    // check和当前的read_buf_idx_对齐
                    checked_idx_ = read_buf_idx_; // 此时 check_idx 已经指向了\n后的一个字符
                    // 如果刚好读完了这个数据包，目前 check_idx 就只向了一个未定义的字符，也就是未来会来的第一个字符
                    download_status_ = LOAD_DONE;
//                    ClearReadBuf(3);
                    reread_times_ = 0;
                } else { // 还要把最后一个边界读出来
                    checked_idx_ = read_buf_idx_;
                    ++reread_times_;
                }
                break;
            }
            default:
                break;
        }

        if (LOAD_DONE == download_status_) {
//            std::string uuid = generateUUID();
//            MYSQL *mysql = nullptr;
//            connectionRAII mysqlConn(&mysql, mysqlPool_);
//            QueryRAII mysql_query(mysql,
////                                  "SET @new_fileID = '%s';"
////                                  "SET @new_user = '%s';"
////                                  "SET @new_address = '%s';"
////                                  "START TRANSACTION;"
////                                  "DELETE FROM material WHERE user = @new_user;"
////                                  "INSERT INTO material (fileID, `user`, address) VALUES (@new_fileID, @new_user, @new_address);"
////                                  "COMMIT;"
//                                  "INSERT INTO material(fileID, `user`, address) VALUES('%s', '%s', '%s');"
//                                  ,
//                                  fileID_.c_str(), data_["users"]["name"].c_str(), load_path_.c_str());
//            if (mysql_query.ret_ == 0)
//                return GET_REQUEST;
            string key = "users:" + data_["users"]["name"] + ":"+ data_["users"]["password"];
            std::map<string, string> userDataMap = cacheClient_->get(key, std::chrono::seconds(18000));

            if (!userDataMap.empty()) {
                userDataMap["fileID"] = fileID_;
                userDataMap["load_path"] = load_path_;
                cacheClient_->set(
                        key,
                        userDataMap,
                        std::chrono::seconds(18000)
                );
                return GET_REQUEST;
            }
        }

        return NO_REQUEST;
    }

    // 加入所有报文的长度都小于读缓冲，那么这种判别就认为已经读到了所有数据
    if (read_buf_idx_ >= (content_length_ + checked_idx_)) {
        text[content_length_] = '\0';
        //POST请求中最后为输入的用户名和密码
        string_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

HttpMVS::HTTP_CODE HttpMVS::ProcessRead() {
    LINE_STATE line_status = LINE_OK;
    HTTP_CODE ret;
    char *text;
    // 循环解析 check_state_在初始化时候的初始化为解析请求行
    // 进行处理的前提：
    // 1：当前行状态为提取完整的 并且 解析状态在解析内如
    // 2：尝试解析出一行，并且解析成功了
    // 当进入读内容部分 ParseLine() 实际上就不工作了
    while ((check_state_ == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = ParseLine()) == LINE_OK))
    {
        text = get_line();
        start_line_ = checked_idx_; // 将开始指针与扫描指针对齐，实际意义就是这一次ParseLine()的完了

//        LOG_INFO("%s", text);
        switch (check_state_) {
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = parse_headers(text); // 这里当返回 NO_REQUEST 表示进入解析内容部分
                if (ret == GET_REQUEST) // 解析完头部就可以看是否有请求，是否需要回应
                    return DoRequest();
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = parse_content(text); // 当进入读取内容的部分，该函数检查目前得到读取缓冲区数据是否已经达到内容的长度，如果没有，则继续读
                if (ret == GET_REQUEST)
                    return DoRequest();
                line_status = LINE_OPEN; // 这里有些问题，当解析内容的时候，解析状态不是 GET_REQUEST 就继续去读直到读完为止，这里把
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST; // 只要返回这个，就会一直试着读取。
}



HttpMVS::HTTP_CODE HttpMVS::DoRequest() {

    strcpy(real_file_, doc_root_);
    int len;
    len = strlen(doc_root_);
    //printf("url_:%s\n", url_);
    const char *p = strrchr(url_, '/');

//    printf("client: %s state: %d op: %s\n", kad_.name, kad_.state, url_);
    //处理登录cgi_
    if (cgi_ == 1 && (*(p + 1) == '2' || *(p + 1) == '3')) {
        //根据标志判断是登录检测还是注册检测

        char *url__real = (char *)malloc(sizeof(char) * 200);
        strcpy(url__real, "/");
        strcat(url__real, url_ + 2);
        strncpy(real_file_ + len, url__real, FILENAME_LEN - len - 1);
        free(url__real);

        //将用户名和密码提取出来
        //user=123&passwd=123
        char name[100], password[100];
        int i;
        for (i = 5; string_[i] != '&'; ++i)
            name[i - 5] = string_[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; string_[i] != '\0'; ++i, ++j)
            password[j] = string_[i];
        password[j] = '\0';

//        kad_.state = NO_LOGGED;

        switch (*(p + 1)) {
            case '2': {
                Reset(7);
                if (0 == LoadUserData(string(name), string(password))) {
                    strcpy(url_, "/login_right.txt");
                } else {
//                    kad_.state = NO_LOGGED;
                    strcpy(url_, "/login_error.txt");
                }
                break;
            }
            case '3': {
                //如果是注册，先检测数据库中是否有重名的
                //没有重名的，进行增加数据
                MYSQL* mysql = nullptr;
                connectionRAII mysqlConn(&mysql, mysqlPool_);
                QueryRAII mysql_query(mysql, "SELECT * FROM users WHERE user = '%s' AND password = '%s';", name, password);
                if (mysql_query.ret_ == 0 && mysql_query.res_->row_count == 0) {
                    // int res;
                    // res = mysql_query(mysql, query);
                    QueryRAII mysql_insert(mysql, "INSERT INTO users(user, password) VALUES('%s', '%s');", name, password);
                    if (mysql_insert.ret_ == 0)
                        strcpy(url_, "/register_right.txt");
                    else
                        strcpy(url_, "/register_error.txt");
                } else {
                    strcpy(url_, "/register_error.txt");
                }
                break;
            }
            default: {
                break;
            }
        }
    }



    switch (*(p + 1)) {
        case '5': {
            char *url__real = (char *)malloc(sizeof(char) * 200);
            strcpy(url__real, "/picture.html");
            strncpy(real_file_ + len, url__real, strlen(url__real));

            free(url__real);
            break;
        }
        case '7': {
            if (data_["users"].empty()) {
                return FORBIDDEN_REQUEST;
            }
            if (have_file_to_download_) {
                char *url__real = (char *)malloc(sizeof(char) * 200);
                strcpy(url__real, "/upload_right.txt");
                strncpy(real_file_ + len, url__real, strlen(url__real));
                free(url__real);
            } else {
                char *url__real = (char *)malloc(sizeof(char) * 200);
                strcpy(url__real, "/upload_error.txt");
                strncpy(real_file_ + len, url__real, strlen(url__real));
                free(url__real);
            }
            // 复位文件读取区域
            delete[] boundary_; boundary_ = nullptr;
            delete[] filename_; filename_ = nullptr;
            file_size_ = 0;
            to_download_idx_ = 0;
            download_idx_ = 0;
            download_status_ = LOAD_NO;
            have_file_to_download_ = false;
            load_path_ = "";
            fileID_ = "";
            break;
        }
        case '9': {
            if (data_["users"].empty()) {
                if (0 != LoadUserData(token_))
                    return FORBIDDEN_REQUEST;
            }

            int process_state_ = 0;
            string key = "users:" + data_["users"]["name"] + ":"+ data_["users"]["password"];
            std::map<string, string> userDataMap = cacheClient_->get(key, std::chrono::seconds(18000));
            if (userDataMap.count("fileID") == 0 || userDataMap.count("load_path") == 0) {
                return INTERNAL_ERROR;
            } else {
                string fileID = userDataMap["fileID"];
                string load_path = userDataMap["load_path"];
                string unzip_path = string(data_path_) + "result/" + fileID + "/";
                string out_path = string(data_path_) + "result/" + fileID + ".ply";
                Dpmvs dpmvs;
                if (0 == dpmvs.Unzip(unzip_path.c_str(), load_path.c_str())) {
                    dpmvs.DoCMVSPMVS(unzip_path, out_path);
//                    dpmvs.Chmode(string("777").c_str(), out_path);
                    // 处理成功
                    process_state_ = 1;
                }
                if (1 == process_state_) {
                    strncpy(real_file_, out_path.c_str(), strlen(out_path.c_str()));
                    real_file_[strlen(out_path.c_str())] = '\0';
                } else {
                    return INTERNAL_ERROR;
                }
            }
            break;
        }
        default: {
            strncpy(real_file_ + len, url_, FILENAME_LEN - len - 1);
            break;
        }
    }

    if (stat(real_file_, &m_file_stat) < 0)
        return NO_RESOURCE;

    if (!(m_file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    if (S_ISDIR(m_file_stat.st_mode))
        return BAD_REQUEST;

    int fd = open(real_file_, O_RDONLY);

    file_address_ = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}
void HttpMVS::unmap() {
    if (file_address_) {
        munmap(file_address_, m_file_stat.st_size);
        file_address_ = nullptr;
    }
}

bool HttpMVS::Write()
{
    if (bytes_to_send == 0) { // 写缓冲没有可写数据，需要把监听状态切换为 听
        channel_->ResetListenEvents(EPOLLIN | EPOLLPRI);
        return true;
    }

    while (true) {
//        string buf0 = string((char*)m_iv[0].iov_base, m_iv[0].iov_len);
//        string buf1 = string((char*)m_iv[1].iov_base, m_iv[1].iov_len);
//        string s = buf0 + buf1;
//        cout << s << endl;
//        string buf2 = string((char*)write_buf_);
////        add_content(file_address_);
//        int temp = ::write(sockfd, s.c_str(), sizeof(char) * s.size());
        int temp = writev(fd_, m_iv, m_iv_count);
        // 这里有一个坑，如果采用非阻塞模式，经过实验，发送一次数据后缓冲区就满了。然后temp会应为缓冲区满而返回-1
        if (temp < 0) {
            if (errno == EAGAIN) { // 写操作阻塞 监听事假还是为写
                return true;
            } else{
                printf("write error connect over: %d\n", errno);
                unmap();
                return false;
            }
        }
        float r = (long double)(bytes_have_send) / (long double)(bytes_to_send+bytes_have_send);
        printf("%.2f\r", r);

        bytes_have_send += temp;
        bytes_to_send -= temp;
        if (bytes_have_send >= m_iv[0].iov_len) {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = file_address_ + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        } else {
            m_iv[0].iov_base = write_buf_ + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

        if (bytes_to_send <= 0) { // 写完毕 时间类型改为 听
            // 这里有一个致命的臭虫，
            // 写数据完了以后需要重新建立听连接，
            // 但是这次写可能是写错误或者说不管是不写错误，
            // 重点是读缓冲区中有脏掉的数据，如果不把他全部清理掉，可能会影响下一次连接
//            ClearReadBuf(0);
//            ClearWriteBuf(0);

            channel_->ResetListenEvents(EPOLLIN | EPOLLPRI);
            // http连接需要初始化
            unmap();
            // 长连接需要继续交互
            if (keep_alive_) {
                Reset(1);
                Reset(2); // 重置写
                Reset(3); // 重置连接信息
                Reset(4); // 重置数据渴望
                memset(real_file_, '\0', FILENAME_LEN);
                // 当你写完一个文件，可能还要继续写，所以长连接设置不能变
                keep_alive_ = false;
                // 这真是个恶心的臭虫，在写完后，会把连接信息全部质控，要是后面有参与的读信息进入
                // 会直接析构连接
//                ClearReadBuf(3);
                return true;
            } else {
                // 非长连接而退出，会引发析构
//                ClearWriteBuf(3);
                return false;
            }
        }
    }
}
bool HttpMVS::add_response(const char *format, ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(write_buf_ + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);

//    LOG_INFO("request:%s", write_buf_);

    return true;
}
// 响应报文 状态行
bool HttpMVS::add_status_line(int status, const char *title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}
// 响应报文 相应头部
bool HttpMVS::add_headers(int content_len) {
    add_token();
//    add_content_type();

    return add_content_length(content_len) && add_linger() &&
           add_blank_line();
}
// 响应报文 相应头部 Content-Length
bool HttpMVS::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}
// 响应报文 相应头部 Content-Type
bool HttpMVS::add_content_type(const char* content_type) {
    return add_response("Content-Type:%s\r\n", content_type);
}
// 响应报文 相应头部 Connection
bool HttpMVS::add_linger() {
    return add_response("Connection:%s\r\n",keep_alive_ ? "keep-alive" : "close");
}
bool HttpMVS::add_blank_line() {
    return add_response("%s", "\r\n");
}
// 响应报文 响应体
bool HttpMVS::add_content(const char *content) {
    return add_response("%s", content);
}
// 响应报文 响应体
bool HttpMVS::add_token() {
    if (!token_.empty())
        return add_response("Authorization:%s\r\n", token_.c_str());
    else
        return true;
}
// 生成指定长度的随机字符串
std::string HttpMVS::generateToken(int length) {
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    std::string token = std::string(length, '0');

    srand(time(NULL));

    for (int i = 0; i < length; ++i) {
        token[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

//    token[length] = '\0'; // 添加字符串结束符

    return token;
}
HttpMVS::PROCESS_WRITE_STATE HttpMVS::ProcessWrite(HTTP_CODE ret) {
    switch (ret) {
        case INTERNAL_ERROR: {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form))
                return NO_WRITE;
            break;
        }
        case BAD_REQUEST: {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form))
                return NO_WRITE;
            break;
        }
        case FORBIDDEN_REQUEST: {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form))
                return NO_WRITE;
            break;
        }
        case FILE_REQUEST: {
            add_status_line(200, ok_200_title);
            // 有文件要写入
            if (m_file_stat.st_size != 0) {
                add_headers(m_file_stat.st_size);
                // 结构体第一部分需要记录状态行和响应头部
                m_iv[0].iov_base = write_buf_;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = file_address_;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                // 这个赋值非常重要，应为用到了mmap，文件的真实长度被记录在了 m_file_stat 中
                bytes_to_send = m_write_idx + m_file_stat.st_size;
                return TO_WRITE;
            } else { // 没有文件要写入
                const char *ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if (!add_content(ok_string))
                    return NO_WRITE;
            }
        }
        default:
            return PROCESS_WRITE_ERROR;
    }
    m_iv[0].iov_base = write_buf_;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return TO_WRITE;
}
bool HttpMVS::Process() {
    HTTP_CODE read_ret = ProcessRead();
    if (read_ret == NO_REQUEST) {
        channel_->ResetListenEvents(EPOLLIN | EPOLLPRI);
        return true;
    }
    // 这个返回值非常关键，如果返回false，无法进入到读阶段
    // 返回false有两种情况
    // 如果没有内容要发送，自然不用进入到write阶段
    // 有异常，无法写入
    auto write_ret = ProcessWrite(read_ret);
    switch (write_ret) {
        case TO_WRITE: {
            channel_->ResetListenEvents(EPOLLOUT| EPOLLRDHUP);
            break;
        }
        case NO_WRITE: {
            // 等待下一次响应要重置读缓冲区
            // 重置读缓冲
//            Reset(1);
//            Reset(2);
            Reset(3);
            Reset(4);
            memset(real_file_, '\0', FILENAME_LEN);
            channel_->ResetListenEvents(EPOLLIN | EPOLLPRI);
            break;
        }
        case PROCESS_WRITE_ERROR: {
            return false;
        }
    }
    return true;
}

//void HttpMVS::ClearReadBuf(int sec) { // 清空读缓冲，读区重置
//    printf("clear read buffer...\n");
//    auto begin = std::chrono::system_clock::now();
//    while (true) {
//        ssize_t bytes_read = recv(fd_, read_buf_, READ_BUFFER_SIZE, 0);
//        auto end = std::chrono::system_clock::now();
//        if (std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() > sec) {
//            if (bytes_read == -1) {
//                if (errno == EAGAIN || errno == EWOULDBLOCK)
//                    break;
//            } else if (bytes_read == 0) {
//                break;
//            }
//        }
//    }
//    Reset(1);
//}


//void HttpMVS::ClearWriteBuf(int sec) {
//    printf("clear write buffer...\n");
//    auto begin = std::chrono::system_clock::now();
//    while (true) {
////        string buf0 = string((char*)m_iv[0].iov_base, m_iv[0].iov_len);
////        string buf1 = string((char*)m_iv[1].iov_base, m_iv[1].iov_len);
////        string s = buf0 + buf1;
////        cout << "dirty write: " << s << endl;
////        int temp = ::write(sockfd, s.c_str(), sizeof(char) * s.size());
//        int temp = writev(fd_, m_iv, m_iv_count);
//        auto end = std::chrono::system_clock::now();
//        if (std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() > sec) {
//            if (temp <= 0) {
//                Reset(2);
//                unmap();
//                return;
//            }
//        }
//    }
//}
// 在长连接环境中，小心使用！
void HttpMVS::Reset(int cho) {
    // reset read
    if (1 == cho) {
//        if(false) ClearReadBuf(3);
        memset(read_buf_, '\0', READ_BUFFER_SIZE);
        read_buf_idx_ = 0;
        checked_idx_ = 0;
        start_line_ = 0;
    }
    // reset write
    if (2 == cho) {
        memset(write_buf_, '\0', WRITE_BUFFER_SIZE);
        m_write_idx = 0;
        bytes_to_send = 0;
        bytes_have_send = 0;
    }
    // reset connect
    if (3 == cho) {
        // 重置连接信息
        check_state_ = CHECK_STATE_REQUESTLINE;
        m_method = GET;
        delete[] url_; url_ = nullptr;
        delete[] version_; version_ = nullptr;
        delete[] host_; host_ = nullptr;
        read_idx_ = 0;
        head_length_ = 0;
        content_length_ = 0;
        // 长连接标志在重置的时候只有写可以改变
//        keep_alive_ = false;
//        memset(real_file_, '\0', FILENAME_LEN);
        delete[] file_address_; file_address_ = nullptr;
        m_file_stat = {};
        // m_iv[2] = {};
        m_iv_count = 0;
        cgi_ = 0;
        string_ = nullptr;
    }
    // 重置接收文件区，只是接收文件，并不是抹除之前的接收信息
    if (4 == cho) {
        // 由于是长连接，用户已经上传过文件后，本次连接要始终能察觉到
        // 重设文件下载
        //delete[] boundary_; boundary_ = nullptr;
        //delete[] filename_; filename_ = nullptr;
        //file_size_ = 0;
        //to_download_idx_ = 0;
        //download_idx_ = 0;
        //download_status_ = LOAD_NO;
        // 不排斥接纳新的文件
        have_file_to_download_ = false;
    }
    // 重置接收文件区，抹除之前的接收信息
    if (5 == cho) {
        delete[] boundary_; boundary_ = nullptr;
        delete[] filename_; filename_ = nullptr;
        file_size_ = 0;
        to_download_idx_ = 0;
        download_idx_ = 0;
        download_status_ = LOAD_NO;
        have_file_to_download_ = false;
        load_path_ = "";
        fileID_ = "";
    }
    // 重置处理状态
//    if (6 == cho) {
//        process_state_ = -1;
//    }
    // 重置用户信息
    if (7 == cho) {
//        memset(kad_.name, '\0', 100);
//        memset(kad_.password, '\0', 100);
//        kad_.state = NO_LOGGED;
//        Reset(1);
        Reset(2);
        Reset(5);
    }

}

int HttpMVS::LoadUserData(const string& token) {
    if (!token.empty()) {
        // 根据token查询用户名
        map<string, string> m = cacheClient_->get("token:" + token, std::chrono::seconds(18000));
        if (!m.empty()) {
            data_["users"] = m;
//            std::string name = m.at("name");
//            data_["users"] = cacheClient_->get("users:" + name, std::chrono::seconds(18000));
            cout << "do cache !" << endl;
            if (!data_["users"].empty())
                return 0;
        }
    }
    return -1;
}

int HttpMVS::LoadUserData(const string& name, const string& password) {
    std::string token;
    // 首先通过 用户:密码 查询是否有信息
    std::map<string, string> map = cacheClient_->get("users:" + name + ":"+ password, std::chrono::seconds(18000));
    if (map.empty()) { // 如果没有查到，到数据库寻找，并建立token
        MYSQL* mysql = nullptr;
        connectionRAII mysqlConn(&mysql, mysqlPool_);
        QueryRAII mysql_query(mysql, "SELECT * FROM users WHERE user = '%s' AND password = '%s';", name.c_str(), password.c_str());

        if (mysql_query.ret_ == 0 && mysql_query.res_->row_count == 1) {
            // 获取token
            token = generateToken(8);
            cacheClient_->set(
                    "users:" + name + ":"+ password,
                    std::map<string, string>{{"token", token}},
                    std::chrono::seconds(18000)
            );
            cacheClient_->set(
                    "token:" + token,
                    std::map<string, string>{{"name", name}, {"password", password}},
                    std::chrono::seconds(18000)
            );
//            cacheClient_->set("users:" + name, data, std::chrono::seconds(18000));

            token_ = token;
            data_["users"] = std::map<string, string>{{"name", name}, {"password", password}};
            return 0;
        } else {
            return -1;
        }
    } else {
        token = map.at("token").data();
        return LoadUserData(token);
    }

}

int HttpMVS::SetUserData(const string &table, const string &rkey, const string& key) {
    if (data_[table].empty())
        return -1;
    MYSQL* mysql = nullptr;
    connectionRAII mysqlConn(&mysql, mysqlPool_);
    if ("*" == key) {
        for (auto kv : data_.at(table)) {
            QueryRAII mysql_query(mysql, "UPDATE %s \n"
                                         "SET %s = %s \n"
                                         "WHERE %s = %s;",
                                         table.c_str(),
                                         kv.first.c_str(), kv.second.c_str(),
                                         PRIMARY_KEY[table].c_str(), rkey.c_str()
                                         );
        }
    } else {
        QueryRAII mysql_query(mysql, "UPDATE %s \n"
                                     "SET %s = %s \n"
                                     "WHERE %s = %s;",
                              table.c_str(),
                              key.c_str(), data_[table][key].c_str(),
                              PRIMARY_KEY[table].c_str(), rkey.c_str()
        );
    }
    cacheClient_->del(table + ":" + rkey);
    return 0;
}

int HttpMVS::LoadMaterial() {
    if (data_["users"].empty())
        return -1;
    MYSQL* mysql = nullptr;
    connectionRAII mysqlConn(&mysql, mysqlPool_);
    QueryRAII mysql_query(mysql, "SELECT * FROM material WHERE user = '%s';", data_["users"]["name"].c_str());
    if (mysql_query.res_->row_count > 0) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(mysql_query.res_))) {
            // 提取每一行的数据
            data_["material"][row[0]] = row[2];
        }
        return data_["material"].size();
    }
    return -1;
}







