#pragma once

#include "QTCPClient.h"

class BaseNetWorkSession:public QObject
{

public:
    BaseNetWorkSession();
    virtual ~BaseNetWorkSession();
    virtual bool Connect(const QString &IP, quint16 Port);
    virtual bool Release();

public: // 供Listener/EndPoint调用,须继承实现
    virtual bool AsyncSend(const Buffer &buffer) = 0;
    virtual bool TryHandshake(uint32_t timeOutMs) = 0;
    virtual CheckHandshakeStatus CheckHandshakeTryMsg(Buffer &buffer) = 0;
    virtual CheckHandshakeStatus CheckHandshakeConfirmMsg(Buffer &buffer) = 0;

public: // 供外部调用
    void BindRecvDataCallBack(std::function<void(BaseNetWorkSession *, Buffer *recv)> callback);
    void BindSessionCloseCallBack(std::function<void(BaseNetWorkSession *)> callback);
    QString GetIPAddr();
    quint16 GetPort();

public: // 供Listener/EndPoint调用
    void RecvData(TCPClient *client, Buffer *buffer);
    void SessionClose(TCPClient *client);
    TCPClient *GetBaseClient();

protected: // 须继承实现
    virtual bool OnSessionClose() = 0;
    virtual bool OnRecvData(Buffer *buffer) = 0;
    virtual void OnBindRecvDataCallBack() = 0;
    virtual void OnBindSessionCloseCallBack() = 0;

protected:
    TCPClient *BaseClient;

    bool isHandshakeComplete;

    std::function<void(BaseNetWorkSession *, Buffer *recv)> _callbackRecvData;
    std::function<void(BaseNetWorkSession *)> _callbackSessionClose;
};
