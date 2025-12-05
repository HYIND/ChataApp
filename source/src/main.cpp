#include "stdafx.h"
#include "ConnectManager.h"
#include "LoginUserManager.h"
#include "MsgManager.h"
#include "MessageRecordStore.h"
#include "FileTransManager.h"

int main()
{
    // LOGGER->SetLoggerPath("server.log");
    InitNetCore();

    ConnectManager ConnectHost;     // 用以处理连接和收发消息
    LoginUserManager LoginUserHost; // 用于存储当前在线用户
    MsgManager MsgHost;             // 用于处理消息的具体逻辑

    // 三者绑定
    ConnectHost.SetLoginUserManager(&LoginUserHost);
    ConnectHost.SetMsgManager(&MsgHost);
    MsgHost.SetLoginUserManager(&LoginUserHost);
    LoginUserHost.SetMsgManager(&MsgHost);

    // 开启消息记录存储
    MESSAGERECORDSTORE->SetEnable(true);
    // 文件传输系统注入用户管理，用以校验用户请求
    FILETRANSMANAGER->SetLoginUserManager(&LoginUserHost);

    NetWorkSessionListener listener(SessionType::CustomTCPSession);
    listener.BindSessionEstablishCallBack(std::bind(&ConnectManager::callBackSessionEstablish, &ConnectHost, std::placeholders::_1));

    std::string IP = "192.168.58.128";
    int port = 8888;
    if (!listener.Listen(IP, port))
    // if (!listener.Listen("127.0.0.1", 8888))
    {
        perror("listen error !");
        return -1;
    }

    RunNetCoreLoop(true);
}