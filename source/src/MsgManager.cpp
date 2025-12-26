#include "MsgManager.h"
#include "MessageRecordStore.h"
#include "FileTransManager.h"
#include "FileRecordStore.h"
#include "MessagePackage.h"
#include "NetWorkHelper.h"

int64_t GetTimeStampSecond()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto second = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    return second;
}

bool MsgManager::ProcessMsg(BaseNetWorkSession *session, Buffer *buf)
{
    MessagePackage package;
    if (!AnalysisMessagePackageFromBuffer(buf, &package))
        return false;

    if (!package.jsonenable)
        return false;

    json &js_src = package.nlmjson;
    Buffer &buf_src = package.bufferdata;

    if (!js_src.contains("command"))
        return false;
    int command = js_src.at("command");

    if (command == 7000 || command == 7001 || command == 7010 || command == 7080 || command == 7070 ||
        command == 8000 || command == 8001 || command == 8010 || command == 4001)
    {
        FILETRANSMANAGER->ProcessMsg(session, js_src, buf_src);
    }

    if (!js_src.contains("jwt"))
        return false;
    string jwtstr = js_src["jwt"];
    string token;

    bool success = false;
    if (HandleLoginUser)
        success = HandleLoginUser->Verfiy(session, jwtstr, token);
    if (success)
    {
        if (js_src.contains("command"))
        {
            if (command == 1001)
            {
                SendOnlineUserMsg(session, token);
            }
            if (command == 1002)
            {
                ProcessChatMsg(session, token, js_src, buf_src);
            }
            if (command == 1003)
            {
                ProcessFetchRecord(session, token, js_src, buf_src);
            }
        }
    }
    return success;
}

bool MsgManager::ProcessChatMsg(BaseNetWorkSession *session, const string& token, json &js_src, Buffer &buf)
{
    if (!js_src.contains("goaltoken"))
    {
        return false;
    }
    if (!js_src.contains("msg"))
    {
        return false;
    }

    string srctoken = token;
    string goaltoken = js_src["goaltoken"];

    if (HandleLoginUser->IsPublicChat(goaltoken))
        return BroadCastPublicChatMsg(srctoken, js_src, buf);
    else
        return ForwardChatMsg(srctoken, goaltoken, js_src, buf);
}

bool MsgManager::BroadCastPublicChatMsg(const string& token, json &js_src, Buffer &buf)
{
    if (!js_src.contains("msg"))
        return false;

    User *sender = nullptr;
    if (!HandleLoginUser->FindByToken(token, sender))
        return false;

    MsgType type = (MsgType)(js_src["type"]);
    string msg = js_src["msg"];

    auto &Users = HandleLoginUser->GetOnlineUsers();

    int64_t time = GetTimeStampSecond();
    json js;
    js["command"] = 2003;
    js["srctoken"] = sender->token;
    js["goaltoken"] = HandleLoginUser->PublicChatToken();
    js["name"] = sender->name;
    js["time"] = time;
    js["ip"] = sender->ip;
    js["port"] = sender->port;
    js["type"] = int(type);
    js["msg"] = msg;

    if (type == MsgType::picture && buf.Length() > 0)
        msg = string(buf.Byte(), buf.Length());

    if (type == MsgType::file)
    {
        string filename = js_src["filename"];
        uint64_t filesize = js_src["filesize"];
        string md5 = js_src["md5"];
        string fileid = js_src["fileid"];
        js["filename"] = filename;
        js["filesize"] = filesize;
        js["md5"] = md5;
        js["fileid"] = fileid;

        FILERECORDSTORE->addFileRecord(fileid, md5, filesize);
        MESSAGERECORDSTORE->StoreMsg(
            MsgRecord{
                .srctoken = sender->token,
                .goaltoken = HandleLoginUser->PublicChatToken(),
                .name = sender->name,
                .time = time,
                .ip = sender->ip,
                .port = sender->port,
                .type = type,
                .filename = filename,
                .filesize = filesize,
                .md5 = md5,
                .fileid = fileid});
    }
    else
    {
        MESSAGERECORDSTORE->StoreMsg(
            MsgRecord{
                .srctoken = sender->token,
                .goaltoken = HandleLoginUser->PublicChatToken(),
                .name = sender->name,
                .time = time,
                .ip = sender->ip,
                .port = sender->port,
                .type = type,
                .msg = msg});
    }

    Users.EnsureCall([&](auto &array) -> void
                     {
        for(auto it= array.begin();it!=array.end();it++)
        {
            User* user =*it;
            
            if(HandleLoginUser->IsPublicChat(user->token))
                continue;

            NetWorkHelper::SendMessagePackage(user->session, &js,&buf);
        } });

    return true;
}

bool MsgManager::ForwardChatMsg(const string& srctoken, const string& goaltoken, json &js_src, Buffer &buf_src)
{
    if (!js_src.contains("msg"))
        return false;

    User *sender = nullptr;
    User *recver = nullptr;
    if (!HandleLoginUser->FindByToken(srctoken, sender) || !HandleLoginUser->FindByToken(goaltoken, recver))
        return false;

    MsgType type = (MsgType)(js_src["type"]);
    string msg = js_src["msg"];

    int64_t time = GetTimeStampSecond();
    json js;
    js["command"] = 2003;
    js["srctoken"] = sender->token;
    js["goaltoken"] = recver->token;
    js["name"] = sender->name;
    js["time"] = time;
    js["ip"] = sender->ip;
    js["port"] = sender->port;
    js["type"] = int(type);
    js["msg"] = msg;

    if (type == MsgType::picture && buf_src.Length() > 0)
        msg = string(buf_src.Byte(), buf_src.Length());

    if (type == MsgType::file)
    {
        string filename = js_src["filename"];
        uint64_t filesize = js_src["filesize"];
        string md5 = js_src["md5"];
        string fileid = js_src["fileid"];
        js["filename"] = filename;
        js["filesize"] = filesize;
        js["md5"] = md5;
        js["fileid"] = fileid;

        FILERECORDSTORE->addFileRecord(fileid, md5, filesize);
        MESSAGERECORDSTORE->StoreMsg(
            MsgRecord{
                .srctoken = sender->token,
                .goaltoken = recver->token,
                .name = sender->name,
                .time = time,
                .ip = sender->ip,
                .port = sender->port,
                .type = type,
                .fileid = fileid});
    }
    else
    {
        MESSAGERECORDSTORE->StoreMsg(
            MsgRecord{
                .srctoken = sender->token,
                .goaltoken = recver->token,
                .name = sender->name,
                .time = time,
                .ip = sender->ip,
                .port = sender->port,
                .type = type,
                .msg = msg});
    }

    for (auto user : {sender, recver})
        NetWorkHelper::SendMessagePackage(user->session, &js, &buf_src);

    return true;
}

bool MsgManager::SendOnlineUserMsg(BaseNetWorkSession *session, const string& token)
{
    auto &Users = HandleLoginUser->GetOnlineUsers();

    json js;
    js["command"] = 2001;

    json js_users;

    Users.EnsureCall([&](auto &array) -> void
                     {
        for (auto it = array.begin(); it != array.end(); it++)
        {
            json js_user;

            User *user = *it;

            js_user["token"] = user->token;
            js_user["name"] = user->name;
            js_user["ip"] = user->ip;
            js_user["port"] = user->port;

            js_users.emplace_back(js_user);
        } });

    js["users"] = js_users;

    return NetWorkHelper::SendMessagePackage(session, &js);
}

bool MsgManager::ProcessFetchRecord(BaseNetWorkSession *session, const string& token, json &js_src, Buffer &buf_src)
{
    if (!js_src.contains("goaltoken"))
        return false;

    string srctoken = token;
    string goaltoken = js_src["goaltoken"];

    vector<MsgRecord> MsgRecords = MESSAGERECORDSTORE->FetchLastMsg(srctoken, goaltoken);

    json js;
    js["command"] = 2004;

    Buffer buf;

    json js_messages = json::array();
    for (auto &msgrecord : MsgRecords)
    {
        json js_record;
        js_record["srctoken"] = msgrecord.srctoken;
        js_record["goaltoken"] = msgrecord.goaltoken;
        js_record["name"] = msgrecord.name;
        js_record["time"] = msgrecord.time;
        js_record["ip"] = msgrecord.ip;
        js_record["port"] = msgrecord.port;
        js_record["type"] = int(msgrecord.type);
        js_record["msg"] = "";

        if (msgrecord.type == MsgType::picture)
        {
            js_record["bufferlen"] = msgrecord.msg.length();
            buf.Write(msgrecord.msg.data(), msgrecord.msg.length());
        }
        else
        {
            js_record["msg"] = msgrecord.msg;
        }

        if (msgrecord.type == MsgType::file)
        {
            js_record["filename"] = msgrecord.filename;
            js_record["filesize"] = msgrecord.filesize;
            js_record["md5"] = msgrecord.md5;
            js_record["fileid"] = msgrecord.fileid;
        }

        js_messages.emplace_back(js_record);
    }
    js["messages"] = js_messages;

    NetWorkHelper::SendMessagePackage(session, &js, &buf);

    return true;
}

void MsgManager::SetLoginUserManager(LoginUserManager *m)
{
    HandleLoginUser = m;
}