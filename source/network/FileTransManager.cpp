#include "FileTransManager.h"
#include "ModelManager.h"
#include "ConnectManager.h"
#include <QUuid>
#include <QTimer>
#include "NetWorkHelper.h"
#include <QCoreApplication>

const QString default_downloaddir = "chat_download/";
;
inline CustomTcpSession* GetSession() {
	return CONNECTMANAGER->Session();
}

FileTransTaskContent::FileTransTaskContent(const QString& id, FileTransferTask* t)
{
	fileid = id;
	task = t;
}

FileTransTaskContent::~FileTransTaskContent()
{
	SAFE_DELETE(task);
	;
}

FileReqRecord::FileReqRecord(const QString& id, const QString& path, uint64_t& size, const QString& md5)
	:fileid(id), filepath(path), filesize(size), md5(md5)
{
}

FileTransManager::FileTransManager()
{
	FileIOHandler::CreateFolder(DownloadDir());
}

FileTransManager* FileTransManager::Instance()
{
	static FileTransManager* instance = new FileTransManager();
	return instance;
}

bool FileTransManager::ProcessMsg(const json& js, Buffer& buf)
{
	int command = js.at("command");
	switch (command)
	{
	case 5001:
		AckTaskRes(js);
		return true;
		break;
	default:
		return DistributeMsg(js, buf);
		break;
	}
	return false;
}

bool FileTransManager::DistributeMsg(const json& js, Buffer& buf)
{
	if (!js.contains("taskid") || !js.at("taskid").is_string())
		return false;

	QString taskid = QString::fromStdString(js["taskid"]);
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return false;

	content->task->ProcessMsg(js, buf);

	return true;
}

void FileTransManager::AckTaskRes(const json& js)
{
	if (!js.contains("fileid") || !js.at("fileid").is_string())
		return;
	if (!js.contains("taskid") || !js.at("taskid").is_string())
		return;
	if (!js.contains("type") || !js.at("type").is_number_unsigned())
		return;
	if (!js.contains("result") || !js.at("result").is_number())
		return;
	if (!js.contains("filesize") || !js.at("filesize").is_number_unsigned())
		return;
	if (!js.contains("suggest_chunksize") || !js.at("suggest_chunksize").is_number_unsigned())
		return;

	QString fileid = QString::fromStdString(js["fileid"]);
	QString taskid = QString::fromStdString(js["taskid"]);
	int type = js["type"];
	int result = js["result"];
	uint64_t filesize = js["filesize"];
	uint64_t suggest_chunksize = js["suggest_chunksize"];

	if (result == -1)
	{
		return;
	}

	if (type == 1)
	{
		if (result == 0)
		{
			FileReqRecord* re = nullptr;
			if (!m_reqrecords.Find(fileid, re))
				return;

			//防止重复添加同一文件的任务
			if (!AddUploadTask(fileid, taskid, re->filepath, suggest_chunksize))
			{
				json js_error;
				js_error["command"] = 7080;
				js_error["taskid"] = taskid.toStdString();
				js_error["result"] = 0;

				NetWorkHelper::SendMessagePackage(&js_error);
			}
		}
	}

	if (type == 2)
	{
		FileReqRecord* re = nullptr;
		if (m_reqrecords.Find(fileid, re))
		{
			if (result == 1)
			{
				//防止重复添加同一文件的任务
				if (!AddDownloadTask(fileid, taskid, re->filepath, re->filesize, re->md5))
				{
					json js_error;
					js_error["command"] = 7080;
					js_error["taskid"] = taskid.toStdString();
					js_error["result"] = 0;

					NetWorkHelper::SendMessagePackage(&js_error);
				}
			}
			else
			{
				re->trycount++;
				if (re->trycount < 5)
				{
					QTimer* timer = new QTimer();
					timer->setSingleShot(true);
					// 连接定时器的 timeout 信号到槽函数
					QObject::connect(timer, &QTimer::timeout, [this, fileid]() {
						this->ReqDownloadFile(fileid);
						});
					timer->start(3000);
				}
			}
		}
	}
}

bool FileTransManager::AddUploadTask(const QString& fileid, const QString& taskid, const QString& filepath, uint64_t suggest_chunksize)
{
	FileTransTaskContent* content = nullptr;
	if (FindTaskByFileId(fileid, content))
		return false;
	if (m_tasks.Find(taskid, content))
		return false;

	FileTransferUploadTask* uploadtask = new FileTransferUploadTask(taskid, filepath);
	uploadtask->BindErrorCallBack(std::bind(&FileTransManager::OnUploadError, this, std::placeholders::_1));
	uploadtask->BindFinishedCallBack(std::bind(&FileTransManager::OnUploadFinish, this, std::placeholders::_1));
	uploadtask->BindInterruptedCallBack(std::bind(&FileTransManager::OnUploadInterrupt, this, std::placeholders::_1));
	uploadtask->BindProgressCallBack(std::bind(&FileTransManager::OnUploadProgress, this, std::placeholders::_1, std::placeholders::_2));
	uploadtask->SetSuggestChunkSize(suggest_chunksize);

	content = new FileTransTaskContent(fileid, uploadtask);
	bool result = m_tasks.Insert(taskid, content);
	if (!result)
		SAFE_DELETE(content);

	if (result)
		result = uploadtask->StartSendFile();
	if (!result)
	{
		m_tasks.Erase(taskid);
		SAFE_DELETE(content);
	}

	return result;
}

bool FileTransManager::AddDownloadTask(const QString& fileid, const QString& taskid, const QString& filepath, uint64_t filesize, const QString& md5)
{
	FileTransTaskContent* content = nullptr;
	if (FindTaskByFileId(fileid, content))
		return false;
	if (m_tasks.Find(taskid, content))
		return false;

	FileTransferDownLoadTask* downloadtask = new FileTransferDownLoadTask(taskid);
	downloadtask->RegisterTransInfo(filepath, filesize);
	downloadtask->BindErrorCallBack(std::bind(&FileTransManager::OnDownloadError, this, std::placeholders::_1));
	downloadtask->BindFinishedCallBack(std::bind(&FileTransManager::OnDownloadFinish, this, std::placeholders::_1));
	downloadtask->BindInterruptedCallBack(std::bind(&FileTransManager::OnDownloadInterrupt, this, std::placeholders::_1));
	downloadtask->BindProgressCallBack(std::bind(&FileTransManager::OnDownloadProgress, this, std::placeholders::_1, std::placeholders::_2));

	content = new FileTransTaskContent(fileid, downloadtask);
	bool result = m_tasks.Insert(taskid, content);
	if (!result)
		SAFE_DELETE(content);
	return result;
}

bool FileTransManager::AddReqRecord(const QString& fileid, const QString& filepath, uint64_t filesize, const QString& md5)
{
	FileReqRecord* re = new FileReqRecord(fileid, filepath, filesize, md5);
	return m_reqrecords.Insert(fileid, re);
}

bool FileTransManager::updateReqRecordStatus(const QString& fileid, transstatus newstatus)
{
	FileReqRecord* re = nullptr;
	if (!m_reqrecords.Find(fileid, re))
		return false;
	re->status = newstatus;
	return true;
}

void FileTransManager::ReqUploadFile(const QString& fileid)
{
	json js;
	js["command"] = 4001;
	js["fileid"] = fileid.toStdString();
	js["taskid"] = QUuid::createUuid().toString().toStdString();
	js["type"] = 1;
    js["jwt"] = USERINFOMODEL->userjwt().toStdString();

	NetWorkHelper::SendMessagePackage(&js);
}

void FileTransManager::ReqDownloadFile(const QString& fileid)
{
	json js;
	js["command"] = 4001;
	js["fileid"] = fileid.toStdString();
	js["taskid"] = QUuid::createUuid().toString().toStdString();
	js["type"] = 2;
    js["jwt"] = USERINFOMODEL->userjwt().toStdString();

	NetWorkHelper::SendMessagePackage(&js);
}

void FileTransManager::InterruptTask(const QString& fileid)
{
	FileTransTaskContent* content = nullptr;
	if (!FindTaskByFileId(fileid, content))
		return;

	QString taskid = content->task->TaskId();
	content->task->InterruptTrans();
	DeleteTask(taskid);
}

QString FileTransManager::DownloadDir()
{
    QString appDirPath = QCoreApplication::applicationDirPath();
    return appDirPath + "/" + default_downloaddir + USERINFOMODEL->usertoken() + "/" ;
}

QString FileTransManager::FindDownloadPathByFileId(const QString &fileid)
{
    QString result = "";
    m_tasks.EnsureCall(
        [&](std::map<QString, FileTransTaskContent*>& map) ->void {
            for (auto& task : map)
            {
                if (task.second->fileid == fileid)
                {
                    FileTransTaskContent* content = task.second;
                    result = content->task->FilePath();
                    return;
                }
            }
        }
        );
    return result;
}

void FileTransManager::DeleteTask(const QString& taskid)
{
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return;

	m_tasks.Erase(taskid);
	SAFE_DELETE(content);
}

bool FileTransManager::FindTaskByFileId(const QString& fileid, FileTransTaskContent*& content)
{
	bool result = false;
	m_tasks.EnsureCall(
		[&](std::map<QString, FileTransTaskContent*>& map) ->void {
			for (auto& task : map)
			{
				if (task.second->fileid == fileid)
				{
					content = task.second;
					result = true;
					return;
				}
			}
		}
	);
	return result;
}

void FileTransManager::OnUploadFinish(FileTransferUploadTask* task)
{
	QString taskid = task->TaskId();
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return;

	updateReqRecordStatus(content->fileid, transstatus::done);
	CHATITEMMODEL->fileTransFinished(content->fileid);
	DeleteTask(taskid);
}

void FileTransManager::OnUploadError(FileTransferUploadTask* task)
{
	QString taskid = task->TaskId();
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return;

	CHATITEMMODEL->fileTransError(content->fileid);
	DeleteTask(taskid);
}

void FileTransManager::OnUploadInterrupt(FileTransferUploadTask* task)
{
	QString taskid = task->TaskId();
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return;

	CHATITEMMODEL->fileTransInterrupted(content->fileid);
	DeleteTask(taskid);
}

void FileTransManager::OnUploadProgress(FileTransferUploadTask* task, uint32_t progress)
{
	QString taskid = task->TaskId();
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return;

	CHATITEMMODEL->fileTransProgressChange(content->fileid, progress);
}

void FileTransManager::OnDownloadFinish(FileTransferDownLoadTask* task)
{
	QString taskid = task->TaskId();
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return;

	updateReqRecordStatus(content->fileid, transstatus::done);
	CHATITEMMODEL->fileTransFinished(content->fileid);
	DeleteTask(taskid);
}

void FileTransManager::OnDownloadError(FileTransferDownLoadTask* task)
{
	QString taskid = task->TaskId();
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return;

	CHATITEMMODEL->fileTransError(content->fileid);
	DeleteTask(taskid);
}

void FileTransManager::OnDownloadInterrupt(FileTransferDownLoadTask* task)
{
	QString taskid = task->TaskId();
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return;

	CHATITEMMODEL->fileTransInterrupted(content->fileid);
	DeleteTask(taskid);
}

void FileTransManager::OnDownloadProgress(FileTransferDownLoadTask* task, uint32_t progress)
{
	QString taskid = task->TaskId();
	FileTransTaskContent* content = nullptr;
	if (!m_tasks.Find(taskid, content))
		return;

	CHATITEMMODEL->fileTransProgressChange(content->fileid, progress);
}

