#include "MessagePackage.h"
#include <QByteArray>
#include <QDebug>

MessagePackage::MessagePackage()
{
	jsonenable = false;
	bufferenable = false;

	jsonlen = 0;
	bufferlen = 0;
}

MessagePackage::MessagePackage(json& nlmjson) : MessagePackage()
{
	SetJson(nlmjson);
}

MessagePackage::MessagePackage(QJsonObject& QJsonObj) : MessagePackage()
{
	SetJson(QJsonObj);
}

MessagePackage::MessagePackage(Buffer& buf) : MessagePackage()
{
	SetBuffer(buf);
}

MessagePackage::MessagePackage(QJsonObject& QJsonObj, Buffer& buf)
{
	SetJson(QJsonObj);
	SetBuffer(buf);
}

MessagePackage::MessagePackage(json& nlmjson, Buffer& buf) : MessagePackage()
{
	SetJson(nlmjson);
	SetBuffer(buf);
}

void MessagePackage::SetJson(json& nlmjson)
{
	jsonenable = true;
	this->nlmjson = nlmjson;
	jsondata.CopyFromBuf(Buffer(nlmjson.dump()));
	jsonlen = jsondata.Length();
}

void MessagePackage::SetJson(QJsonObject& QJsonObj)
{
	QJsonDocument doc;
	doc.setObject(QJsonObj);
	QByteArray jsonbytes = doc.toJson(QJsonDocument::JsonFormat::Compact);

	Buffer buf(jsonbytes.data(), jsonbytes.length());

	bool result = true;
	std::string str(buf.Byte(), buf.Length());
	try
	{
		this->nlmjson = json::parse(str);
	}
	catch (...)
	{
		result = false;
		//std::cout << fmt::format("MessagePackage::SetJson json::parse error : {}\n", str);
	}

	if (result)
	{
		jsonenable = true;
		jsondata.QuoteFromBuf(buf);
		jsonlen = jsondata.Length();
	}
}

void MessagePackage::SetBuffer(Buffer& buf)
{
	bufferenable = true;
	bufferdata.CopyFromBuf(buf);
	bufferlen = bufferdata.Length();
}

QJsonDocument MessagePackage::ToQtJson()
{
	if (!jsonenable || jsonlen == 0 || jsondata.Length() <= 0)
		return QJsonDocument();

	QByteArray bytes(jsondata.Byte(), jsondata.Length());
	QJsonParseError jsonError;
	QJsonDocument jdoc = QJsonDocument::fromJson(bytes, &jsonError);
	if (jsonError.error != QJsonParseError::NoError && !jdoc.isNull()) {
		qDebug() << "MessagePackage JsonParse解析错误,error:" << jsonError.error << " data:" << bytes.data();
		return QJsonDocument();
	}
	return jdoc;
}

bool AnalysisMessagePackageFromBuffer(Buffer* buf, MessagePackage* package)
{
    int oripos = buf->Position();

	bool result = true;

	package->jsonenable = false;
	package->bufferenable = false;

	package->jsonlen = 0;
	package->bufferlen = 0;

	package->jsondata.ReSize(0);
	package->bufferdata.ReSize(0);

	buf->Read(&(package->jsonlen), sizeof(package->jsonlen));
	buf->Read(&(package->bufferlen), sizeof(package->bufferlen));

	if (result && package->jsonlen > 0)
	{
        if (buf->Remain() >= package->jsonlen)
		{
			package->jsondata.Append(*buf, package->jsonlen);
			std::string str(package->jsondata.Byte(), package->jsonlen);
			try
			{
				package->nlmjson = json::parse(str);
			}
			catch (...)
			{
				result = false;
				//std::cout << fmt::format("GenerateMessagePackage json::parse error : {}\n", str);
			}
		}
		else
		{
			result = false;
		}
		package->jsonenable = result;
	}

	if (result && package->bufferlen > 0)
	{
        if (buf->Remain() >= package->bufferlen)
		{
			package->bufferdata.ReSize(package->bufferlen);
			package->bufferdata.WriteFromOtherBufferPos(*buf, package->bufferlen);
			package->bufferdata.Seek(0);
		}
		else
		{
			result = false;
		}
		package->bufferenable = result;
	}

	buf->Seek(oripos);

	return result;
}

void GenerateMessagePackageToBuffer(MessagePackage* package, Buffer* buf)
{
	package->bufferdata.Seek(0);
	package->jsondata.Seek(0);

	buf->ReSize(sizeof(package->jsonlen) + sizeof(package->bufferlen) + package->jsonlen + package->bufferlen);

	buf->Write(&(package->jsonlen), sizeof(package->jsonlen));
	buf->Write(&(package->bufferlen), sizeof(package->bufferlen));

	buf->WriteFromOtherBufferPos(package->jsondata);
	buf->WriteFromOtherBufferPos(package->bufferdata);
}

