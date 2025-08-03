#pragma once

#include "FileTransferTask.h"

class FileTransferUploadTask : public FileTransferTask
{
public:
    FileTransferUploadTask(const string &filepath, const string &taskid);
    virtual ~FileTransferUploadTask();

public:
    bool StartSendFile(BaseNetWorkSession *session);
    void ProcessMsg(BaseNetWorkSession *session, const Buffer &buf);
    void ReleaseSource();
    void InterruptTrans(BaseNetWorkSession *session);

    void BindErrorCallBack(std::function<void(FileTransferUploadTask *)> callback);
    void BindFinishedCallBack(std::function<void(FileTransferUploadTask *)> callback);
    void BindInterruptedCallBack(std::function<void(FileTransferUploadTask *)> callback);

protected:
    virtual void OnError();
    virtual void OnFinished();
    virtual void OnInterrupted();
    
    void SendTransReq(BaseNetWorkSession *session);
    void AckTransReqResult(BaseNetWorkSession *session, const json &js);
    void RecvChunkMapAndSendNextData(BaseNetWorkSession *session, const json &js);
    void SendNextChunkData(BaseNetWorkSession *session);
    void SendErrorInfo(BaseNetWorkSession *session);
    void RecvPeerError(const json &js);
    void RecvPeerFinish(const json &js);
    void RecvPeerInterrupt(const json &js);

private:
    bool ParseFile();
    bool ParseChunkMap(const json &js);
    bool ParseSuggestChunkSize(const json &js);
    void OccurError(BaseNetWorkSession *session);
    void OccurFinish();
    void OccurInterrupt();

protected:
    std::function<void(FileTransferUploadTask *)> _callbackError;
    std::function<void(FileTransferUploadTask *)> _callbackFinieshed;
    std::function<void(FileTransferUploadTask *)> _callbackInterrupted;

private:
    bool IsFileEnable = false;
    bool IsTransFinish = false;
    uint64_t suggest_chunksize = 1;
};