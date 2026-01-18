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
    void DisConnect();

signals:
    void loginSuccess();
    void loginFail();
    void disConnect();
};

#endif // LOGINMODEL_H
