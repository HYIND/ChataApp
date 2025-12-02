#pragma once

#include <QAbstractListModel>
#include <QVector>
#include "ChatItemListModel.h"

class ChatSessionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString sessionToken READ sessionToken NOTIFY sessionTokenChanged)
    Q_PROPERTY(QString sessionTitle READ sessionTitle NOTIFY sessionTitleChanged)
    Q_PROPERTY(QString sessionSubtitle READ sessionSubtitle NOTIFY sessionSubtitleChanged)

public:
    enum ChatMsgsRoles {
        SrcTokenRole,
        NameRole,
        AddressRole,
        TimeRole,
        TypeRole,
        MsgRole,
        FileNameRole,
        FileSizeStrRole,
        Md5Role,
        FileIdRole,
        FileProgressRole,
        FileStatusRole,
        FilePathRole
    };

    explicit ChatSessionModel(QObject *parent = nullptr);

    // 基本模型接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // 会话属性
    QString sessionToken() const;
    QString sessionTitle() const;
    QString sessionSubtitle() const;

    // 会话操作
    void loadSession(const QString &token);
    void addMessage(const ChatMsg& chatmsg);
    void clearSession();

    void fileTransProgressChange(const QString& fileid,uint32_t progress);
    void fileTransInterrupted(const QString& fileid);
    void fileTransFinished(const QString& fileid);
    void fileTransError(const QString& fileid);

public slots:
    void sendMessage(const QString& goaltoken, const QString& msg);
    void sendPicture(const QString& goaltoken, const QString &url);
    void sendFile(const QString& goaltoken, const QString &url);
    bool isMyToken(const QString& token);

    void startTrans(const QString& fileid);
    void stopTrans(const QString& fileid);

signals:
    void sessionTokenChanged();
    void sessionTitleChanged();
    void sessionSubtitleChanged();
    void newMessageAdded();

private:
    QString m_token;
    QString m_name;
    QString m_address;
    QVector<ChatMsg> m_messages;
};
