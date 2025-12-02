#include "ModelManager.h"
#include "ChatItemListModel.h"
#include "MsgManager.h"
#include "FileTransManager.h"
#include "RequestManager.h"

QString getFileSizeStr(uint64_t filesize)
{
    const uint64_t KB = 1024;
    const uint64_t MB = KB * 1024;
    const uint64_t GB = MB * 1024;
    const uint64_t TB = GB * 1024;

    if (filesize < KB) {
        return QString("%1 B").arg(filesize);
    } else if (filesize < MB) {
        return QString("%1 KB").arg(QString::number(filesize / static_cast<double>(KB), 'f', 1));
    } else if (filesize < GB) {
        return QString("%1 MB").arg(QString::number(filesize / static_cast<double>(MB), 'f', 1));
    } else if (filesize < TB) {
        return QString("%1 GB").arg(QString::number(filesize / static_cast<double>(GB), 'f', 1));
    } else {
        return QString("%1 TB").arg(QString::number(filesize / static_cast<double>(TB), 'f', 1));
    }
}

ChatMsg::ChatMsg(const QString &srctoken, const QString &name, const QString &address,
                 const QDateTime &time, const MsgType type,const QString &msg,
                 const QString &filename, uint64_t filesize, const QString &md5, const QString &fileid)
    :srctoken(srctoken),name(name), address(address), time(time), type(type), msg(msg),filename(filename),filesize(filesize),md5(md5),fileid(fileid)
{
    filesizestr = getFileSizeStr(filesize);
}


ChatItemListModel::ChatItemListModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(this,&ChatItemListModel::signal_deleteChatItem,this,&ChatItemListModel::slot_deleteChatItem);
}

int ChatItemListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_chatitems.size();
}

QVariant ChatItemListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_chatitems.size())
        return QVariant();

    const ChatItemData &chatitem = m_chatitems.at(index.row());
    switch (role) {
    case TokenRole:
        return chatitem.token;
    case NameRole:
        return chatitem.name;
    case AddressRole:
        return chatitem.address;
    case IsOnlineRole:
        return chatitem.isOnline;
    case HasUnreadRole:
        return chatitem.hasUnread;
    case LastMsgUserNameRole:
        return chatitem.lastmsgusername;
    case LastMessageTimeRole:
        return chatitem.lastmsgtime;
    case LastMsgRole:
        return chatitem.lastmsg;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ChatItemListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TokenRole] = "token";
    roles[NameRole] = "name";
    roles[AddressRole] = "address";
    roles[IsOnlineRole] = "isonline";
    roles[HasUnreadRole] = "hasunread";
    roles[LastMsgUserNameRole] = "lastmsgusername";
    roles[LastMessageTimeRole] = "lastmsgtime";
    roles[LastMsgRole] = "lastmsg";
    return roles;
}

void ChatItemListModel::addChatItem(const ChatItemData &item)
{
    if(item.token == USERINFOMODEL->usertoken())
        return;
    if(findbytoken(item.token))
        return;
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_chatitems.append(item);
    endInsertRows();
}

void ChatItemListModel::addChatItem(const QList<ChatItemData> &items)
{
    for (auto& item:items)
    {
        if(item.token == USERINFOMODEL->usertoken())
            continue;
        if(findbytoken(item.token))
            continue;
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        m_chatitems.append(item);
        endInsertRows();
    }
    for(int i=0;i<m_chatitems.size();i++)
    {
        bool find = false;
        for(auto& item:items)
        {
            if(m_chatitems[i].token == item.token)
            {
                find = true;
                break;
            }
        }
        if(!find)
        {
            m_chatitems[i].isOnline = false;
            QModelIndex modelIndex = createIndex(i, 0);
            emit dataChanged(modelIndex, modelIndex, {IsOnlineRole});
        }
    }
}

void ChatItemListModel::addNewMsg(const QString& goaltoken,const ChatMsg& chatmsg)
{

    int index = -1;
    QString itemtoken;

    if(MSGMANAGER->isPublicChat(goaltoken))
        itemtoken = goaltoken;
    else
        itemtoken = !USERINFOMODEL->isMyToken(chatmsg.srctoken) ? chatmsg.srctoken : goaltoken;

    for (int i=0;i<m_chatitems.size();i++)
    {
        if(m_chatitems[i].token==itemtoken)
        {
            index = i;
        }
    }

    if(index==-1)
    {
        ChatItemData item(itemtoken,chatmsg.name,chatmsg.address);
        index = m_chatitems.size() - 1;
        addChatItem(item);
    }

    if(!m_chatitems[index].addMessage(chatmsg))
        return;

    m_chatitems[index].lastmsgusername = chatmsg.name;
    m_chatitems[index].lastmsgtime = chatmsg.time;
    if (chatmsg.type == MsgType::picture)
        m_chatitems[index].lastmsg="[图片]";
    else if (chatmsg.type == MsgType::file)
        m_chatitems[index].lastmsg="[文件]";
    else
        m_chatitems[index].lastmsg=chatmsg.msg;

    if(SESSIONMODEL->sessionToken()!=itemtoken && !USERINFOMODEL->isMyToken(chatmsg.srctoken))
        m_chatitems[index].hasUnread = true;

    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {HasUnreadRole,LastMsgUserNameRole,LastMessageTimeRole,LastMsgRole});

    emit layoutAboutToBeChanged();

    QModelIndexList oldIndexes = persistentIndexList();  // 获取当前持久化索引（如选中项）

    sortItemsByLastMessage();

    int curindex = -1;
    if(findbytoken(SESSIONMODEL->sessionToken(),&curindex))
        emit setCurrentIndex(curindex);

    emit layoutChanged();

    if(SESSIONMODEL->sessionToken()==itemtoken)
        SESSIONMODEL->addMessage(chatmsg);

    if (chatmsg.type == MsgType::file)
    {
        if(USERINFOMODEL->isMyToken(chatmsg.srctoken))
        {
            FILETRANSMANAGER->ReqUploadFile(chatmsg.fileid);
        }
        else
        {
            QString filepath = FILETRANSMANAGER->DownloadDir() + chatmsg.filename;
            FILETRANSMANAGER->AddReqRecord(chatmsg.fileid, filepath, chatmsg.filesize, chatmsg.md5);
        }
    }
}

void ChatItemListModel::fileTransProgressChange(const QString &fileid, uint32_t progress)
{
    bool find = false;
    QString itemtoken;
    for (int i=0;i<m_chatitems.size();i++)
    {
        if(find)
            break;
        for(auto & msg:m_chatitems[i].chatmsgs)
        {
            if(msg.type!=MsgType::file)
                continue;
            if(msg.fileid == fileid)
            {
                find = true;
                itemtoken = m_chatitems[i].token;
                msg.fileprogress = progress;
                msg.filestatus = FileStatus::Transing;
                break;
            }
        }
    }
    if (find)
    {
        if(SESSIONMODEL->sessionToken()==itemtoken)
            SESSIONMODEL->fileTransProgressChange(fileid,progress);
    }
}

void ChatItemListModel::fileTransInterrupted(const QString &fileid)
{
    bool find = false;
    QString itemtoken;
    for (int i=0;i<m_chatitems.size();i++)
    {
        if(find)
            break;
        for(auto & msg:m_chatitems[i].chatmsgs)
        {
            if(msg.type!=MsgType::file)
                continue;
            if(msg.fileid == fileid)
            {
                find = true;
                itemtoken = m_chatitems[i].token;
                msg.filestatus = FileStatus::Stop;
                break;
            }
        }
    }
    if (find)
    {
        if(SESSIONMODEL->sessionToken()==itemtoken)
            SESSIONMODEL->fileTransInterrupted(fileid);
    }
}

void ChatItemListModel::fileTransFinished(const QString &fileid)
{
    bool find = false;
    QString itemtoken;
    for (int i=0;i<m_chatitems.size();i++)
    {
        if(find)
            break;
        for(auto & msg:m_chatitems[i].chatmsgs)
        {
            if(msg.type!=MsgType::file)
                continue;
            if(msg.fileid == fileid)
            {
                find = true;
                itemtoken = m_chatitems[i].token;
                msg.fileprogress = 100;
                msg.filestatus = FileStatus::Success;
                msg.filepath = FILETRANSMANAGER->FindDownloadPathByFileId(fileid);
                break;
            }
        }
    }
    if (find)
    {
        if(SESSIONMODEL->sessionToken()==itemtoken)
            SESSIONMODEL->fileTransFinished(fileid);
    }
}

void ChatItemListModel::fileTransError(const QString &fileid)
{
    bool find = false;
    QString itemtoken;
    for (int i=0;i<m_chatitems.size();i++)
    {
        if(find)
            break;
        for(auto & msg:m_chatitems[i].chatmsgs)
        {
            if(msg.type!=MsgType::file)
                continue;
            if(msg.fileid == fileid)
            {
                find = true;
                itemtoken = m_chatitems[i].token;
                msg.fileprogress = 0;
                msg.filestatus = FileStatus::Fail;
                break;
            }
        }
    }
    if (find)
    {
        if(SESSIONMODEL->sessionToken()==itemtoken)
            SESSIONMODEL->fileTransError(fileid);
    }
}

bool ChatItemListModel::findbytoken(const QString &token,int* pindex)
{
    bool result = false;
    for (int i=0;i<m_chatitems.size();i++)
    {
        if(m_chatitems[i].token==token)
        {
            result = true;
            if(pindex!=nullptr)
                *pindex = i;
            break;
        }
    }

    return result;
}

bool ChatItemListModel::getChatSessionDataByToken(const QString &token, QString &name, QString &address, QList<ChatMsg> &chatmsgs)
{
    for (int i=0;i<m_chatitems.size();i++)
    {
        if(m_chatitems[i].token==token)
        {
            name = m_chatitems[i].name;
            address = m_chatitems[i].address;
            chatmsgs = m_chatitems[i].chatmsgs;
            return true;
        }
    }
    return false;
}

void ChatItemListModel::activeSession(const QString &token)
{
    int index = -1;
    if(findbytoken(token,&index))
    {
        m_chatitems[index].hasUnread = false;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {HasUnreadRole});
    }
}

void ChatItemListModel::handleItemClicked(const QString &token)
{
    SESSIONMODEL->loadSession(token);
    REQUESTMANAGER->RequestMessageRecord(token);
}

bool ChatItemListModel::isPublicChatItem(const QString &token)
{
    return MSGMANAGER->isPublicChat(token);
}

bool ChatItemListModel::getMsgsByToken(const QString &token,QList<ChatMsg> &chatmsgs)
{
    for (int i=0;i<m_chatitems.size();i++)
    {
        if(m_chatitems[i].token==token)
        {
            chatmsgs = m_chatitems[i].chatmsgs;
            return true;
        }
    }
    return false;
}

void ChatItemListModel::slot_deleteChatItem(const QString &token)
{
    for (auto it=m_chatitems.begin();it!=m_chatitems.end();it++)
    {
        if((*it).token==token)
        {
            m_chatitems.erase(it);
            break;
        }
    }
}

void ChatItemListModel::sortItemsByLastMessage()
{
    beginResetModel();
    QString PublicChatToken = MSGMANAGER->PublicChatToken();
    std::sort(m_chatitems.begin(), m_chatitems.end(),
              [&](const ChatItemData &a, const ChatItemData &b) {
                if(a.token == PublicChatToken)
                    return true;
                if(b.token == PublicChatToken)
                    return false;
                return a.lastmsgtime > b.lastmsgtime;  // 按时间降序排列
              });
    endResetModel();
}

