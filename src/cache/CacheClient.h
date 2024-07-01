/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_CACHECLIENT_H
#define MYSERVER_CACHECLIENT_H

#include <iostream>
#include <string>
#include <map>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <chrono>

#include <sw/redis++/redis++.h>
//using namespace sw::redis;

using namespace rapidjson;

class CacheClient {
public:
    //单例模式
    static CacheClient* GetInstance();
    void init(const std::string& url);
    void set(const std::string& key, const std::map<std::string, std::string>& value, std::chrono::seconds ttl);
    std::map<std::string, std::string> get(const std::string& key);
    std::map<std::string, std::string> get(const std::string& key, std::chrono::seconds ttl);

    void del(const std::string& key);
private:
    CacheClient();
    ~CacheClient();
    // 将 C++ map 序列化为 JSON 字符串
    static std::string serializeMapToJson(const std::map<std::string, std::string>& mapData);
    // 将 JSON 字符串反序列化为 C++ map
    static std::map<std::string, std::string> deserializeJsonToMap(const std::string& jsonString);

private:
    sw::redis::Redis* redis_{};


};


#endif //MYSERVER_CACHECLIENT_H
