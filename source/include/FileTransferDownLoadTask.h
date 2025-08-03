#pragma once
#include "FileTransferTask.h"

class FileTransferDownLoadTask : public FileTransferTask
{
public:
    FileTransferDownLoadTask(const string &filepath, const string &taskid, uint64_t length = 0);
    virtual ~FileTransferDownLoadTask();

public:
    void ProcessMsg(BaseNetWorkSession *session, const Buffer &buf);
    void ReleaseSource();
    void InterruptTrans(BaseNetWorkSession *session);

    void BindErrorCallBack(std::function<void(FileTransferDownLoadTask *)> callback);
    void BindFinishedCallBack(std::function<void(FileTransferDownLoadTask *)> callback);
    void BindInterruptedCallBack(std::function<void(FileTransferDownLoadTask *)> callback);

protected:
    virtual void OnError();
    virtual void OnFinished();
    virtual void OnInterrupted();

    void AckTransReq(BaseNetWorkSession *session, const json &js);
    void RecvChunkDataAndAck(BaseNetWorkSession *session, const json &js);
    void AckRecvFinished(BaseNetWorkSession *session, const json &js);
    void SendErrorInfo(BaseNetWorkSession *session);
    void RecvPeerError(const json &js);
    void RecvPeerInterrupt(const json &js);

private:
    bool ParseFile();
    bool ReadChunkFile();
    bool ParseChunkMap(const json &js);
    bool WriteToChunkFile();
    bool CheckFinish();
    void OccurError(BaseNetWorkSession *session);
    void OccurFinish();
    void OccurInterrupt();

protected:
    std::function<void(FileTransferDownLoadTask *)> _callbackError;
    std::function<void(FileTransferDownLoadTask *)> _callbackFinieshed;
    std::function<void(FileTransferDownLoadTask *)> _callbackInterrupted;

private:
    bool IsFileEnable = false;
    bool IsChunkFileEnable = false;
    bool IsTransFinish = false;
    FileIOHandler chunkfile_io;
};