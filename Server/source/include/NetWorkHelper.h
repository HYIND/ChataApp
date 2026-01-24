#pragma once
#include "stdafx.h"
#include "Net/include/Session/BaseNetWorkSession.h"
#include "MessagePackage.h"

namespace NetWorkHelper
{
    bool SendMessagePackage(BaseNetWorkSession *session, json *json);
    bool SendMessagePackage(BaseNetWorkSession *session, Buffer *buf);
    bool SendMessagePackage(BaseNetWorkSession *session, json *json, Buffer *buf);
    bool SendMessagePackage(BaseNetWorkSession *session, MessagePackage *package);
}