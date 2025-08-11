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
    if(type == MsgType::file)
        return;

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

void RequestManager::SendFile(const QString &goaltoken, const QString &filename, int64_t filesize, const QString& md5,const QString& fileid)
{
    QJsonObject jsonObj;
    jsonObj.insert("command", 1002);
    jsonObj.insert("token",USERINFOMODEL->usertoken());
    jsonObj.insert("goaltoken",goaltoken);
    jsonObj.insert("type",(int)MsgType::file);
    jsonObj.insert("msg","");

    jsonObj.insert("filename",filename);
    jsonObj.insert("filesize",(int64_t)filesize);
    jsonObj.insert("md5",md5);
    jsonObj.insert("fileid",fileid);

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

void RequestManager::RequestMessageRecord(const QString &goaltoken)
{
    static QMap<QString,bool>requestrecord; //只请求一次，防止频繁请求
    if(requestrecord.find(goaltoken)!=requestrecord.end())
        return;
    else
        requestrecord.insert(goaltoken,true);

    QJsonObject jsonObj;
    jsonObj.insert("command", 1003);
    jsonObj.insert("token",USERINFOMODEL->usertoken());
    jsonObj.insert("goaltoken",goaltoken);

    QJsonDocument doc;
    doc.setObject(jsonObj);
    QByteArray jsonbytes = doc.toJson(QJsonDocument::JsonFormat::Compact);
    CONNECTMANAGER->Send(jsonbytes);
}

RequestManager::RequestManager() {}

RequestManager::~RequestManager(){}



