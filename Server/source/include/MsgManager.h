#pragma once

#include "stdafx.h"
#include "LoginUserManager.h"

class LoginUserManager;

using namespace std;

class MsgManager
{
public:
    // 处理消息的入口
    bool ProcessMsg(BaseNetWorkSession *session, Buffer *buf);
    // 处理发送消息的请求
    bool ProcessChatMsg(BaseNetWorkSession *session, const string& token, json &js_src, Buffer &buf);
    // 拉取当前在线的用户
    bool SendOnlineUserMsg(BaseNetWorkSession *session, const string& token);
    // 拉取聊天记录
    bool ProcessFetchRecord(BaseNetWorkSession *session, const string& token, json &js_src, Buffer &buf);

private:
    // 转发私聊消息
    bool ForwardChatMsg(const string& srctoken, const string& goaltoken, json &js_src, Buffer &buf_src);
    // 广播公共频道消息
    bool BroadCastPublicChatMsg(const string& token, json &js_src, Buffer &buf_src);

public:
    void SetLoginUserManager(LoginUserManager *m);

private:
    LoginUserManager *HandleLoginUser = nullptr;
};