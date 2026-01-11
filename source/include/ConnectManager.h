#pragma once

#include "stdafx.h"
#include "LoginUserManager.h"
#include "MsgManager.h"

class ConnectManager
{
public:
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
    SafeArray<BaseNetWorkSession *> sessions;
    LoginUserManager *HandleLoginUser = nullptr;
    MsgManager *HandleMsg = nullptr;
};