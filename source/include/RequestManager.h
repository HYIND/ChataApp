#ifndef REQUESTMANAGER_H
#define REQUESTMANAGER_H

#include <QString>
#include "ChatItemListModel.h"
#include "Buffer.h"

class RequestManager
{
public:
    static RequestManager* Instance();

public:
    void SendMsg(const QString& goaltoken, const MsgType type, const QString& msg);
    void SendPicture(const QString& goaltoken,const QString& filename, int64_t filesize, const QString& md5, const QString& fileid,Buffer& buf);
    void SendFile(const QString &goaltoken, const QString &filename, int64_t filesize, const QString& md5,const QString& fileid);
    void RequestOnlineUserData();
    void RequestMessageRecord(const QString &goaltoken);

private:
    RequestManager();
    ~RequestManager();

};

#define REQUESTMANAGER RequestManager::Instance()

#endif // REQUESTMANAGER_H
