#pragma once

#include <iostream>
#include <vector>
#include <cstdint>
#include "Buffer.h"

class MD5Helper {
public:
    static std::string computeMD5(const char* data, size_t length);
    static std::string computeMD5(const std::vector<uint8_t>& data);
    static std::string computeMD5(const std::string& str);
    static std::string computeMD5(const Buffer& buf);
    static std::string computeFileMD5(const std::string& filepath);

    static bool verifyMD5(const char* data, size_t length, const std::string& expectedMD5);
    static bool verifyMD5(const std::vector<uint8_t>& data, const std::string& expectedMD5);
    static bool verifyMD5(const std::string& str, const std::string& expectedMD5);
    static bool verifyMD5(const Buffer& buf, const std::string& expectedMD5);
    static bool verifyFileMD5(const std::string& filepath, const std::string& expectedMD5);
};
