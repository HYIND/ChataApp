#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QDateTime>

enum class MsgType{
    text = 1,
    picture =2 ,
    file =3
};

enum class FileStatus{
    Stop = 0,
    Transing = 1,
    Success = 2,
    Fail = 3
};

class ChatMsg{

public:
    ChatMsg() = default;
    ChatMsg(const QString& srctoken, const QString& name, const QString& address,
            const QDateTime& time,const MsgType type ,const QString& msg,
            const QString& filename="",uint64_t filesize=0, const QString& md5 = "", const QString& fileid = "");

    // 添加比较运算符便于排序
    bool operator<(const ChatMsg& other) const {
        return time < other.time;
    }

    // 添加转换为QVariantMap的方法，便于QML使用
    QVariantMap toVariantMap() const {
        return {
            {"srctoken",srctoken},
            {"name", name},
            {"address", address},
            {"time", time},
            {"msg", msg},
            {"filename", filename},
            {"filesizestr", filesizestr},
            {"md5",md5},
            {"fileid",fileid},
            {"fileprogress",fileprogress},
            {"filepath",filepath}
        };
    }


public:
    QString srctoken;
    QString name;
    QString address;
    QDateTime time;
    MsgType type;
    QString msg;
    QString filename;
    uint64_t filesize;
    QString filesizestr;
    QString md5;
    QString fileid;
    uint32_t fileprogress=0;
    FileStatus filestatus=FileStatus::Stop;
    QString filepath;
};

class ChatItemData {

public:
    ChatItemData() = default;
    ChatItemData(const QString& token, const QString& name,
                 const QString& address)
        : token(token), name(name), address(address),isOnline(true),hasUnread(false) {}

    // 添加消息处理方法
    bool addMessage(const ChatMsg& msg) {
        // 检查是否已存在相同记录
        bool isDuplicate = std::any_of(
            chatmsgs.begin(),
            chatmsgs.end(),
            [&msg](const ChatMsg& existingMsg) {
                return msg.srctoken == existingMsg.srctoken && msg.time == existingMsg.time && msg.type == existingMsg.type;
            }
            );

        if (!isDuplicate) {
            chatmsgs.append(msg);
            std::sort(chatmsgs.begin(), chatmsgs.end());
        }
        return !isDuplicate;
    }

    // 转换为QVariantMap
    QVariantMap toVariantMap() const {
        return {
            {"token", token},
            {"name", name},
            {"address", address},
            {"isOnline",isOnline},
            {"hasUnread",hasUnread},
            {"lastMsgUsername", lastmsgusername},
            {"lastMsgTime", lastmsgtime},
            {"lastMsg", lastmsg},
            {"messageCount", chatmsgs.size()}
        };
    }

public:
    QString token;
    QString name;
    QString address;
    bool isOnline;

    bool hasUnread;
    QString lastmsgusername;
    QDateTime lastmsgtime;
    QString lastmsg;

    QList <ChatMsg> chatmsgs;
};

class ChatItemListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum UserRoles {
        TokenRole = Qt::UserRole + 1,
        NameRole,
        AddressRole,
        IsOnlineRole,
        HasUnreadRole,
        LastMsgUserNameRole,
        LastMessageTimeRole,
        LastMsgRole
    };

    explicit ChatItemListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addChatItem(const ChatItemData &item);
    void addChatItem(const QList<ChatItemData> &item);
    void deleteChatItem(const QString& token);
    void addNewMsg(const QString& goaltoken,const ChatMsg& chatmsg);
    void sendMsgError();

    void fileTransProgressChange(const QString& fileid,uint32_t progress);
    void fileTransInterrupted(const QString& fileid);
    void fileTransFinished(const QString& fileid);
    void fileTransError(const QString& fileid);

public:
    bool findbytoken(const QString& token,int* pindex=nullptr);
    bool findtokenbyname(const QString &name, QString &token);

    const QVector<ChatItemData>& getAllUserInfo();
    bool isUseronline(const QString &token);

    bool getMsgsByToken(const QString& token,QList <ChatMsg>& chatmsgs);
    bool getChatSessionDataByToken(const QString& token, QString& name, QString& address, QList <ChatMsg>& chatmsgs);
    void activeSession(const QString& token);

public slots:
    void handleItemClicked(const QString& token);
    bool isPublicChatItem(const QString& token);

private:
    void sortItemsByLastMessage();

signals:
    void setCurrentIndex(int index);
    void signal_deleteChatItem(const QString& token);
    void msgError();

public slots:
    void slot_deleteChatItem(const QString& token);

private:
    QVector<ChatItemData> m_chatitems;
};
