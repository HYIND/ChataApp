#ifndef CONNECTMANAGER_H
#define CONNECTMANAGER_H

#include <QTcpSocket>
#include "QCustomTcpSession.h"
#include <QThread>

//用于管理与服务器的连接

enum class ConnectStatus
{
    disconnect = 0,
    connecting = 1,
    connected = 2
};

class ConnectManager:public QObject
{
    Q_OBJECT

public:
    static ConnectManager* Instance();
    void Login(const QString& IP,quint16 port);
    void Logout();
    void Send(const QByteArray &buf);
    ConnectStatus connectStatus();

public:
    void OnPeerClose(BaseNetWorkSession* s);
    void OnRecvMessage(BaseNetWorkSession* s,Buffer* recv,Buffer* response);

    CustomTcpSession* Session()const;
signals:
    void initialized();
    void signal_Login(const QString& IP,quint16 port);
    void signal_Logout();
    void signal_Send(const QByteArray &buf);
    void signal_PeerClose();
    void signal_RecvMessage(QByteArray& recv);

public slots:
    void slots_Login(const QString& IP,quint16 port);
    void slots_Logout();
    void slots_Send(const QByteArray &buf);
    void slots_PeerClose();
    void slots_RecvMessage(QByteArray& recv);

private:
    ConnectManager();
    ~ConnectManager();

private:
    ConnectStatus status = ConnectStatus::disconnect;
    CustomTcpSession* session=nullptr;
};

#define CONNECTMANAGER ConnectManager::Instance()

#endif // CONNECTMANAGER_H
