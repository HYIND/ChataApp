#include "RequestManager.h"
#include "ConnectManager.h"
#include "MsgManager.h"
#include "ModelManager.h"
#include "NetWorkHelper.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonParseError>

RequestManager* RequestManager::Instance()
{
	static RequestManager* instance = new RequestManager();
	return instance;
}

void RequestManager::SendMsg(const QString& goaltoken, const MsgType type, const QString& content)
{
	if (type == MsgType::file)
		return;

	QJsonObject jsonObj;
	jsonObj.insert("command", 1002);
    jsonObj.insert("jwt", USERINFOMODEL->userjwt());
	jsonObj.insert("goaltoken", goaltoken);
	jsonObj.insert("type", (int)type);
	jsonObj.insert("msg", content);

	NetWorkHelper::SendMessagePackage(&jsonObj);
}

void RequestManager::SendPicture(const QString& goaltoken,
                                const QString& filename, int64_t filesize, const QString& md5, const QString& fileid,
                                Buffer& buf)
{
    QJsonObject jsonObj;
    jsonObj.insert("command", 1002);
    jsonObj.insert("jwt", USERINFOMODEL->userjwt());
    jsonObj.insert("goaltoken", goaltoken);
    jsonObj.insert("type", (int)MsgType::picture);
    jsonObj.insert("msg", "");

    jsonObj.insert("filename", filename);
    jsonObj.insert("filesize", (int64_t)filesize);
    jsonObj.insert("md5", md5);
    jsonObj.insert("fileid", fileid);

    NetWorkHelper::SendMessagePackage(&jsonObj, &buf);
}


void RequestManager::SendFile(const QString& goaltoken, const QString& filename, int64_t filesize, const QString& md5, const QString& fileid)
{
	QJsonObject jsonObj;
	jsonObj.insert("command", 1002);
    jsonObj.insert("jwt", USERINFOMODEL->userjwt());
	jsonObj.insert("goaltoken", goaltoken);
	jsonObj.insert("type", (int)MsgType::file);
	jsonObj.insert("msg", "");

	jsonObj.insert("filename", filename);
	jsonObj.insert("filesize", (int64_t)filesize);
	jsonObj.insert("md5", md5);
	jsonObj.insert("fileid", fileid);

	NetWorkHelper::SendMessagePackage(&jsonObj);
}

void RequestManager::RequestOnlineUserData()
{
	QJsonObject jsonObj;
	jsonObj.insert("command", 1001);
    jsonObj.insert("jwt", USERINFOMODEL->userjwt());

	NetWorkHelper::SendMessagePackage(&jsonObj);
}

void RequestManager::RequestMessageRecord(const QString& goaltoken)
{
	static QMap<QString, bool>requestrecord; //只请求一次，防止频繁请求
	if (requestrecord.find(goaltoken) != requestrecord.end())
		return;
	else
		requestrecord.insert(goaltoken, true);

	QJsonObject jsonObj;
	jsonObj.insert("command", 1003);
    jsonObj.insert("jwt", USERINFOMODEL->userjwt());
	jsonObj.insert("goaltoken", goaltoken);

	NetWorkHelper::SendMessagePackage(&jsonObj);
}

RequestManager::RequestManager() {}

RequestManager::~RequestManager() {}



