#ifndef USERINFOMODEL_H
#define USERINFOMODEL_H

#include <QAbstractListModel>

class UserInfoModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString usertoken READ usertoken NOTIFY usertokenChanged)
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    Q_PROPERTY(QString useraddress READ useraddress NOTIFY useraddressChanged)

public:
    enum UserRoles {
        UserTokenRole = Qt::UserRole + 1,
        UserNameRole,
        UserAddressRole,
    };

    UserInfoModel();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public:
    QString usertoken() const;
    QString username() const;
    QString useraddress() const;

    void setUserToken(const QString& token);
    void setUserName(const QString& name);
    void setUserAddress(const QString& address);

    bool isMyToken(const QString& token);

signals:
    void usertokenChanged();
    void usernameChanged();
    void useraddressChanged();

private:
    QString m_token;
    QString m_name;
    QString m_address;
};

#endif // USERINFOMODEL_H
