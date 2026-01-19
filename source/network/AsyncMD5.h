#pragma once

#include "Buffer.h"
#include <memory>

struct MD5SnapShot
{
    unsigned int status[4];
    unsigned long long count;
    Buffer cacheBuffer;
    bool isfinish;
    std::string md5string;

    MD5SnapShot();
    MD5SnapShot(MD5SnapShot &other);
    MD5SnapShot(const MD5SnapShot &other);
    MD5SnapShot &operator=(MD5SnapShot &other);
};

class AsyncMD5Handle;
class AsyncMD5
{
public:
    AsyncMD5();
    AsyncMD5(MD5SnapShot &snapshot);
    ~AsyncMD5();

    AsyncMD5(const AsyncMD5 &) = delete;
    AsyncMD5(AsyncMD5 &&) = delete;
    AsyncMD5 &operator=(const AsyncMD5 &) = delete;
    AsyncMD5 &operator=(AsyncMD5 &&) = delete;


    void Update(Buffer &data);
    void UpdateSync(Buffer &data);
    std::string Final();

    uint64_t Count();
    MD5SnapShot SnapShot();
    void LoadSnapShot(MD5SnapShot &shot);

private:
    std::shared_ptr<AsyncMD5Handle> _handle;
    std::string _md5string;
};
