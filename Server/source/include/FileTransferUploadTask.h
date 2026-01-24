#pragma once

#include "FileTransferTask.h"

class FileTransferUploadTask : public FileTransferTask
{
public:
    FileTransferUploadTask(const string &taskid, const string &filepath, const string &md5 = "");
    virtual ~FileTransferUploadTask();

public:
    bool StartSendFile(BaseNetWorkSession *session);

    virtual void ProcessMsg(BaseNetWorkSession *session, const json &js, Buffer &buf);
    virtual void ReleaseSource();
    virtual void InterruptTrans(BaseNetWorkSession *session);

    void BindErrorCallBack(std::function<void(FileTransferUploadTask *)> callback);
    void BindFinishedCallBack(std::function<void(FileTransferUploadTask *)> callback);
    void BindInterruptedCallBack(std::function<void(FileTransferUploadTask *)> callback);
    void BindProgressCallBack(std::function<void(FileTransferUploadTask *, uint32_t)> callback);

protected:
    virtual void OnError();
    virtual void OnFinished();
    virtual void OnInterrupted();
    virtual void OnProgress(uint32_t progress);

    virtual void OccurError(BaseNetWorkSession *session);
    virtual void OccurFinish();
    virtual void OccurInterrupt();
    virtual void OccurProgressChange();

    virtual uint32_t Progress();

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
    bool ParseReqResult(const json &js);
    bool ParseChunkMap(const json &js);
    bool ParseSuggestChunkSize(const json &js);

protected:
    std::function<void(FileTransferUploadTask *)> _callbackError;
    std::function<void(FileTransferUploadTask *)> _callbackFinieshed;
    std::function<void(FileTransferUploadTask *)> _callbackInterrupted;
    std::function<void(FileTransferUploadTask *, uint32_t)> _callbackProgress;

private:
    uint64_t suggest_chunksize = 1;
};