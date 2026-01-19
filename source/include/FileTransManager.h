#pragma once

#include "stdafx.h"
#include "FileTransferUploadTask.h"
#include "FileTransferDownLoadTask.h"
#include "LoginUserManager.h"

using namespace std;

struct FileTransTaskContent
{
    string fileid;
    FileTransferTask *task = nullptr;
    BaseNetWorkSession *session = nullptr;
    int64_t timestamp;

    FileTransTaskContent(const string &id, FileTransferTask *t, BaseNetWorkSession *s);
    ~FileTransTaskContent();
};

class FileTransManager
{
public:
    static FileTransManager *Instance();

public:
    // 处理消息的入口
    bool ProcessMsg(BaseNetWorkSession *session, const json &js, Buffer &buf);
    bool DistributeMsg(BaseNetWorkSession *session, const json &js, Buffer &buf);

    ~FileTransManager();

protected:
    void AckTaskReq(BaseNetWorkSession *session, const json &js);

public:
    void OnUploadFinish(FileTransferUploadTask *task);
    void OnUploadError(FileTransferUploadTask *task);
    void OnUploadInterrupt(FileTransferUploadTask *task);
    void OnUploadProgress(FileTransferUploadTask *task, uint32_t progress);

    void OnDownloadFinish(FileTransferDownLoadTask *task);
    void OnDownloadError(FileTransferDownLoadTask *task);
    void OnDownloadInterrupt(FileTransferDownLoadTask *task);
    void OnDownloadProgress(FileTransferDownLoadTask *task, uint32_t progress);

    void SetLoginUserManager(LoginUserManager *m);
    void SessionClose(BaseNetWorkSession *session);

private:
    FileTransManager();
    bool AddUploadTask(const string &fileid, const string &taskid, const string &filepath, const string &md5, BaseNetWorkSession *session);
    bool AddDownloadTask(const string &fileid, const string &taskid, const string &filepath, const string &md5, uint64_t filesize, BaseNetWorkSession *session);
    void DeleteTask(const string &taskid);
    void CleanExpireTask();
    void UpdateTimeStamp(const string &taskid);

private:
    SafeMap<string, FileTransTaskContent *> m_tasks; // taskid->content
    LoginUserManager *HandleLoginUser;
    std::shared_ptr<TimerTask> CleanExpiredTask;
};

#define FILETRANSMANAGER FileTransManager::Instance()