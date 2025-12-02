
#pragma once
#include "stdafx.h"
#include "QBaseNetWorkSession.h"
#include "MessagePackage.h"

namespace NetWorkHelper
{
    bool SendMessagePackage(json* json);
    bool SendMessagePackage(QJsonObject* jsonObj);
    bool SendMessagePackage(Buffer* buf);
    bool SendMessagePackage(json* json, Buffer* buf);
    bool SendMessagePackage(QJsonObject* jsonObj, Buffer* buf);
    bool SendMessagePackage(MessagePackage* package);
}