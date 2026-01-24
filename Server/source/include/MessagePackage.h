#pragma once

#include "stdafx.h"
#include "Net/include/Helper/Buffer.h"

// 定义自定义的消息包，包含json字段和可选的buffer字段，
// 避免把过长的流数据放在json导致序列化速度降低
struct MessagePackage
{
    bool jsonenable;
    bool bufferenable;

    uint32_t jsonlen;
    uint64_t bufferlen;

    json nlmjson;

    Buffer jsondata;
    Buffer bufferdata;

    MessagePackage();
    MessagePackage(json &nlmjson);
    MessagePackage(Buffer &buf);
    MessagePackage(json &nlmjson, Buffer &buf);

    void SetJson(json &nlmjson);
    void SetBuffer(Buffer &buf);
};

// input buf
// output package
bool AnalysisMessagePackageFromBuffer(Buffer *buf, MessagePackage *package);

// input package
// output buf
void GenerateMessagePackageToBuffer(MessagePackage *package, Buffer *buf);
