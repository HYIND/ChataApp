#pragma once

#include "FileTransferTask.h"
#include <QString>

class FileTransferUploadTask : public FileTransferTask
{
public:
    FileTransferUploadTask(const QString& taskid, const QString& filepath, const QString& md5 = "");
	virtual ~FileTransferUploadTask();

public:
	bool StartSendFile();

	virtual void ProcessMsg(const json& js, Buffer& buf);
	virtual void ReleaseSource();
	virtual void InterruptTrans();

	void BindErrorCallBack(std::function<void(FileTransferUploadTask*)> callback);
	void BindFinishedCallBack(std::function<void(FileTransferUploadTask*)> callback);
	void BindInterruptedCallBack(std::function<void(FileTransferUploadTask*)> callback);
	void BindProgressCallBack(std::function<void(FileTransferUploadTask*, uint32_t)> callback);

	void SetSuggestChunkSize(uint64_t size);
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

	void SendTransReq();
	void AckTransReqResult(const json& js);
	void RecvChunkMapAndSendNextData(const json& js);
	void SendNextChunkData();
	void SendErrorInfo();
	void RecvPeerError(const json& js);
	void RecvPeerFinish(const json& js);
	void RecvPeerInterrupt(const json& js);

private:
	bool ParseFile();
	bool ParseReqResult(const json& js);
	bool ParseChunkMap(const json& js);
	bool ParseSuggestChunkSize(const json& js);

protected:
	std::function<void(FileTransferUploadTask*)> _callbackError;
	std::function<void(FileTransferUploadTask*)> _callbackFinieshed;
	std::function<void(FileTransferUploadTask*)> _callbackInterrupted;
	std::function<void(FileTransferUploadTask*, uint32_t)> _callbackProgress;

private:
	uint64_t suggest_chunksize = 1;
};
