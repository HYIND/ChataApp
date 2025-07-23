#include "UserInfoModel.h"

UserInfoModel::UserInfoModel() {
    m_token = "";
    m_name = "";
    m_address = "";
}

int UserInfoModel::rowCount(const QModelIndex &parent) const
{
    return 0;
}

QVariant UserInfoModel::data(const QModelIndex &index, int role) const
{
    return QVariant();
}

QHash<int, QByteArray> UserInfoModel::roleNames() const
{
    static QHash<int, QByteArray> roles = {
        {UserTokenRole,"usertoken"},
        {UserNameRole, "username"},
        {UserAddressRole, "useraddress"}
    };
    return roles;
}

QString UserInfoModel::usertoken() const
{
    return m_token;
}

QString UserInfoModel::username() const
{
    return m_name;
}

QString UserInfoModel::useraddress() const
{
    return m_address;
}

void UserInfoModel::setUserToken(const QString &token)
{
    m_token = token;
    emit usertokenChanged();
}

void UserInfoModel::setUserName(const QString &name)
{
    m_name = name;
    emit usernameChanged();
}

void UserInfoModel::setUserAddress(const QString &address)
{
    m_address= address;
    emit useraddressChanged();
}

bool UserInfoModel::isMyToken(const QString &token)
{
    return token == m_token;
}
