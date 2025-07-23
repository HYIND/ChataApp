#ifndef REQUESTMANAGER_H
#define REQUESTMANAGER_H

#include <QString>
#include "ChatItemListModel.h"

class RequestManager
{
public:
    static RequestManager* Instance();

public:
    void SendMsg(const QString& goaltoken, const MsgType type, const QString& msg);
    void RequestOnlineUserData();

private:
    RequestManager();
    ~RequestManager();

};

#define REQUESTMANAGER RequestManager::Instance()

#endif // REQUESTMANAGER_H
