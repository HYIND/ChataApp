#pragma once

#include "stdafx.h"
#include "MsgManager.h"

class MsgManager;

using namespace std;

struct User
{
    string token;
    string name;
    string ip;
    uint16_t port;
    BaseNetWorkSession *session;
};

class LoginUserManager
{
public:
    LoginUserManager();
    ~LoginUserManager();

public:
    // 登录
    bool Login(BaseNetWorkSession *session, string ip, uint16_t port);
    // 登出
    bool Logout(BaseNetWorkSession *session, string token, string ip, uint16_t port);
    // 验证
    bool Verfiy(BaseNetWorkSession *session, string token, string ip, uint16_t port);
    // 用户登录后返回给用户的登录信息
    bool SendLoginInfo(User *u);

public:
    // 获取在线用户
    SafeArray<User *> &GetOnlineUsers();
    bool FindByToken(const string &token, User *&out);

public:
    static string PublicChatToken();
    static bool IsPublicChat(const string &token);

public:
    void SetMsgManager(MsgManager *m);

private:
    SafeArray<User *> OnlineUsers;
    MsgManager *HandleMsgManager;
};