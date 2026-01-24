#ifndef FILETRANSMANAGER_H
#define FILETRANSMANAGER_H

#include "FileTransferTask.h"
#include "FileTransferUploadTask.h"
#include "FileTransferDownLoadTask.h"
#include "SafeStl.h"

struct FileTransTaskContent
{
	QString fileid;
	FileTransferTask* task = nullptr;

	FileTransTaskContent(const QString& id, FileTransferTask* t);
	~FileTransTaskContent();
};

enum class transstatus {
	no_start = 1,
	doing = 2,
	done = 3
};

struct FileReqRecord
{
	QString fileid;
	QString filepath;
	uint64_t filesize;
	QString md5;
	transstatus status = transstatus::no_start;
	int trycount = 0;

	FileReqRecord(const QString& id, const QString& path, uint64_t& size, const QString& md5);
};

class FileTransManager
{
public:
	static FileTransManager* Instance();

public:
	// 处理消息的入口
	bool ProcessMsg(const json& js, Buffer& buf);
	bool DistributeMsg(const json& js, Buffer& buf);

protected:
	void AckTaskRes(const json& js);

public:
	void OnUploadFinish(FileTransferUploadTask* task);
	void OnUploadError(FileTransferUploadTask* task);
	void OnUploadInterrupt(FileTransferUploadTask* task);
	void OnUploadProgress(FileTransferUploadTask* task, uint32_t progress);

	void OnDownloadFinish(FileTransferDownLoadTask* task);
	void OnDownloadError(FileTransferDownLoadTask* task);
	void OnDownloadInterrupt(FileTransferDownLoadTask* task);
	void OnDownloadProgress(FileTransferDownLoadTask* task, uint32_t progress);

	bool AddReqRecord(const QString& fileid, const QString& filepath, uint64_t filesize, const QString& md5);
	bool updateReqRecordStatus(const QString& fileid, transstatus newstatus);

	void ReqUploadFile(const QString& fileid);
	void ReqDownloadFile(const QString& fileid);

	void InterruptTask(const QString& fileid);

	QString DownloadDir();
    QString FindDownloadPathByFileId(const QString& fileid);

private:
	FileTransManager();
    bool AddUploadTask(const QString& fileid, const QString& taskid, const QString& filepath, const QString& md5, uint64_t suggest_chunksize = 1000);
    bool AddDownloadTask(const QString& fileid, const QString& taskid, const QString& filepath, const QString& md5, uint64_t filesize);
	void DeleteTask(const QString& taskid);
	bool FindTaskByFileId(const QString& fileid, FileTransTaskContent*& content);

private:
	SafeMap<QString, FileTransTaskContent*> m_tasks; // taskid->content
	SafeMap<QString, FileReqRecord*> m_reqrecords; // taskid->record
};

#define FILETRANSMANAGER FileTransManager::Instance()

#endif // FILETRANSMANAGER_H
