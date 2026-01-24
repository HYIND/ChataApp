#include "stdafx.h"
#include "ConnectManager.h"
#include "LoginUserManager.h"
#include "MsgManager.h"
#include "MessageRecordStore.h"
#include "FileTransManager.h"

void signal_handler(int sig)
{
    if (sig == SIGINT)
    {
        StopNetCoreLoop();
    }
}

int main()
{
    // LOGGER->SetLoggerPath("server.log");
    InitNetCore();

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

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

    std::string IP = "192.168.58.130";
    int port = 8888;
    if (!ConnectHost.Start(IP, port))
        return -1;

    RunNetCoreLoop(true);
}