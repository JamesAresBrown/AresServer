/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#include "CacheClient.h"

CacheClient::CacheClient() = default;
void CacheClient::init(const std::string &url) {
    try {
        redis_ = new sw::redis::Redis(url);
    } catch (...) {
        std::cout << "exception" << std::endl;
    }
}

CacheClient::~CacheClient() = default;

CacheClient* CacheClient::GetInstance() {
    static CacheClient cacheClient;
    return &cacheClient;
}

std::string CacheClient::serializeMapToJson(const std::map<std::string, std::string>& mapData) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);

    // 创建 JSON 对象
    writer.StartObject();

    for (const auto& pair : mapData) {
        // 添加键值对到 JSON 对象
        writer.Key(pair.first.c_str());
        writer.String(pair.second.c_str());
    }

    // 结束 JSON 对象
    writer.EndObject();

    // 返回序列化后的 JSON 字符串
    return buffer.GetString();
}

// 将 JSON 字符串反序列化为 C++ map
std::map<std::string, std::string> CacheClient::deserializeJsonToMap(const std::string& jsonString) {
    std::map<std::string, std::string> resultMap;

    Document document;
    document.Parse(jsonString.c_str());

    if (document.IsObject()) {
        for (Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
            if (itr->value.IsString()) {
                resultMap[itr->name.GetString()] = itr->value.GetString();
            }
        }
    }

    return resultMap;
}

void CacheClient::set(const std::string &key, const std::map<std::string, std::string>& value, std::chrono::seconds ttl) {
    std::string s = serializeMapToJson(value);
    redis_->setex(key, std::chrono::seconds(ttl), serializeMapToJson(value));
}
std::map<std::string, std::string> CacheClient::get(const std::string &key) {
    auto value = redis_->get(key);
    if (value) {// 重置 TTL 为 300 秒
        std::string valueStr = *value;
        return deserializeJsonToMap(valueStr);
    } else {
        return {};
    }
}
std::map<std::string, std::string> CacheClient::get(const std::string &key, std::chrono::seconds ttl) {
    auto m = get(key);
    if (!m.empty()) {
        redis_->expire(key, ttl);
    }
    return m;
}

void CacheClient::del(const std::string &key) {
    redis_->del(key);
}
