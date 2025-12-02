#include "NetWorkHelper.h"
#include "ConnectManager.h"

bool NetWorkHelper::SendMessagePackage(json* json)
{
	MessagePackage package(*json);
	return NetWorkHelper::SendMessagePackage(&package);
}

bool NetWorkHelper::SendMessagePackage(QJsonObject* jsonObj)
{
	MessagePackage package(*jsonObj);
	return NetWorkHelper::SendMessagePackage(&package);
}

bool NetWorkHelper::SendMessagePackage(Buffer* buf)
{
	MessagePackage package(*buf);
	return NetWorkHelper::SendMessagePackage(&package);
}

bool NetWorkHelper::SendMessagePackage(json* json, Buffer* buf)
{
	MessagePackage package(*json, *buf);
	return NetWorkHelper::SendMessagePackage(&package);
}

bool NetWorkHelper::SendMessagePackage(QJsonObject* jsonObj, Buffer* buf)
{
	MessagePackage package(*jsonObj, *buf);
	return NetWorkHelper::SendMessagePackage(&package);
}

bool NetWorkHelper::SendMessagePackage(MessagePackage* package)
{
	Buffer buf;
	GenerateMessagePackageToBuffer(package, &buf);
	QByteArray bytes(buf.Byte(), buf.Length());
	CONNECTMANAGER->Send(bytes);
	return true;
}