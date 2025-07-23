#include "RequestManager.h"
#include "ConnectManager.h"
#include "MsgManager.h"
#include "ModelManager.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonParseError>

RequestManager *RequestManager::Instance()
{
    static RequestManager *instance = new RequestManager();
    return instance;
}

void RequestManager::SendMsg(const QString &goaltoken, const MsgType type, const QString &content)
{
    QJsonObject jsonObj;
    jsonObj.insert("command", 1002);
    jsonObj.insert("token",USERINFOMODEL->usertoken());
    jsonObj.insert("goaltoken",goaltoken);
    jsonObj.insert("type",(int)type);
    jsonObj.insert("msg",content);

    QJsonDocument doc;
    doc.setObject(jsonObj);
    QByteArray jsonbytes = doc.toJson(QJsonDocument::JsonFormat::Compact);
    CONNECTMANAGER->Send(jsonbytes);
}

void RequestManager::RequestOnlineUserData()
{
    QJsonObject jsonObj;
    jsonObj.insert("command", 1001);
    jsonObj.insert("token",USERINFOMODEL->usertoken());

    QJsonDocument doc;
    doc.setObject(jsonObj);
    QByteArray jsonbytes = doc.toJson(QJsonDocument::JsonFormat::Compact);
    CONNECTMANAGER->Send(jsonbytes);
}

RequestManager::RequestManager() {}

RequestManager::~RequestManager(){}



