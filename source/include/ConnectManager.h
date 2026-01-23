#pragma once

#include "stdafx.h"
#include "LoginUserManager.h"
#include "MsgManager.h"

class ConnectManager
{
public:
    ConnectManager();
    bool Start(const std::string &IP, int Port);

    // 建立连接
    void callBackSessionEstablish(BaseNetWorkSession *session);
    // 收取消息
    void callBackRecvMessage(BaseNetWorkSession *basesession, Buffer *recv);
    // 连接关闭
    void callBackCloseConnect(BaseNetWorkSession *session);

public:
    void SetLoginUserManager(LoginUserManager *m);
    void SetMsgManager(MsgManager *m);

private:
    std::unique_ptr<NetWorkSessionListener> listener;
    SafeArray<BaseNetWorkSession *> sessions;
    std::string ip;
    int port;

    LoginUserManager *HandleLoginUser = nullptr;
    MsgManager *HandleMsg = nullptr;
};