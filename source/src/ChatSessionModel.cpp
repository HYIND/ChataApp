#include "ChatSessionModel.h"
#include <QDateTime>
#include "ModelManager.h"
#include "RequestManager.h"
#include <QFile>
#include "FileTransManager.h"
#include <QUuid>
#include <QImage>
#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QTimer>

QString generateThumbnailBase64(const QString& imagePath,
                                int maxWidth = 320,
                                int maxHeight = 240,
                                int quality = 85,
                                const QString& format = "JPEG") {

    // 1. 加载图片
    QImage image(imagePath);
    if (image.isNull()) {
        qWarning() << "无法加载图片:" << imagePath;
        return QString();
    }

    // 2. 计算缩略图尺寸（保持纵横比）
    QSize originalSize = image.size();
    QSize thumbnailSize = originalSize;

    if (originalSize.width() > maxWidth || originalSize.height() > maxHeight) {
        thumbnailSize = originalSize.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio);
    }

    // 3. 生成缩略图（高质量缩放）
    QImage thumbnail;
    if (thumbnailSize != originalSize) {
        // 使用平滑变换
        thumbnail = image.scaled(thumbnailSize,
                                 Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
    } else {
        thumbnail = image;
    }

    // 4. 可选：转换为RGB格式节省空间（如果原图是RGBA）
    if (thumbnail.hasAlphaChannel()) {
        thumbnail = thumbnail.convertToFormat(QImage::Format_RGB888);
    }

    // 5. 保存为base64
    QByteArray imageBytes;
    QBuffer buffer(&imageBytes);
    buffer.open(QIODevice::WriteOnly);

    if (thumbnail.save(&buffer, format.toUtf8().constData(), quality)) {
        QString base64 = QString::fromLatin1(imageBytes.toBase64());

        // 调试信息
        // qDebug() << "缩略图生成完成:";
        // qDebug() << "原图尺寸:" << originalSize << "大小:" << QFile(imagePath).size() / 1024 << "KB";
        // qDebug() << "缩略图尺寸:" << thumbnailSize << "大小:" << imageBytes.size() / 1024 << "KB";
        // qDebug() << "压缩比例:" << (imageBytes.size() * 100.0 / QFile(imagePath).size()) << "%";

        return base64;
    }

    return QString();
}

bool openExplorerAndSelectFile(const QString& filePath) {
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        return false;
    }

    std::wstring wpath = QDir::toNativeSeparators(
                             fileInfo.absoluteFilePath()).toStdWString();

    // 使用 ShellExecute
    HINSTANCE result = ShellExecuteW(
        NULL,                    // 父窗口句柄
        L"open",                 // 操作
        L"explorer",             // 程序
        (L"/select,\"" + wpath + L"\"").c_str(),  // 参数
        NULL,                    // 工作目录
        SW_SHOWNORMAL            // 显示方式
        );

    return (int)result > 32;
}

ChatSessionModel::ChatSessionModel(QObject* parent)
	: QAbstractListModel(parent)
{
	m_token = "";
	m_name = "";
	m_address = "";
}

int ChatSessionModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return m_messages.size();
}

QVariant ChatSessionModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || index.row() >= m_messages.size())
		return QVariant();

	const ChatMsg& msg = m_messages.at(index.row());

	switch (role) {
	case SrcTokenRole:
		return msg.srctoken;
	case NameRole:
		return msg.name;
	case AddressRole:
		return msg.address;
	case TimeRole:
		return msg.time;
	case TypeRole:
		return (int)msg.type;
	case MsgRole:
		return msg.msg;
	case FileNameRole:
		return msg.filename;
	case FileSizeStrRole:
		return msg.filesizestr;
	case Md5Role:
		return msg.md5;
	case FileIdRole:
		return msg.fileid;
	case FileProgressRole:
		return msg.fileprogress;
	case FileStatusRole:
		return (int)msg.filestatus;
    case FilePathRole:
        return msg.filepath;
	default:
		return QVariant();
	}
}

QHash<int, QByteArray> ChatSessionModel::roleNames() const
{
	static QHash<int, QByteArray> roles = {
		{SrcTokenRole,"srctoken"},
		{NameRole, "name"},
		{AddressRole, "address"},
		{TimeRole, "time"},
		{TypeRole, "type"},
		{MsgRole, "msg"},
		{FileNameRole,"filename"},
		{FileSizeStrRole,"filesizestr"},
		{Md5Role,"md5"},
		{FileIdRole,"fileid"},
		{FileProgressRole,"fileprogress"},
        {FileStatusRole,"filestatus"},
        {FilePathRole,"filepath"}
	};
	return roles;
}

QString ChatSessionModel::sessionToken() const
{
	return m_token;
}

QString ChatSessionModel::sessionTitle() const
{
	return m_name;
}

QString ChatSessionModel::sessionSubtitle() const
{
	return m_address;
}

void ChatSessionModel::loadSession(const QString& token)
{
	beginResetModel();
	m_token = token;
	if (!CHATITEMMODEL->findbytoken(token))
		return;

	m_name = "";
	m_address = "";
	m_messages.clear();

	CHATITEMMODEL->activeSession(token);
	CHATITEMMODEL->getChatSessionDataByToken(token, m_name, m_address, m_messages);

	endResetModel();
	emit sessionTokenChanged();
	emit sessionTitleChanged();
	emit sessionSubtitleChanged();
	emit newMessageAdded();
}

void ChatSessionModel::addMessage(const ChatMsg& chatmsg)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());

	m_messages.append(chatmsg);

	endInsertRows();
	emit newMessageAdded();
}

void ChatSessionModel::clearSession()
{
	beginResetModel();
	m_token = "";
	m_name = "";
	m_address = "";
	m_messages.clear();
	emit sessionTokenChanged();
	endResetModel();
}

void ChatSessionModel::fileTransProgressChange(const QString& fileid, uint32_t progress)
{
	for (int index = 0; index < m_messages.size(); index++)
	{
        if (m_messages[index].type != MsgType::file && m_messages[index].type != MsgType::picture)
			continue;
		if (m_messages[index].fileid == fileid)
		{
			m_messages[index].fileprogress = progress;
			m_messages[index].filestatus = FileStatus::Transing;
			QModelIndex modelIndex = createIndex(index, 0);
			emit dataChanged(modelIndex, modelIndex, { FileStatusRole,FileProgressRole });
			break;
		}
	}
}

void ChatSessionModel::fileTransInterrupted(const QString& fileid)
{
	for (int index = 0; index < m_messages.size(); index++)
	{
        if (m_messages[index].type != MsgType::file && m_messages[index].type != MsgType::picture)
			continue;
		if (m_messages[index].fileid == fileid)
		{
			QModelIndex modelIndex = createIndex(index, 0);
			m_messages[index].filestatus = FileStatus::Stop;
			emit dataChanged(modelIndex, modelIndex, { FileStatusRole,FileProgressRole });
			break;
		}
	}
}

void ChatSessionModel::fileTransFinished(const QString& fileid)
{
	for (int index = 0; index < m_messages.size(); index++)
	{
        if (m_messages[index].type != MsgType::file && m_messages[index].type != MsgType::picture)
			continue;
		if (m_messages[index].fileid == fileid)
		{
			m_messages[index].fileprogress = 100;
			m_messages[index].filestatus = FileStatus::Success;
            m_messages[index].filepath = FILETRANSMANAGER->FindDownloadPathByFileId(fileid);
			QModelIndex modelIndex = createIndex(index, 0);
			emit dataChanged(modelIndex, modelIndex, { FileStatusRole,FileProgressRole });
			break;
		}
	}
}

void ChatSessionModel::fileTransError(const QString& fileid)
{
	for (int index = 0; index < m_messages.size(); index++)
	{
		if (m_messages[index].type != MsgType::file)
			continue;
        if (m_messages[index].fileid == fileid && m_messages[index].type != MsgType::picture)
		{
			m_messages[index].fileprogress = 0;
			m_messages[index].filestatus = FileStatus::Fail;
			QModelIndex modelIndex = createIndex(index, 0);
			emit dataChanged(modelIndex, modelIndex, { FileStatusRole,FileProgressRole });
			break;
		}
	}
}

void ChatSessionModel::sendMessage(const QString& goaltoken, const QString& msg)
{
	REQUESTMANAGER->SendMsg(goaltoken, MsgType::text, msg);
}

void ChatSessionModel::sendPicture(const QString& goaltoken, const QString& url)
{
	QFile file(url);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "sendPicture error ,无法打开文件:" << url;
        return;
	}

    QString filepath = url;
    QString filename = getFilenameFromPath(file.fileName());
    QString fileid = QUuid::createUuid().toString();
    QString md5 = "";
    qint64 filesize = file.size();

	file.close();

    QString base64Thumbnailcontent = generateThumbnailBase64(url);  //略缩图
    QByteArray bytes = base64Thumbnailcontent.toUtf8();
	Buffer buf(bytes.data(), bytes.length());

    FILETRANSMANAGER->AddReqRecord(fileid, filepath, filesize, md5);
    REQUESTMANAGER->SendPicture(goaltoken, filename, filesize, md5, fileid,buf);
}

void ChatSessionModel::sendFile(const QString& goaltoken, const QString& url)
{
	QFile file(url);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "sendPicture error ,无法打开文件:" << url;
        return;
	}

	QString filepath = url;
	QString filename = getFilenameFromPath(file.fileName());
	QString fileid = QUuid::createUuid().toString();
	QString md5 = "";
	qint64 filesize = file.size();

    file.close();

	FILETRANSMANAGER->AddReqRecord(fileid, filepath, filesize, md5);
	REQUESTMANAGER->SendFile(goaltoken, filename, filesize, md5, fileid);
}

bool ChatSessionModel::isMyToken(const QString& token)
{
	return USERINFOMODEL->isMyToken(token);
}

void ChatSessionModel::startTrans(const QString& fileid)
{
	for (auto& chatmsg : m_messages)
	{
        if (chatmsg.fileid == fileid && (chatmsg.type == MsgType::file || chatmsg.type == MsgType::picture))
		{
			if (USERINFOMODEL->isMyToken(chatmsg.srctoken))
			{
                CHATITEMMODEL->fileTransProgressChange(fileid,0);

                // FILETRANSMANAGER->ReqUploadFile(chatmsg.fileid);

                QTimer* timer = new QTimer();
                timer->setSingleShot(true);
                QObject::connect(timer, &QTimer::timeout, [this, id = chatmsg.fileid]() {
                    FILETRANSMANAGER->ReqUploadFile(id);
                });
                timer->start(300);
			}
			else
			{
                CHATITEMMODEL->fileTransProgressChange(fileid,0);

                // FILETRANSMANAGER->ReqDownloadFile(chatmsg.fileid);

                QTimer* timer = new QTimer();
                timer->setSingleShot(true);
                QObject::connect(timer, &QTimer::timeout, [this, id = chatmsg.fileid]() {
                    FILETRANSMANAGER->ReqDownloadFile(id);
                });
                timer->start(300);
			}
			return;
		}
	}
}

void ChatSessionModel::stopTrans(const QString& fileid)
{
	for (auto& chatmsg : m_messages)
	{
        if (chatmsg.fileid == fileid && (chatmsg.type == MsgType::file || chatmsg.type == MsgType::picture))
		{
			FILETRANSMANAGER->InterruptTask(chatmsg.fileid);
			return;
		}
	}
}

void ChatSessionModel::selectFileinExplore(const QString &filepath)
{
    bool success = openExplorerAndSelectFile(filepath);
}
