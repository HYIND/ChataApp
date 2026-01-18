#include "LoginModel.h"

LoginModel::LoginModel() {}

void LoginModel::LoginSuccess()
{
    emit loginSuccess();
}

void LoginModel::LoginFail()
{
    emit loginFail();
}

void LoginModel::DisConnect()
{
    emit disConnect();
}
