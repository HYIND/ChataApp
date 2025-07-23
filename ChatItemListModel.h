#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QDateTime>

enum class MsgType{
    text = 1,
    picture
};

class ChatMsg{

public:
    ChatMsg() = default;
    ChatMsg(const QString& srctoken, const QString& name, const QString& address,
            const QDateTime& time,const MsgType type ,const QString& msg)
        : name(name), address(address), time(time), type(type), msg(msg){}

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
            {"msg", msg}
        };
    }


public:
    QString srctoken;
    QString name;
    QString address;
    QDateTime time;
    MsgType type;
    QString msg;
};

class ChatItemData {

public:
    ChatItemData() = default;
    ChatItemData(const QString& token, const QString& name,
                 const QString& address)
        : token(token), name(name), address(address),isOnline(true),hasUnread(false) {}

    // 添加消息处理方法
    void addMessage(const ChatMsg& msg) {
        chatmsgs.append(msg);
        // 可以在这里自动排序
        std::sort(chatmsgs.begin(), chatmsgs.end());
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
    void addNewMsg(const QString& srctoken, const QString& goaltoken,const QDateTime& time, const QString& name, const QString& address, const MsgType type, const QString& msg);

public:
    bool findbytoken(const QString& token,int* pindex=nullptr);
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

public slots:
    void slot_deleteChatItem(const QString& token);

private:
    QVector<ChatItemData> m_chatitems;
};
