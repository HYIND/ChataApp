#include "LoginUserManager.h"

std::string GenerateSimpleUuid()
{
    // 获取当前时间戳（毫秒精度）
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    // 初始化随机数生成器
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint16_t> dis(0, 0xFFFF);

    // 分解时间戳
    uint32_t time_low = static_cast<uint32_t>(millis & 0xFFFFFFFF);     // 低32位
    uint16_t time_mid = static_cast<uint16_t>((millis >> 32) & 0xFFFF); // 中16位
    uint16_t time_hi = static_cast<uint16_t>((millis >> 48) & 0x0FFF);  // 高12位

    // 生成4位随机数 (16位)
    uint16_t rand_num = dis(gen) & 0xFFFF;

    // 组合成UUID格式
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << time_low                         /* << "-" */
       << std::setw(4) << time_mid                         /* << "-" */
       << std::setw(4) << time_hi                          /* << "-" */
       << std::setw(4) << (rand_num & 0x0FFF) /* << "-" */ // 使用12位随机数
       << std::setw(4) << (rand_num >> 4);                 // 使用剩余4位

    return ss.str();
}

string publicchattoken = "publicchat";
User *GeneratePublicChat()
{
    User *publicchat = new User();
    publicchat->token = publicchattoken;
    publicchat->name = "公共频道";
    publicchat->ip = "0.0.0.0";
    publicchat->port = 0;
    publicchat->session = nullptr;
    return publicchat;
}

bool LoginUserManager::IsPublicChat(const string &token)
{
    return publicchattoken == token;
}

string LoginUserManager::PublicChatToken()
{
    return publicchattoken;
}

LoginUserManager::LoginUserManager()
{
    User *publicchat = GeneratePublicChat();
    OnlineUsers.emplace(publicchat);
    HandleMsgManager = nullptr;
}

LoginUserManager::~LoginUserManager()
{
}

bool LoginUserManager::Login(BaseNetWorkSession *session, string ip, uint16_t port)
{
    bool success = false;

    bool exist = false;
    OnlineUsers.EnsureCall(
        [&](std::vector<User *> &array) -> void
        {
            for (auto it = array.begin(); it != array.end(); it++)
            {
                User *u = *it;
                if (u->session == session && u->port == port && u->ip == ip)
                {
                    exist = true;
                    return;
                }
            }
        });

    if (!exist)
    {
        string uuid = GenerateSimpleUuid();
        User *u = new User();
        u->token = uuid;
        u->name = "用户" + uuid.substr(uuid.size() - 4, 4);
        u->ip = ip;
        u->port = port;
        u->session = session;
        OnlineUsers.emplace(u);
        if (!SendLoginInfo(u))
        {
        }
        if (HandleMsgManager)
        {
            HandleMsgManager->SendOnlineUserMsg(u->session, u->token, u->ip, u->port);
        }
        success = true;
    }

    return success;
}

bool LoginUserManager::Logout(BaseNetWorkSession *session, string token, string ip, uint16_t port)
{
    bool success = false;

    OnlineUsers.EnsureCall(
        [&](std::vector<User *> &array) -> void
        {
            for (auto it = array.begin(); it != array.end(); it++)
            {
                User *u = *it;
                if (u->session == session /* && u->token == token */ && u->port == port && u->ip == ip)
                {
                    array.erase(it);
                    SAFE_DELETE(u);
                    success = true;
                    return;
                }
            }
        });

    return success;
}

bool LoginUserManager::Verfiy(BaseNetWorkSession *session, string token, string ip, uint16_t port)
{
    bool success = false;

    OnlineUsers.EnsureCall(
        [&](std::vector<User *> &array) -> void
        {
            for (auto it = array.begin(); it != array.end(); it++)
            {
                User *u = *it;
                if (u->session == session && u->token == token && u->port == port && u->ip == ip)
                {
                    success = true;
                    return;
                }
            }
        });

    return success;
}

bool LoginUserManager::SendLoginInfo(User *u)
{
    if (!u || !u->session)
        return false;

    json js;
    js["command"] = 2000;
    js["token"] = u->token;
    js["name"] = u->name;
    js["ip"] = u->ip;
    js["port"] = u->port;

    Buffer buf = js.dump();
    return u->session->AsyncSend(buf);
}

SafeArray<User *> &LoginUserManager::GetOnlineUsers()
{
    return OnlineUsers;
}

bool LoginUserManager::FindByToken(const string &token, User *&out)
{
    bool success = false;

    OnlineUsers.EnsureCall(
        [&](std::vector<User *> &array) -> void
        {
            for (auto it = array.begin(); it != array.end(); it++)
            {
                User *u = *it;
                if (u->token == token)
                {
                    out = u;
                    success = true;
                    return;
                }
            }
        });

    return success;
}

void LoginUserManager::SetMsgManager(MsgManager *m)
{
    HandleMsgManager = m;
}
