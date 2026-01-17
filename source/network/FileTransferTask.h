#pragma once

#include "stdafx.h"
#include "FileIOHandler.h"
#include "Buffer.h"

struct FileTransferChunkInfo
{
	int index;
	uint64_t range_left;
	uint64_t range_right;
	FileTransferChunkInfo(int index, uint64_t left, uint64_t right) : index(index), range_left(left), range_right(right) {}
};

struct FileTransferChunkData
{
	uint64_t range_left;
	uint64_t range_right;
	Buffer buf;
	std::vector<uint8_t> ToBinary();
	void FromBinary(const std::vector<uint8_t>& datas);
};

QString getFilenameFromPath(const QString& filepath);
std::vector<FileTransferChunkInfo> mergeChunks(const std::vector<FileTransferChunkInfo>& chunks);
std::vector<FileTransferChunkInfo> getUntransferredChunks(const std::vector<FileTransferChunkInfo>& transferredChunks, uint64_t totalFileSize);
uint32_t CountProgress(const std::vector<FileTransferChunkInfo>& chunks, uint64_t totalFileSize);
uint64_t GetSuggestChunsize(uint64_t file_size);

class FileTransferTask
{
public:
    FileTransferTask(const QString& taskid, const QString& filepath, const QString& md5);
	virtual ~FileTransferTask();

	const FileIOHandler& FileHandler();
	const QString TaskId();
    const QString FilePath();

public:
	virtual void ProcessMsg(const json& js, Buffer& buf) = 0;
	virtual void ReleaseSource() = 0;
	virtual void InterruptTrans() = 0;

protected:
	virtual void OnError() = 0;
	virtual void OnFinished() = 0;
	virtual void OnInterrupted() = 0;
	virtual void OnProgress(uint32_t progress) = 0;

	virtual void OccurError() = 0;
	virtual void OccurFinish() = 0;
	virtual void OccurInterrupt() = 0;
	virtual void OccurProgressChange() = 0;

	virtual uint32_t Progress() = 0;

protected:
	QString file_path;
    QString _md5;
	QString task_id;
	FileIOHandler file_io;
	uint64_t file_size = 0;
	std::vector<FileTransferChunkInfo> chunk_map;

	bool IsError = false;
	bool IsFinished = false;
	bool IsInterrupted = false;

	bool IsFileEnable = false;
	bool IsTransFinish = false;

	CriticalSectionLock InterruptedLock;
};
