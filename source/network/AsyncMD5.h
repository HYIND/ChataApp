#pragma once

#include "Buffer.h"
#include <memory>

class AsyncMD5Handle;
class AsyncMD5
{
public:
    AsyncMD5();
    ~AsyncMD5();

    AsyncMD5(const AsyncMD5&) = delete;
    AsyncMD5(AsyncMD5&&) = delete;
    AsyncMD5& operator=(const AsyncMD5&) = delete;
    AsyncMD5& operator=(AsyncMD5&&) = delete;

    void Update(Buffer &data);
    void UpdateSync(Buffer &data);
    std::string Final();

    uint64_t Count();

private:
    std::shared_ptr<AsyncMD5Handle> _handle;
    std::string _md5string;
};
