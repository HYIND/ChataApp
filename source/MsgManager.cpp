#include "MsgManager.h"
#include "ConnectManager.h"
#include "ModelManager.h"

QString publicchattoken = "publicchat";

MsgManager *MsgManager::Instance()
{
    static MsgManager *instance = new MsgManager();
    return instance;
}

MsgManager::MsgManager() {}

MsgManager::~MsgManager(){}

void MsgManager::ProcessMsg(QByteArray *bytes)
{
    QJsonParseError jsonError;
    QJsonDocument jdoc=QJsonDocument::fromJson(*bytes, &jsonError);
    if (jsonError.error != QJsonParseError::NoError && !jdoc.isNull()) {
        qDebug() << "JsonParse解析错误,error:" << jsonError.error<<" data:"<<bytes->data();
        return;
    }

    QJsonObject rootObj = jdoc.object();
    if(!rootObj.contains("command"))
    {
        qDebug() << "Json without command!,data:" <<bytes->data();
    }
    QJsonValue jcommandValue = rootObj.value("command");
    int command = jcommandValue.toInt();

    switch (command) {
    case 2000:
        ProcessLoginInfo(rootObj);
        break;
    case 2001:
        ProcessOnlineUser(rootObj);
        break;
    case 2002:
        break;
    case 2003:
        ProcessUserMsg(rootObj);
    default:
        break;
    }
}

void MsgManager::ProcessLoginInfo(const QJsonObject &jsonObj)
{
    if(!jsonObj.contains("token")||!jsonObj.contains("name")||!jsonObj.contains("ip")||!jsonObj.contains("port"))
    {
        QJsonDocument doc;
        doc.setObject(jsonObj);
        qDebug() << "ProcessLoginInfo error!,data:" <<doc.toJson();
        return;
    }


    QString token = jsonObj.value("token").toString();
    QString name = jsonObj.value("name").toString();
    QString ip = jsonObj.value("ip").toString();
    int port = jsonObj.value("port").toInt();

    QString address = ip+":"+QString::number(port);

    m_ip = ip;
    m_port = port;

    USERINFOMODEL->setUserToken(token);
    USERINFOMODEL->setUserName(name);
    USERINFOMODEL->setUserAddress(address);

    qDebug()<<"token :"<<jsonObj.value("token").toString();
    qDebug()<<"name :"<<jsonObj.value("name").toString();
    qDebug()<<"ip :"<<jsonObj.value("ip").toString();
    qDebug()<<"port :"<<jsonObj.value("port").toInt();
}

void MsgManager::ProcessOnlineUser(const QJsonObject &jsonObj)
{
    if(!jsonObj.contains("users")||!jsonObj.value("users").isArray())
    {
        QJsonDocument doc;
        doc.setObject(jsonObj);
        qDebug() << "ProcessOnlineUser error!,data:" <<doc.toJson();
        return;
    }

    QList<ChatItemData> datas;
    QJsonArray juserarray = jsonObj.value("users").toArray();
    for(int i=0;i<juserarray.size();i++)
    {
        QJsonObject juserobject = juserarray.at(i).toObject();
        QString token = juserobject.value("token").toString();
        QString name = juserobject.value("name").toString();
        QString ip = juserobject.value("ip").toString();
        int port = juserobject.value("port").toInt();

        datas.emplaceBack(ChatItemData(token,name,ip +":" +QString::number(port)));
    }
    CHATITEMMODEL->addChatItem(datas);
}

void MsgManager::ProcessUserMsg(const QJsonObject &jsonObj)
{
    if(!jsonObj.contains("srctoken")
        ||!jsonObj.contains("goaltoken")
        ||!jsonObj.contains("msg"))
        return;

    QString srctoken=jsonObj.value("srctoken").toString();
    QString goaltoken=jsonObj.value("goaltoken").toString();
    QString name = jsonObj.value("name").toString();
    qint64 timestamp = jsonObj.value("time").toInteger();
    QString ip=jsonObj.value("ip").toString();
    int port=jsonObj.value("port").toInt();
    MsgType type=(MsgType)jsonObj.value("type").toInt();
    QString msg=jsonObj.value("msg").toString();
    QDateTime time =QDateTime::fromSecsSinceEpoch(timestamp);

    QString address = ip+":"+QString::number(port);

    CHATITEMMODEL->addNewMsg(srctoken,goaltoken,time,name,address,type,msg);
}

bool MsgManager::isPublicChat(const QString& token)
{
    return token == publicchattoken;
}

QString MsgManager::PublicChatToken()
{
    return publicchattoken;
}



