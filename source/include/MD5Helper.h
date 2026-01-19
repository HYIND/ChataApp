#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include <memory>
#include "Net/include/Helper/Buffer.h"

struct MD5Context;

class MD5Helper
{
public:
    static std::string computeMD5(const char *data, size_t length);
    static std::string computeMD5(const std::vector<uint8_t> &data);
    static std::string computeMD5(const std::string &str);
    static std::string computeMD5(const Buffer &buf);
    static std::string computeFileMD5(const std::string &filepath);

    static bool verifyMD5(const char *data, size_t length, const std::string &expectedMD5);
    static bool verifyMD5(const std::vector<uint8_t> &data, const std::string &expectedMD5);
    static bool verifyMD5(const std::string &str, const std::string &expectedMD5);
    static bool verifyMD5(const Buffer &buf, const std::string &expectedMD5);
    static bool verifyFileMD5(const std::string &filepath, const std::string &expectedMD5);

    // 分块计算支持
    static std::shared_ptr<MD5Context> MD5Ctx_Init();                                                                 // 获取Context
    static void MD5Ctx_Update(std::shared_ptr<MD5Context> ctx, const unsigned char *data, unsigned long long length); // 向Context填入数据并计算分块
    static std::string MD5Ctx_Final(std::shared_ptr<MD5Context> ctx);                                                 // 完成计算，返回MD5字符串
};