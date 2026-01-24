#include "NetWorkHelper.h"

bool NetWorkHelper::SendMessagePackage(BaseNetWorkSession *session, json *json)
{
    MessagePackage package(*json);
    return NetWorkHelper::SendMessagePackage(session, &package);
}

bool NetWorkHelper::SendMessagePackage(BaseNetWorkSession *session, Buffer *buf)
{
    MessagePackage package(*buf);
    return NetWorkHelper::SendMessagePackage(session, &package);
}

bool NetWorkHelper::SendMessagePackage(BaseNetWorkSession *session, json *json, Buffer *buf)
{
    MessagePackage package(*json, *buf);
    return NetWorkHelper::SendMessagePackage(session, &package);
}

bool NetWorkHelper::SendMessagePackage(BaseNetWorkSession *session, MessagePackage *package)
{
    Buffer buf;
    GenerateMessagePackageToBuffer(package, &buf);
    return session->AsyncSend(buf);
}
