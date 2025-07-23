#ifndef LOGINMODEL_H
#define LOGINMODEL_H

#include <QObject>

class LoginModel :public QObject
{
    Q_OBJECT
public:
    LoginModel();
    void LoginSuccess();
    void LoginFail();

signals:
    void loginSuccess();
    void loginFail();
};

#endif // LOGINMODEL_H
