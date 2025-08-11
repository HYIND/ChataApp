
// 聊天记录存储，用以云端存储聊天记录
// 聊天记录以文件形式存储在本地

#pragma once

#include "stdafx.h"
#include "FileIOHandler.h"

using namespace std;

enum class MsgType : int
{
    text = 1,
    picture = 2,
    file = 3
};

struct MsgRecord
{
    string srctoken;
    string goaltoken;
    string name;
    int64_t time;
    string ip;
    uint16_t port;
    MsgType type;
    string msg;
    string filename;
    uint64_t filesize;
    string md5;
    string fileid;
};

class MessageRecordStore
{
public:
    static MessageRecordStore *Instance();

public:
    // 写入存储消息
    bool StoreMsg(const MsgRecord &msg);
    // 拉取存储消息
    vector<MsgRecord> FetchAllMsg(const string &srctoken = "", const string &goaltoken = "");
    vector<MsgRecord> FetchLastMsg(const string &srctoken, const string &goaltoken, uint32_t count = 20);
    void SetEnable(bool value);

private:
    MessageRecordStore();
    bool enable = true;
};

#define MESSAGERECORDSTORE MessageRecordStore::Instance()