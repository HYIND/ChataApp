#include "ConnectManager.h"
#include "MsgManager.h"

ConnectManager *ConnectManager::Instance()
{
    static ConnectManager* instance = new ConnectManager();
    return instance;
}

ConnectManager::ConnectManager()
{
    if (!session)
        session =new CustomTcpSession();
    connect(this,&ConnectManager::signal_Login,this,&ConnectManager::slots_Login);
    connect(this,&ConnectManager::signal_Logout,this,&ConnectManager::slots_Logout);
    connect(this,&ConnectManager::signal_Send,this,&ConnectManager::slots_Send);
    connect(this,&ConnectManager::signal_RecvMessage,this,&ConnectManager::slots_RecvMessage);
    connect(this,&ConnectManager::signal_PeerClose,this,&ConnectManager::slots_PeerClose);

    emit initialized();
}

ConnectManager::~ConnectManager()
{
    if(session)
    {
        session->Release();
        delete session;
        session=nullptr;
    }
}

void ConnectManager::Login(const QString &IP, quint16 port)
{
    if (status!=ConnectStatus::disconnect)
        session->Release();

    status = ConnectStatus::connecting;

    emit signal_Login(IP,port);
}

void ConnectManager::Logout()
{
    status = ConnectStatus::disconnect;
    emit signal_Logout();
}

void ConnectManager::Send(const QByteArray &buf)
{
    emit signal_Send(buf);
}

ConnectStatus ConnectManager::connectStatus()
{
    return status;
}

void ConnectManager::OnPeerClose(BaseNetWorkSession *s)
{
    emit signal_PeerClose();
}

void ConnectManager::OnRecvMessage(BaseNetWorkSession *s, Buffer *recv)
{
    QByteArray buffer(recv->Byte(),recv->Length());
    emit signal_RecvMessage(buffer);
}

CustomTcpSession *ConnectManager::Session() const
{
    return session;
}

void ConnectManager::slots_Login(const QString &IP, quint16 port)
{
    if(!session->Connect(IP,port))
    {
        status=ConnectStatus::disconnect;
        return;
    }

    session->BindSessionCloseCallBack(std::bind(&ConnectManager::OnPeerClose,this,std::placeholders::_1));
    session->BindRecvDataCallBack(std::bind(&ConnectManager::OnRecvMessage,this,std::placeholders::_1,std::placeholders::_2));

    status = ConnectStatus::connected;
}

void ConnectManager::slots_Logout()
{
    session->Release();
}

void ConnectManager::slots_Send(const QByteArray &buf)
{
    if (status!=ConnectStatus::connected)
        return;

    Buffer buffer(buf.data(),buf.length());
    bool result = session->AsyncSend(buffer);
    if(!result)
    {
    }
}

void ConnectManager::slots_PeerClose()
{
    status = ConnectStatus::disconnect;
}

void ConnectManager::slots_RecvMessage(QByteArray& recv)
{
    try{
        MSGMANAGER->ProcessMsg(&recv);
    }
    catch(...)
    {
        qDebug()<<"OnRecvMessage error when process data:"<<recv.data();
    }
}

