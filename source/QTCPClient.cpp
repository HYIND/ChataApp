#include "QTCPClient.h"
#include <QNetworkProxy>

TCPClient::TCPClient(QTcpSocket *con)
{
    if (con)
        BaseCon = con;
    else
        BaseCon = new QTcpSocket();
}

TCPClient::~TCPClient()
{
    Release();
    SAFE_DELETE(BaseCon);
}
#include <QThread>
bool TCPClient::Connect(const QString &IP, quint16 Port)
{

    bool result =false;

    QHostAddress address(IP);
    BaseCon->setProxy(QNetworkProxy::NoProxy);
    BaseCon->connectToHost(address,Port); // 连接到主机和端口

    // 阻塞等待连接完成，超时时间3000毫秒
    if (BaseCon->waitForConnected(3000)) {
        // 连接成功
        result=true;
        qDebug() << "Connected!" << BaseCon->peerAddress().toString();
    } else {
        // 连接失败
        qDebug() << "Error When connect "<<BaseCon->peerAddress().toString()<<",error:" << BaseCon->errorString();
    }
    if (!result)
        return false;

    // connect(socket, &QTcpSocket::connected, this, &MyClient::onConnected);
    // connect(socket, &QTcpSocket::readyRead, this, &MyClient::onReadyRead);
    // connect(socket, &QTcpSocket::disconnected, this, &MyClient::onDisconnected);
    // connect(socket, &QTcpSocket::errorOccurred, this, &MyClient::onError);

    QObject::connect(BaseCon,&QTcpSocket::readyRead,this,&TCPClient::RecvBuffer);
    QObject::connect(BaseCon,&QTcpSocket::disconnected,this,&TCPClient::ConnectClose);


    // BaseCon->BindBufferCallBack(std::bind(&TCPClient::RecvBuffer, this, std::placeholders::_1, std::placeholders::_2));
    // BaseCon->BindRDHUPCallBack(std::bind(&TCPClient::ConnectClose, this, std::placeholders::_1));

    return true;
}

bool TCPClient::Release()
{
    cacheBuffer.Release();

    if (!BaseCon)
        return true;
    BaseCon->disconnectFromHost();
    isHandshakeComplete = false;
    _callbackMessage = nullptr;
    _callbackClose = nullptr;

    return true;
}

bool TCPClient::OnRecvBuffer(Buffer *buffer)
{
    if (!isHandshakeComplete)
    {
        if (CheckHandshakeConfirmMsg(*buffer) != CheckHandshakeStatus::Success)
            return false;
    }

    if (buffer->Remaind() > 0)
        cacheBuffer.Append(*buffer);

    if (_callbackMessage)
    {
        _callbackMessage(this, &cacheBuffer);
        cacheBuffer.Release();
    }

    return true;
}

bool TCPClient::OnConnectClose()
{
    auto callback = _callbackClose;
    Release();
    if (callback)
        callback(this);
    return true;
}

bool TCPClient::Send(const Buffer &buffer)
{
    try
    {
        if (!buffer.Data() || buffer.Length() <= 0)
            return true;
        bool result = BaseCon->write(buffer.Byte(),buffer.Length())!=0;
        if(result)
            BaseCon->flush();
        return result;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }
}

bool TCPClient::TryHandshake(uint32_t timeOutMs)
{
    return true;
}

CheckHandshakeStatus TCPClient::CheckHandshakeTryMsg(Buffer &buffer)
{
    isHandshakeComplete = true;
    return CheckHandshakeStatus::Success;
}

CheckHandshakeStatus TCPClient::CheckHandshakeConfirmMsg(Buffer &buffer)
{
    isHandshakeComplete = true;
    return CheckHandshakeStatus::Success;
}

void TCPClient::RecvBuffer()
{
    // if (con != BaseCon)
    //     return false;

    if (!BaseCon->isValid())
        return;
    QByteArray bytes = BaseCon->readAll();
    Buffer buffer(bytes.data(),bytes.length());
    OnRecvBuffer(&buffer);
}

void TCPClient::ConnectClose()
{
    // if (con != BaseCon)
    //     return false;

    if (!BaseCon->isValid())
        return;

    OnConnectClose();
}

void TCPClient::BindMessageCallBack(std::function<void(TCPClient *, Buffer *)> callback)
{
    _callbackMessage = callback;
}
void TCPClient::BindCloseCallBack(std::function<void(TCPClient *)> callback)
{
    _callbackClose = callback;
}

QTcpSocket *TCPClient::GetBaseCon()
{
    return BaseCon;
}

