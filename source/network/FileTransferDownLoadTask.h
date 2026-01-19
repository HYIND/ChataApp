#pragma once
#include "FileTransferTask.h"
#include "AsyncMD5.h"

class FileTransferDownLoadTask : public FileTransferTask
{
public:
    FileTransferDownLoadTask(const QString& taskid, const QString& filepath = "", const QString& md5 = "");
	virtual ~FileTransferDownLoadTask();

public:
	virtual void ProcessMsg(const json& js, Buffer& buf);
	virtual void ReleaseSource();
	virtual void InterruptTrans();

	void BindErrorCallBack(std::function<void(FileTransferDownLoadTask*)> callback);
	void BindFinishedCallBack(std::function<void(FileTransferDownLoadTask*)> callback);
	void BindInterruptedCallBack(std::function<void(FileTransferDownLoadTask*)> callback);
	void BindProgressCallBack(std::function<void(FileTransferDownLoadTask*, uint32_t)> callback);

    void RegisterTransInfo(const QString& filepath, const QString& md5, uint64_t filesize); // 使用提前注册好的传输信息，而非被动接收对方的信息，在确认信息时执行校验行为
protected:
	virtual void OnError();
	virtual void OnFinished();
	virtual void OnInterrupted();
	virtual void OnProgress(uint32_t progress);

	virtual void OccurError();
	virtual void OccurFinish();
	virtual void OccurInterrupt();
	virtual void OccurProgressChange();

	virtual uint32_t Progress();

	void RecvTransReq(const json& js);
	void AckTransReq(const json& js);
	void RecvChunkDataAndAck(const json& js, Buffer& buf);
	void AckRecvFinished(const json& js);
	void SendErrorInfo();
	void RecvPeerError(const json& js);
	void RecvPeerInterrupt(const json& js);

private:
	bool ParseFile();
	bool ReadChunkFile();
	bool ParseChunkMap(const json& js);
	bool WriteToChunkFile();
    bool CheckTransFinish();

    void AsyncMD5Update();
    std::string AsyncMD5Final();

protected:
	std::function<void(FileTransferDownLoadTask*)> _callbackError;
	std::function<void(FileTransferDownLoadTask*)> _callbackFinieshed;
	std::function<void(FileTransferDownLoadTask*)> _callbackInterrupted;
	std::function<void(FileTransferDownLoadTask*, uint32_t)> _callbackProgress;

private:
	bool IsChunkFileEnable = false;
	FileIOHandler chunkfile_io;
	bool IsRegister = false;
    AsyncMD5 _asyncmd5;
};
