//
// Created by ares on 2024/3/20.
//

#ifndef DPMVS_DPMVS5_H
#define DPMVS_DPMVS5_H

#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/stat.h>

class Dpmvs {
public:
    int Do(const std::string& data_path, const std::string& out_path, float race = 1);
    int DoCMVSPMVS(const std::string& data_path, const std::string& out_path);
//    std::string unzip2(const char* zip_file_path) {
//        HZIP hz;
//        hz = OpenZip("c:\\test.zip",0);
//        SetUnzipBaseDir(hz,"c:\\");
//        ZIPENTRY ze;
//        GetZipItem(hz,-1,&ze);
//        int numitems = ze.index;
//        for (int i = 0; i < numitems; i++)
//        {
//            GetZipItem(hz,i,&ze);
//            UnzipItem(hz,i,ze.name);
//        }
//        CloseZip(hz);
//    }


    int Unzip(const char* d,const char* zip_file) {
        char command[1024]; // 假设命令不超过 255 个字符
        strcpy(command, "unzip -o -d ");
        strcat(command, d);
        strcat(command, " ");
        strcat(command, zip_file);

        return std::system(command);
    }
    int Chmode(const char* mod, const char* file_path) {
        char command[1024]; // 假设命令不超过 255 个字符
        strcpy(command, "chmod ");
        strcat(command, mod);
        strcat(command, " ");
        strcat(command, file_path);

        return std::system(command);
    }

private:
    void MakePmvs(const std::string& path);
    void MakeConfig(const std::string& path);

private:
    int frame_num_ = 0;

};

#endif //DPMVS_DPMVS5_H
