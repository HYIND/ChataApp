
// 聊天记录存储，用以云端存储聊天记录
// 聊天记录以文件形式存储在本地

#pragma once

#include "stdafx.h"
#include "FileIOHandler.h"

using namespace std;

struct MsgRecord
{
    string srctoken;
    string goaltoken;
    string name;
    int64_t time;
    string ip;
    uint16_t port;
    int type;
    string msg;
};

class MessageRecordStore
{
public:
    static MessageRecordStore *Instance();

public:
    // 写入存储消息
    bool StoreMsg(const MsgRecord &msg);
    // 拉取存储消息
    vector<MsgRecord> FetchMsg(const string &srctoken = "", const string &goaltoken = "");
    void SetEnable(bool value);

private:
    MessageRecordStore() = default;
    bool enable = true;
};

#define MESSAGERECORDSTORE MessageRecordStore::Instance()