#include "QBaseNetWorkSession.h"

BaseNetWorkSession::BaseNetWorkSession()
    : isHandshakeComplete(false)
{
}

BaseNetWorkSession::~BaseNetWorkSession()
{
    isHandshakeComplete = false;
    _callbackRecvData = nullptr;
    _callbackSessionClose = nullptr;
}

bool BaseNetWorkSession::Connect(const QString &IP, quint16 Port)
{
    Release();

    bool result = BaseClient->Connect(IP, Port);
    if (!result)
        return false;

    BaseClient->BindMessageCallBack(std::bind(&BaseNetWorkSession::RecvData, this, std::placeholders::_1, std::placeholders::_2));
    // 尝试握手，超时时间10秒
    if (!TryHandshake(10 * 1000))
    {
        qDebug() << "CustomTcpSession::TryHandshake Connect Fail! CloseConnection\n";
        Release();
        return false;
    }

    BaseClient->BindCloseCallBack(std::bind(&BaseNetWorkSession::SessionClose, this, std::placeholders::_1));
    return true;
}

bool BaseNetWorkSession::Release()
{
    isHandshakeComplete = false;
    _callbackRecvData = nullptr;
    _callbackSessionClose = nullptr;
    return BaseClient->Release();
}

void BaseNetWorkSession::BindRecvDataCallBack(std::function<void(BaseNetWorkSession *, Buffer *recv)> callback)
{
    _callbackRecvData = callback;
    OnBindRecvDataCallBack();
}
void BaseNetWorkSession::BindSessionCloseCallBack(std::function<void(BaseNetWorkSession *)> callback)
{
    _callbackSessionClose = callback;
    OnBindSessionCloseCallBack();
}

QString BaseNetWorkSession::GetIPAddr()
{
    return BaseClient->GetBaseCon()->peerAddress().toString();
}

quint16 BaseNetWorkSession::GetPort()
{
    return BaseClient->GetBaseCon()->peerPort();
}

void BaseNetWorkSession::RecvData(TCPClient *client, Buffer *buffer)
{
    if (client != BaseClient)
        return;
    OnRecvData(buffer);
}

void BaseNetWorkSession::SessionClose(TCPClient *client)
{
    if (client != BaseClient)
        return;

    OnSessionClose();
}

TCPClient *BaseNetWorkSession::GetBaseClient()
{
    return BaseClient;
}
