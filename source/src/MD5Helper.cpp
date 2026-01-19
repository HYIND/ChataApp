#include "MD5Helper.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include "FileIOHandler.h"

constexpr uint64_t MAX_CLIP_SIZE = 5 * 1024 * 1024;

#define MD5_F(x, y, z) ((x & y) | (~x & z))
#define MD5_G(x, y, z) ((x & z) | (y & ~z))
#define MD5_H(x, y, z) (x ^ y ^ z)
#define MD5_I(x, y, z) (y ^ (x | ~z))

namespace MD5Calculate
{
    // MD5数据块大小
    const int MD5_BLOCK_SIZE = 64;
    const int MD5_DIGEST_SIZE = 16;

    static const unsigned int T[64] = {
        0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE, 0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
        0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE, 0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
        0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA, 0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
        0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED, 0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
        0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C, 0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
        0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05, 0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
        0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039, 0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
        0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1, 0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391};

    // 部分计算
    // a=b+((a+(G(b,c,d)+m+sin)<<< s)
    //(当前传入的s为位移数，x为公式中  a+(G(b,c,d)+m+sin  )
    void MD5_Value(unsigned int *a, unsigned int b, unsigned int x, unsigned int s)
    {
        //((x << s) | (x >> (32 - s))):4字节数据x 向左循环位移s位（从左侧移出去的位从右侧重新导入）
        *a = b + ((x << s) | (x >> (32 - s)));
    }

    void processBlock(unsigned int *a, unsigned int *b, unsigned int *c, unsigned int *d, const unsigned char *block)
    {
        // 记录abcd的初始值
        unsigned int A = *a;
        unsigned int B = *b;
        unsigned int C = *c;
        unsigned int D = *d;

        unsigned int count = 0;

        unsigned int *m = (unsigned int *)block;
        // 4字节数据索引
        unsigned int ptr = 0;

        // 16轮ff运算
        for (int i = 0; i < 4; i++)
        {
            MD5_Value(a, *b, *a + MD5_F(*b, *c, *d) + m[ptr++] + T[count++], 7);
            MD5_Value(d, *a, *d + MD5_F(*a, *b, *c) + m[ptr++] + T[count++], 12);
            MD5_Value(c, *d, *c + MD5_F(*d, *a, *b) + m[ptr++] + T[count++], 17);
            MD5_Value(b, *c, *b + MD5_F(*c, *d, *a) + m[ptr++] + T[count++], 22);
        }

        // 16轮GG运算
        ptr = 12;
        for (int i = 0; i < 4; i++)
        {
            ptr = (ptr + 5) % 16;
            MD5_Value(a, *b, *a + MD5_G(*b, *c, *d) + m[ptr] + T[count++], 5);
            ptr = (ptr + 5) % 16;
            MD5_Value(d, *a, *d + MD5_G(*a, *b, *c) + m[ptr] + T[count++], 9);
            ptr = (ptr + 5) % 16;
            MD5_Value(c, *d, *c + MD5_G(*d, *a, *b) + m[ptr] + T[count++], 14);
            ptr = (ptr + 5) % 16;
            MD5_Value(b, *c, *b + MD5_G(*c, *d, *a) + m[ptr] + T[count++], 20);
        }

        // 16轮HH运算
        ptr = 2;
        for (int i = 0; i < 4; i++)
        {
            ptr = (ptr + 3) % 16;
            MD5_Value(a, *b, *a + MD5_H(*b, *c, *d) + m[ptr] + T[count++], 4);
            ptr = (ptr + 3) % 16;
            MD5_Value(d, *a, *d + MD5_H(*a, *b, *c) + m[ptr] + T[count++], 11);
            ptr = (ptr + 3) % 16;
            MD5_Value(c, *d, *c + MD5_H(*d, *a, *b) + m[ptr] + T[count++], 16);
            ptr = (ptr + 3) % 16;
            MD5_Value(b, *c, *b + MD5_H(*c, *d, *a) + m[ptr] + T[count++], 23);
        }

        // 16论II计算
        ptr = 9;
        for (int i = 0; i < 4; i++)
        {
            ptr = (ptr + 7) % 16;
            MD5_Value(a, *b, *a + MD5_I(*b, *c, *d) + m[ptr] + T[count++], 6);
            ptr = (ptr + 7) % 16;
            MD5_Value(d, *a, *d + MD5_I(*a, *b, *c) + m[ptr] + T[count++], 10);
            ptr = (ptr + 7) % 16;
            MD5_Value(c, *d, *c + MD5_I(*d, *a, *b) + m[ptr] + T[count++], 15);
            ptr = (ptr + 7) % 16;
            MD5_Value(b, *c, *b + MD5_I(*c, *d, *a) + m[ptr] + T[count++], 21);
        }

        // 将abcd数据加上原始值
        *a += A;
        *b += B;
        *c += C;
        *d += D;
    }

    // 计算MD5
    void computeMD5(const unsigned char *message, unsigned long long messageLength, unsigned char *hash)
    {
        if (!message)
            messageLength = 0;

        // 初始化
        unsigned int A = 0x67452301;
        unsigned int B = 0xefcdab89;
        unsigned int C = 0x98badcfe;
        unsigned int D = 0x10325476;

        for (unsigned long long index = 0; index < messageLength / MD5_BLOCK_SIZE; index++)
            processBlock(&A, &B, &C, &D, message + index * MD5_BLOCK_SIZE);

        unsigned long long lastMessageLength = messageLength % MD5_BLOCK_SIZE;

        unsigned long long paddinglastMessageLength = 0;
        if (MD5_BLOCK_SIZE - lastMessageLength >= 9)
            paddinglastMessageLength = MD5_BLOCK_SIZE;
        else
            paddinglastMessageLength = 2 * MD5_BLOCK_SIZE;

        unsigned char *paddinglastMessage = new unsigned char[paddinglastMessageLength];
        uint32_t offset = 0;

        if (lastMessageLength > 0)
        {
            memcpy(paddinglastMessage, message + (messageLength - lastMessageLength), lastMessageLength);
            offset += lastMessageLength;
        }

        paddinglastMessage[lastMessageLength] = 0x80;
        offset += 1;

        memset(paddinglastMessage + offset, 0, paddinglastMessageLength - (lastMessageLength + 9));
        offset += paddinglastMessageLength - (lastMessageLength + 9);

        unsigned long long bitLength = messageLength * 8;
        memcpy(paddinglastMessage + offset, &bitLength, 8);
        offset += 8;

        assert(offset == MD5_BLOCK_SIZE || offset == 2 * MD5_BLOCK_SIZE);

        processBlock(&A, &B, &C, &D, paddinglastMessage);

        if (offset == 2 * MD5_BLOCK_SIZE)
            processBlock(&A, &B, &C, &D, paddinglastMessage + MD5_BLOCK_SIZE);

        delete[] paddinglastMessage;

        // 输出结果
        memcpy(hash, &A, 4);
        memcpy(hash + 4, &B, 4);
        memcpy(hash + 8, &C, 4);
        memcpy(hash + 12, &D, 4);
    }

    // 将MD5哈希值转换为16进制字符串
    std::string toHexString(unsigned char *hash)
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        for (int i = 0; i < MD5_DIGEST_SIZE; i++)
        {
            ss << std::setw(2) << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

    std::string computeMD5String(const unsigned char *data, size_t length)
    {
        unsigned char hash[MD5_DIGEST_SIZE];
        computeMD5(data, length, hash);
        return toHexString(hash);
    }
}

MD5Context::MD5Context()
{
    status[0] = 0x67452301;
    status[1] = 0xefcdab89;
    status[2] = 0x98badcfe;
    status[3] = 0x10325476;
    count = 0;
}

std::string MD5Helper::computeMD5(const char *data, size_t length)
{
    return MD5Calculate::computeMD5String(reinterpret_cast<const unsigned char *>(data), length);
}

std::string MD5Helper::computeMD5(const std::vector<uint8_t> &data)
{
    return MD5Calculate::computeMD5String(reinterpret_cast<const unsigned char *>(data.data()), data.size());
}

std::string MD5Helper::computeMD5(const std::string &str)
{
    return MD5Calculate::computeMD5String(reinterpret_cast<const unsigned char *>(str.c_str()), str.length());
}

std::string MD5Helper::computeMD5(const Buffer &buf)
{
    return MD5Calculate::computeMD5String(reinterpret_cast<const unsigned char *>(buf.Byte()), buf.Length());
}

std::string MD5Helper::computeFileMD5(const std::string &filepath)
{
    auto ctx = MD5Ctx_Init();

    FileIOHandler handle(filepath, FileIOHandler::OpenMode::READ_ONLY);
    uint64_t totalsize = handle.GetSize();
    uint64_t remainsize = totalsize;
    while (remainsize > 0)
    {
        uint64_t readsize = std::min(remainsize, MAX_CLIP_SIZE);
        Buffer buf;
        handle.Read(buf, readsize);
        MD5Ctx_Update(ctx, reinterpret_cast<const unsigned char *>(buf.Byte()), buf.Length());
        remainsize -= readsize;
    }
    return MD5Ctx_Final(ctx);
}

bool MD5Helper::verifyMD5(const char *data, size_t length, const std::string &expectedMD5)
{
    return expectedMD5 == computeMD5(data, length);
}

bool MD5Helper::verifyMD5(const std::vector<uint8_t> &data, const std::string &expectedMD5)
{
    return expectedMD5 == computeMD5(data);
}

bool MD5Helper::verifyMD5(const std::string &str, const std::string &expectedMD5)
{
    return expectedMD5 == computeMD5(str);
}

bool MD5Helper::verifyMD5(const Buffer &buf, const std::string &expectedMD5)
{
    return expectedMD5 == computeMD5(buf);
}

bool MD5Helper::verifyFileMD5(const std::string &filepath, const std::string &expectedMD5)
{
    return expectedMD5 == computeFileMD5(filepath);
}

std::shared_ptr<MD5Context> MD5Helper::MD5Ctx_Init()
{
    return std::make_shared<MD5Context>();
}

void MD5Helper::MD5Ctx_Update(std::shared_ptr<MD5Context> ctx, const unsigned char *data, unsigned long long length)
{
    if (data && length > 0)
    {
        ctx->cacheBuffer.Seek(ctx->cacheBuffer.Length());
        ctx->cacheBuffer.Write(data, length);
        ctx->cacheBuffer.Seek(0);
    }
    while (ctx->cacheBuffer.Remain() >= MD5Calculate::MD5_BLOCK_SIZE)
    {
        MD5Calculate::processBlock(&(ctx->status[0]), &(ctx->status[1]), &(ctx->status[2]), &(ctx->status[3]),
                                   reinterpret_cast<const unsigned char *>(ctx->cacheBuffer.Byte() + ctx->cacheBuffer.Position()));
        ctx->cacheBuffer.Seek(ctx->cacheBuffer.Position() + MD5Calculate::MD5_BLOCK_SIZE);
        ctx->count += MD5Calculate::MD5_BLOCK_SIZE;
    }
    ctx->cacheBuffer.Shift(ctx->cacheBuffer.Position());
    ctx->cacheBuffer.Seek(0);
}

std::string MD5Helper::MD5Ctx_Final(std::shared_ptr<MD5Context> ctx)
{
    if (ctx->cacheBuffer.Length() >= 64)
        MD5Ctx_Update(ctx, nullptr, 0);

    ctx->count += ctx->cacheBuffer.Length();

    unsigned long long lastMessageLength = ctx->cacheBuffer.Length();
    unsigned long long paddingBlcokLength = 0;
    if (MD5Calculate::MD5_BLOCK_SIZE - ctx->cacheBuffer.Length() >= 9)
        paddingBlcokLength = MD5Calculate::MD5_BLOCK_SIZE;
    else
        paddingBlcokLength = 2 * MD5Calculate::MD5_BLOCK_SIZE;

    ctx->cacheBuffer.ReSize(paddingBlcokLength);
    char *ptr = ctx->cacheBuffer.Byte();
    ptr[lastMessageLength] = 0x80;
    unsigned long long paddingzerolen = paddingBlcokLength - (lastMessageLength + 1 + 8);
    memset(ptr + (lastMessageLength + 1), 0, paddingzerolen);
    unsigned long long bitLength = ctx->count * 8;
    memcpy(ptr + (lastMessageLength + 1 + paddingzerolen), &bitLength, 8);
    assert(paddingBlcokLength == MD5Calculate::MD5_BLOCK_SIZE || paddingBlcokLength == 2 * MD5Calculate::MD5_BLOCK_SIZE);

    MD5Ctx_Update(ctx, nullptr, 0);

    return MD5Calculate::toHexString(reinterpret_cast<unsigned char *>(ctx->status));
}

