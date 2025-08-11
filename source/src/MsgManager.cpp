#include "MsgManager.h"
#include "MessageRecordStore.h"
#include "FileTransManager.h"
#include "FileRecordStore.h"

int64_t GetTimeStampSecond()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto second = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    return second;
}

bool MsgManager::ProcessMsg(BaseNetWorkSession *session, string ip, uint16_t port, Buffer *buf)
{
    string str(buf->Byte(), buf->Length());
    json js;
    try
    {
        js = json::parse(str);
    }
    catch (...)
    {
        cout << fmt::format("json::parse error : {}\n", str);
    }
    if (!js.contains("command"))
        return false;
    int command = js.at("command");

    if (command == 7000 || command == 7001 || command == 7010 || command == 7080 || command == 7070 ||
        command == 8000 || command == 8001 || command == 8010 || command == 4001)
    {
        FILETRANSMANAGER->ProcessMsg(session, ip, port, js);
    }

    if (!js.contains("token"))
        return false;
    string token = js["token"];

    bool success = false;
    if (HandleLoginUser)
        success = HandleLoginUser->Verfiy(session, token);
    if (success)
    {
        if (js.contains("command"))
        {
            if (command == 1001)
            {
                SendOnlineUserMsg(session, token, ip, port);
            }
            if (command == 1002)
            {
                ProcessChatMsg(session, token, ip, port, js);
            }
            if (command == 1003)
            {
                ProcessFetchRecord(session, token, js);
            }
        }
    }
    return success;
}

bool MsgManager::ProcessChatMsg(BaseNetWorkSession *session, string token, string ip, uint16_t port, json &js_src)
{
    if (!js_src.contains("token"))
    {
        return false;
    }
    if (!js_src.contains("goaltoken"))
    {
        return false;
    }
    if (!js_src.contains("msg"))
    {
        return false;
    }

    string srctoken = js_src["token"];
    string goaltoken = js_src["goaltoken"];

    if (HandleLoginUser->IsPublicChat(goaltoken))
        return BroadCastPublicChatMsg(srctoken, js_src);
    else
        return ForwardChatMsg(srctoken, goaltoken, js_src);
}

bool MsgManager::BroadCastPublicChatMsg(string token, json &js_src)
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

            Buffer buf = js.dump();
            user->session->AsyncSend(buf);
        } });

    return true;
}

bool MsgManager::ForwardChatMsg(string srctoken, string goaltoken, json &js_src)
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

    Buffer buf = js.dump();
    for (auto user : {sender, recver})
        user->session->AsyncSend(buf);

    return true;
}

bool MsgManager::SendOnlineUserMsg(BaseNetWorkSession *session, string token, string ip, uint16_t port)
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

    Buffer buf = js.dump();
    session->AsyncSend(buf);

    return true;
}

bool MsgManager::ProcessFetchRecord(BaseNetWorkSession *session, string token, json &js_src)
{
    if (!js_src.contains("goaltoken"))
        return false;

    string srctoken = token;
    string goaltoken = js_src["goaltoken"];

    vector<MsgRecord> MsgRecords = MESSAGERECORDSTORE->FetchLastMsg(srctoken, goaltoken);

    json js;
    js["command"] = 2004;

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
        js_record["msg"] = msgrecord.msg;

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

    Buffer buf = js.dump();
    session->AsyncSend(buf);

    return true;
}

void MsgManager::SetLoginUserManager(LoginUserManager *m)
{
    HandleLoginUser = m;
}