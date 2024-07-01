/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_CONFIG_H
#define MYSERVER_CONFIG_H
//namespace cnf {
//#define NAME_UBUNTU_DOCKER_MYSERVER
//
//#if defined(NAME_UBUNTU2_MYSERVER)
//    // 100.107.41.18 WIN工作站虚拟机ubuntu ubuntu2_MyServer
//    const char* ROOT_PATH = "/home/ares/Workspace/MyServer/root/";
//    const char* DATA_PATH = "/home/ares/tmp/";
//    const char* MYSQL_URL = "localhost";
//    const char* MYSQL_USER = "root";
//    const char* MYSQL_PASSWORD = "320510";
//    const char* MYSQL_DATABASENAME = "test";
//    const int MYSQL_PORT = 3306;
//#endif
//
//
//#if defined(NAME_UBUNTU3_MYSERVER)
//    const char* ROOT_PATH = "/home/lighthouse/Workspace/MyServer/root/";
//    const char* DATA_PATH = "/home/lighthouse/tmp/";
//    const char* MYSQL_URL = "localhost";
//    const char* MYSQL_USER = "debian-sys-maint";
//    const char* MYSQL_PASSWORD = "qlAeYLfdUgz2QLRb";
//    const char* MYSQL_DATABASENAME = "test";
//    const int MYSQL_PORT = 3306;
//#endif
//
//#if defined(NAME_UBUNTU_DOCKER_MYSERVER)
//const char* ROOT_PATH = "/home/ares/Workspace/MyServer/root/";
//const char* DATA_PATH = "/home/ares/tmp/";
//const char* MYSQL_URL = "172.17.0.1";
//const char* MYSQL_USER = "root";
//const char* MYSQL_PASSWORD = "320510";
//const char* MYSQL_DATABASENAME = "test";
//const int MYSQL_PORT = 3306;
//#endif
//}

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <map>

std::map<std::string, std::string> parseINI(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;
    std::string section;

    if (!file.is_open()) {
        std::cerr << "Unable to open the file: " << filename << std::endl;
        return config;
    }

    while (getline(file, line)) {
        std::stringstream ss(line);
        std::string key, value;

        // Remove spaces
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        if (line.empty() || line[0] == ';' || line[0] == '#') {
            // Skip empty lines and comments
            continue;
        }

        if (line[0] == '[') {
            // New section
            section = line.substr(1, line.find(']') - 1);
            continue;
        }

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            key = section + "." + line.substr(0, delimiterPos);
            value = line.substr(delimiterPos + 1);
            config[key] = value;
        }
    }

    return config;
}

// Trim leading and trailing whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of('"');
    size_t last = str.find_last_not_of('"');
    return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, (last - first + 1));
}

// Remove surrounding quotes from a string
std::string toString(const std::string& value) {
    std::string trimmed = trim(value);
    return trimmed;
}

// Convert string to integer with error handling
int toInt(const std::string& value) {
    std::string v = trim(value);
    try {
        size_t pos;
        int result = std::stoi(v, &pos);
        if (pos == v.length()) {
            return result;
        }
    } catch (std::exception& e) {
        // handle conversion errors
    }
    return 0;
}

// Convert string to double with error handling
double toDouble(const std::string& value) {
    std::string v = trim(value);
    try {
        size_t pos;
        double result = std::stod(v, &pos);
        if (pos == v.length()) {
            return result;
        }
    } catch (std::exception& e) {
        // handle conversion errors
    }
    return 0.0;
}

std::map<std::string, std::string> loadConfig(char* config_path) {
    auto config = parseINI(config_path);

    for (const auto& kv : config) {
        std::cout << kv.first << " = " << kv.second << std::endl;
    }

    return config;
}

#endif //MYSERVER_CONFIG_H
