#include "ConnectManager.h"

void ConnectManager::callBackSessionEstablish(BaseNetWorkSession *session)
{
    session->BindRecvDataCallBack(std::bind(&ConnectManager::callBackRecvMessage, this, std::placeholders::_1, std::placeholders::_2));
    session->BindSessionCloseCallBack(std::bind(&ConnectManager::callBackCloseConnect, this, std::placeholders::_1));
    sessions.emplace(session);

    std::cout << fmt::format("User Login :RemoteAddr={}:{} \n",
                             session->GetIPAddr(), session->GetPort());

    bool success = false;
    if (HandleLoginUser)
        success = HandleLoginUser->Login(session, session->GetIPAddr(), session->GetPort());
    if (success)
    {
    }
}

void ConnectManager::callBackRecvMessage(BaseNetWorkSession *basesession, Buffer *recv)
{
    CustomTcpSession *session = (CustomTcpSession *)basesession;

    // std::cout << fmt::format("Server recvData:RemoteAddr={}:{} \n",
    //                          session->GetIPAddr(), session->GetPort());

    bool success = false;
    if (HandleMsg)
        HandleMsg->ProcessMsg(session, recv);
    if (success)
    {
    }
}

void ConnectManager::callBackCloseConnect(BaseNetWorkSession *session)
{

    std::cout << fmt::format("Client Connection Close: RemoteIpAddr={}:{} \n",
                             session->GetIPAddr(), session->GetPort());

    bool success = false;
    if (HandleLoginUser)
        HandleLoginUser->Logout(session, session->GetIPAddr(), session->GetPort());
    if (success)
    {
    }

    sessions.EnsureCall(
        [&](std::vector<BaseNetWorkSession *> &array) -> void
        {
            for (auto it = array.begin(); it != array.end(); it++)
            {
                if ((*it) == session)
                {
                    array.erase(it);
                    return;
                }
            }
        }

    );

    DeleteLater(session);
}

void ConnectManager::SetLoginUserManager(LoginUserManager *m)
{
    HandleLoginUser = m;
}

void ConnectManager::SetMsgManager(MsgManager *m)
{
    HandleMsg = m;
}
