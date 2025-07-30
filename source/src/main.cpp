#include "stdafx.h"
#include "ConnectManager.h"
#include "LoginUserManager.h"
#include "MsgManager.h"
#include "MessageRecordStore.h"

int main()
{
    // LOGGER->SetLoggerPath("server.log");
    InitNetCore();

    ConnectManager ConnectHost;     // 用以处理连接和收发消息
    LoginUserManager LoginUserHost; // 用于存储当前在线用户
    MsgManager MsgHost;             // 用于处理消息的具体逻辑

    //三者绑定
    ConnectHost.SetLoginUserManager(&LoginUserHost);
    ConnectHost.SetMsgManager(&MsgHost);
    MsgHost.SetLoginUserManager(&LoginUserHost);
    LoginUserHost.SetMsgManager(&MsgHost);

    //开启消息记录存储
    MESSAGERECORDSTORE->SetEnable(true);

    NetWorkSessionListener listener(SessionType::CustomTCPSession);
    listener.BindSessionEstablishCallBack(std::bind(&ConnectManager::callBackSessionEstablish, &ConnectHost, std::placeholders::_1));
    if (!listener.Listen("192.168.58.128", 8888))
    // if (!listener.Listen("127.0.0.1", 8888))
    {
        perror("listen error !");
        return -1;
    }

    RunNetCoreLoop(true);
}