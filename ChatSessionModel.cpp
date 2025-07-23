#include "ChatSessionModel.h"
#include <QDateTime>
#include "ModelManager.h"
#include "RequestManager.h"
#include "MsgManager.h"
#include <QFile>

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
        {MsgRole, "msg"}
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

bool ChatSessionModel::isMyToken(const QString &token)
{
    return USERINFOMODEL->isMyToken(token);
}
