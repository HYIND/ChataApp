#include "ChatSessionModel.h"
#include <QDateTime>
#include "ModelManager.h"
#include "RequestManager.h"
#include "MsgManager.h"
#include <QFile>
#include "FileTransManager.h"

ChatSessionModel::ChatSessionModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_token = "";
    m_name = "";
    m_address = "";
}

int ChatSessionModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_messages.size();
}

QVariant ChatSessionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.size())
        return QVariant();

    const ChatMsg &msg = m_messages.at(index.row());

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
        {FileStatusRole,"filestatus"}
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

void ChatSessionModel::loadSession(const QString &token)
{
    beginResetModel();
    m_token = token;
    if(!CHATITEMMODEL->findbytoken(token))
        return;

    m_name = "";
    m_address = "";
    m_messages.clear();

    CHATITEMMODEL->activeSession(token);
    CHATITEMMODEL->getChatSessionDataByToken(token,m_name,m_address,m_messages);

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

void ChatSessionModel::fileTransProgressChange(const QString &fileid, uint32_t progress)
{
    for (int index=0;index<m_messages.size();index++)
    {
        if(m_messages[index].type!=MsgType::file)
            continue;
        if(m_messages[index].fileid == fileid)
        {
            m_messages[index].fileprogress = progress;
            m_messages[index].filestatus = FileStatus::Transing;
            QModelIndex modelIndex = createIndex(index, 0);
            emit dataChanged(modelIndex, modelIndex, {FileStatusRole,FileProgressRole});
            break;
        }
    }
}

void ChatSessionModel::fileTransInterrupted(const QString &fileid)
{
    for (int index=0;index<m_messages.size();index++)
    {
        if(m_messages[index].type!=MsgType::file)
            continue;
        if(m_messages[index].fileid == fileid)
        {
            QModelIndex modelIndex = createIndex(index, 0);
            m_messages[index].filestatus = FileStatus::Stop;
            emit dataChanged(modelIndex, modelIndex, {FileStatusRole,FileProgressRole});
            break;
        }
    }
}

void ChatSessionModel::fileTransFinished(const QString &fileid)
{
    for (int index=0;index<m_messages.size();index++)
    {
        if(m_messages[index].type!=MsgType::file)
            continue;
        if(m_messages[index].fileid == fileid)
        {
            m_messages[index].fileprogress = 100;
            m_messages[index].filestatus = FileStatus::Success;
            QModelIndex modelIndex = createIndex(index, 0);
            emit dataChanged(modelIndex, modelIndex, {FileStatusRole,FileProgressRole});
            break;
        }
    }
}

void ChatSessionModel::fileTransError(const QString &fileid)
{
    for (int index=0;index<m_messages.size();index++)
    {
        if(m_messages[index].type!=MsgType::file)
            continue;
        if(m_messages[index].fileid == fileid)
        {
            m_messages[index].fileprogress = 0;
            m_messages[index].filestatus = FileStatus::Fail;
            QModelIndex modelIndex = createIndex(index, 0);
            emit dataChanged(modelIndex, modelIndex, {FileStatusRole,FileProgressRole});
            break;
        }
    }
}

void ChatSessionModel::sendMessage(const QString& goaltoken, const QString &msg)
{
    REQUESTMANAGER->SendMsg(goaltoken,MsgType::text,msg);
}

void ChatSessionModel::sendPicture(const QString& goaltoken, const QString &url)
{
    QFile file(url);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "sendPicture error ,无法打开文件:" << url;
    }
    QByteArray fileData = file.readAll();
    file.close();

    QString base64content = QString::fromLatin1(fileData.toBase64());

    REQUESTMANAGER->SendMsg(goaltoken,MsgType::picture,base64content);
}

void ChatSessionModel::sendFile(const QString& goaltoken, const QString &url)
{
    QFile file(url);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "sendPicture error ,无法打开文件:" << url;
    }

    QString filepath = url;
    QString filename = getFilenameFromPath(file.fileName());
    QString fileid = QUuid::createUuid().toString();
    QString md5 = "";
    qint64 filesize = file.size();

    FILETRANSMANAGER->AddReqRecord(fileid,filepath,filesize,md5);
    REQUESTMANAGER->SendFile(goaltoken,filename,filesize,md5,fileid);

    file.close();
}

bool ChatSessionModel::isMyToken(const QString &token)
{
    return USERINFOMODEL->isMyToken(token);
}

void ChatSessionModel::startTrans(const QString &fileid)
{
    for (auto &chatmsg:m_messages)
    {
        if (chatmsg.fileid == fileid && chatmsg.type == MsgType::file)
        {
            if(USERINFOMODEL->isMyToken(chatmsg.srctoken))
            {
                FILETRANSMANAGER->ReqUploadFile(chatmsg.fileid);
            }
            else
            {
                FILETRANSMANAGER->ReqDownloadFile(chatmsg.fileid);
            }
            return;
        }
    }
}

void ChatSessionModel::stopTrans(const QString &fileid)
{
    for (auto &chatmsg:m_messages)
    {
        if (chatmsg.fileid == fileid && chatmsg.type == MsgType::file)
        {
            FILETRANSMANAGER->InterruptTask(chatmsg.fileid);
            return;
        }
    }
}
