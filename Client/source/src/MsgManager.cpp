#include "MsgManager.h"
#include "ConnectManager.h"
#include "ModelManager.h"
#include "FileTransManager.h"
#include "MessagePackage.h"

QString publicchattoken = "publicchat";

MsgManager* MsgManager::Instance()
{
	static MsgManager* instance = new MsgManager();
	return instance;
}

MsgManager::MsgManager() {}

MsgManager::~MsgManager() {}

void MsgManager::ProcessMsg(QByteArray* bytes)
{
	MessagePackage package;
	Buffer buf(bytes->data(), bytes->length());
	if (!AnalysisMessagePackageFromBuffer(&buf, &package))
		return;

	if (!package.jsonenable)
		return;

	json& js_src = package.nlmjson;
	Buffer& buf_src = package.bufferdata;

	QJsonDocument jdoc = package.ToQtJson();

	QJsonObject rootObj = jdoc.object();
	if (!rootObj.contains("command"))
	{
		qDebug() << "Json without command!,data:" << bytes->data();
	}
	QJsonValue jcommandValue = rootObj.value("command");
	int command = jcommandValue.toInt();

	if (command == 7000 || command == 7001 || command == 7010 || command == 7080 || command == 7070 ||
		command == 8000 || command == 8001 || command == 8010 || command == 5001)
	{
		FILETRANSMANAGER->ProcessMsg(js_src, buf_src);
		return;
	}

	switch (command) {
	case 2000:
		ProcessLoginInfo(rootObj);
		break;
	case 2001:
		ProcessOnlineUser(rootObj);
		break;
	case 2002:
        ProcessSendMsgReceipt(rootObj);
		break;
	case 2003:
		ProcessUserMsg(rootObj, buf_src);
		break;
	case 2004:
		ProcessUserMsgRecord(rootObj, buf_src);
		break;
	default:
		break;
	}
}

void MsgManager::ProcessLoginInfo(const QJsonObject& jsonObj)
{
    if (!jsonObj.contains("jwt") || !jsonObj.contains("token") || !jsonObj.contains("name") || !jsonObj.contains("ip") || !jsonObj.contains("port"))
	{
		QJsonDocument doc;
		doc.setObject(jsonObj);
		qDebug() << "ProcessLoginInfo error!,data:" << doc.toJson();
		return;
	}


    QString token = jsonObj.value("token").toString();
    QString jwt = jsonObj.value("jwt").toString();
	QString name = jsonObj.value("name").toString();
	QString ip = jsonObj.value("ip").toString();
	int port = jsonObj.value("port").toInt();

	QString address = ip + ":" + QString::number(port);

	m_ip = ip;
	m_port = port;

    USERINFOMODEL->setUserJwt(jwt);
	USERINFOMODEL->setUserToken(token);
	USERINFOMODEL->setUserName(name);
	USERINFOMODEL->setUserAddress(address);

	qDebug() << "token :" << jsonObj.value("token").toString();
	qDebug() << "name :" << jsonObj.value("name").toString();
	qDebug() << "ip :" << jsonObj.value("ip").toString();
	qDebug() << "port :" << jsonObj.value("port").toInt();
}

void MsgManager::ProcessOnlineUser(const QJsonObject& jsonObj)
{
	if (!jsonObj.contains("users") || !jsonObj.value("users").isArray())
	{
		QJsonDocument doc;
		doc.setObject(jsonObj);
		qDebug() << "ProcessOnlineUser error!,data:" << doc.toJson();
		return;
	}

	QList<ChatItemData> datas;
	QJsonArray juserarray = jsonObj.value("users").toArray();
	for (int i = 0; i < juserarray.size(); i++)
	{
		QJsonObject juserobject = juserarray.at(i).toObject();
		QString token = juserobject.value("token").toString();
		QString name = juserobject.value("name").toString();
		QString ip = juserobject.value("ip").toString();
		int port = juserobject.value("port").toInt();

		datas.emplaceBack(ChatItemData(token, name, ip + ":" + QString::number(port)));
	}
	CHATITEMMODEL->addChatItem(datas);
}

void MsgManager::ProcessUserMsg(const QJsonObject& jsonObj, Buffer& buf)
{
	if (!jsonObj.contains("srctoken")
		|| !jsonObj.contains("goaltoken")
		|| !jsonObj.contains("msg"))
		return;

	QString srctoken = jsonObj.value("srctoken").toString();
	QString goaltoken = jsonObj.value("goaltoken").toString();
	QString name = jsonObj.value("name").toString();
	qint64 timestamp = jsonObj.value("time").toInteger();
	QString ip = jsonObj.value("ip").toString();
	int port = jsonObj.value("port").toInt();
	MsgType type = (MsgType)jsonObj.value("type").toInt();
	QString msg = jsonObj.value("msg").toString();
	QDateTime time = QDateTime::fromSecsSinceEpoch(timestamp);

	QString address = ip + ":" + QString::number(port);

	QString filename;
	uint64_t filesize = 0;
	QString md5;
	QString fileid;
    if (type == MsgType::file || type == MsgType::picture)
	{
		filename = jsonObj.value("filename").toString();
		filesize = jsonObj.value("filesize").toInteger();
		md5 = jsonObj.value("md5").toString();
		fileid = jsonObj.value("fileid").toString();
	}
	if (type == MsgType::picture)
	{
		if (jsonObj.contains("bufferlen"))
		{
			qint64 bufferlen = jsonObj.value("bufferlen").toInteger();
            if (bufferlen > 0 && buf.Remain() >= bufferlen)
			{
				Buffer picbuffer;
				picbuffer.Append(buf, bufferlen);
				msg = QString::fromUtf8(picbuffer.Byte(), picbuffer.Length());
			}
		}
		else {
			if (buf.Length() > 0)
			{
				msg = QString::fromUtf8(buf.Byte(), buf.Length());
			}
		}
	}


	ChatMsg chatmsg(srctoken,
		name,
		address,
		time,
		type,
		msg,
		filename,
		filesize,
		md5,
		fileid);

	CHATITEMMODEL->addNewMsg(goaltoken, chatmsg);
}

void MsgManager::ProcessUserMsgRecord(const QJsonObject& jsonObj, Buffer& buf_src)
{
	if (!jsonObj.contains("messages") || !jsonObj.value("messages").isArray())
		return;

	QJsonArray js_messages = jsonObj.value("messages").toArray();

	for (auto it = js_messages.begin(); it != js_messages.end(); it++)
	{
		if (it->isObject())
		{
			QJsonObject msg = it->toObject();
			ProcessUserMsg(msg, buf_src);
		}
		else
		{
			return;
		}
	}
}

void MsgManager::ProcessSendMsgReceipt(const QJsonObject &jsonObj)
{
    if (!jsonObj.contains("result") || !jsonObj.value("result").isBool())
        return;
    bool result = jsonObj.value("result").toBool();
    if(!result)
    {
        CHATITEMMODEL->sendMsgError();
    }
}

bool MsgManager::isPublicChat(const QString& token)
{
	return token == publicchattoken;
}

QString MsgManager::PublicChatToken()
{
	return publicchattoken;
}



