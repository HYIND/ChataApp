#pragma once

#include <QTcpSocket>
#include "Buffer.h"

enum class CheckHandshakeStatus
{
    None = -1,
    Fail = 0,       // 失败
    Success = 1,    // 成功
    BufferAgain = 2 // 等待数据
};


// 基于TCP协议的应用层客户端封装
class TCPClient: public QObject
{
    Q_OBJECT
public:
    TCPClient(QTcpSocket *con = nullptr);
    ~TCPClient();

public:
    virtual bool Connect(const QString &IP, quint16 Port);
    virtual bool Release();

    virtual bool OnRecvBuffer(Buffer *buffer);
    virtual bool OnConnectClose();

    virtual bool Send(Buffer &buffer);

    QTcpSocket *GetBaseCon();

public:
    virtual bool TryHandshake(uint32_t timeOutMs);
    virtual CheckHandshakeStatus CheckHandshakeTryMsg(Buffer &buffer);
    virtual CheckHandshakeStatus CheckHandshakeConfirmMsg(Buffer &buffer);

    void BindMessageCallBack(std::function<void(TCPClient *, Buffer *)> callback);
    void BindCloseCallBack(std::function<void(TCPClient *)> callback);

public slots:
    void RecvBuffer();      // 用于连接QTcpSocket的readyread信号
    void ConnectClose();    // 用于连接QTcpSocket的disconnected信号

protected:
    QTcpSocket *BaseCon;
    bool isHandshakeComplete = false;

    std::function<void(TCPClient *, Buffer *)> _callbackMessage;
    std::function<void(TCPClient *)> _callbackClose;

private:
    Buffer cacheBuffer;
};
