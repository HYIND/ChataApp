#pragma once
#include "FileTransferTask.h"

class FileTransferDownLoadTask : public FileTransferTask
{
public:
    FileTransferDownLoadTask(const string &taskid, const string &filepath_dir = "");
    virtual ~FileTransferDownLoadTask();

public:
    virtual void ProcessMsg(BaseNetWorkSession *session, const json &js, Buffer &buf);
    virtual void ReleaseSource();
    virtual void InterruptTrans(BaseNetWorkSession *session);

    void BindErrorCallBack(std::function<void(FileTransferDownLoadTask *)> callback);
    void BindFinishedCallBack(std::function<void(FileTransferDownLoadTask *)> callback);
    void BindInterruptedCallBack(std::function<void(FileTransferDownLoadTask *)> callback);
    void BindProgressCallBack(std::function<void(FileTransferDownLoadTask *, uint32_t)> callback);

    void RegisterTransInfo(const string &filepath, uint64_t filesize); // 使用提前注册好的传输信息，而非被动接收对方的信息，在确认信息时执行校验行为
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

    void RecvTransReq(BaseNetWorkSession *session, const json &js);
    void AckTransReq(BaseNetWorkSession *session, const json &js);
    void RecvChunkDataAndAck(BaseNetWorkSession *session, const json &js, Buffer &buf);
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

protected:
    std::function<void(FileTransferDownLoadTask *)> _callbackError;
    std::function<void(FileTransferDownLoadTask *)> _callbackFinieshed;
    std::function<void(FileTransferDownLoadTask *)> _callbackInterrupted;
    std::function<void(FileTransferDownLoadTask *, uint32_t)> _callbackProgress;

private:
    bool IsChunkFileEnable = false;
    FileIOHandler chunkfile_io;
    bool IsRegister = false;
};