#ifndef MSGMANAGER_H
#define MSGMANAGER_H

#include "Buffer.h"
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonParseError>

class MsgManager
{
public:
    static MsgManager* Instance();

public:
    void ProcessMsg(QByteArray *bytes);

public:
    void ProcessLoginInfo(const QJsonObject& jsonObj);
    void ProcessOnlineUser(const QJsonObject& jsonObj);
    void ProcessUserMsg(const QJsonObject& jsonObj);

public:
    bool isPublicChat(const QString& token);
    QString PublicChatToken();

private:
    MsgManager();
    ~MsgManager();

public:
    QString m_ip;
    int m_port;
};

#define MSGMANAGER MsgManager::Instance()

#endif // MSGMANAGER_H
