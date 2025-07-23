#include "MsgManager.h"

enum class MsgType
{
    text = 1,
    picture
};

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
    if (!js.contains("token"))
        return false;
    string token = js["token"];

    bool success = false;
    if (HandleLoginUser)
        success = HandleLoginUser->Verfiy(session, token, ip, port);
    if (success)
    {
        if (js.contains("command"))
        {
            int command = js.at("command");
            if (command == 1001)
            {
                SendOnlineUserMsg(session, token, ip, port);
            }
            if (command == 1002)
            {
                ProcessChatMsg(session, token, ip, port, js);
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

    json js;
    js["command"] = 2003;
    js["srctoken"] = sender->token;
    js["goaltoken"] = HandleLoginUser->PublicChatToken();
    js["name"] = sender->name;
    js["time"] = GetTimeStampSecond();
    js["ip"] = sender->ip;
    js["port"] = sender->port;
    js["type"] = int(type);
    js["msg"] = msg;

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

    json js;
    js["command"] = 2003;
    js["srctoken"] = sender->token;
    js["goaltoken"] = recver->token;
    js["name"] = sender->name;
    js["time"] = GetTimeStampSecond();
    js["ip"] = sender->ip;
    js["port"] = sender->port;
    js["type"] = int(type);
    js["msg"] = msg;

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

void MsgManager::SetLoginUserManager(LoginUserManager *m)
{
    HandleLoginUser = m;
}