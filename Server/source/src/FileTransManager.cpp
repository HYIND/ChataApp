#include "FileTransManager.h"
#include "FileRecordStore.h"
#include "LoginUserManager.h"
#include "NetWorkHelper.h"
#include "Timer.h"

constexpr int64_t taskexpiredseconds = 1800;

int64_t GetTimestampSeconds()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

FileTransTaskContent::FileTransTaskContent(const string &id, FileTransferTask *t, BaseNetWorkSession *s)
{
    fileid = id;
    task = t;
    session = s;
    timestamp = GetTimestampSeconds();
}

FileTransTaskContent::~FileTransTaskContent()
{
    SAFE_DELETE(task);
    session = nullptr;
}

FileTransManager::FileTransManager()
    : HandleLoginUser(nullptr)
{
    CleanExpiredTask = TimerTask::CreateRepeat("FileTransManager::CleanExpiredFileTransTimer",
                                               30 * 1000,
                                               std::bind(&FileTransManager::CleanExpireTask, this),
                                               30 * 1000);
    CleanExpiredTask->Run();
}

FileTransManager::~FileTransManager()
{
    if (CleanExpiredTask)
    {
        CleanExpiredTask->Clean();
        CleanExpiredTask = nullptr;
    }
}

FileTransManager *FileTransManager::Instance()
{
    static FileTransManager *instance = new FileTransManager();
    return instance;
}

bool FileTransManager::ProcessMsg(BaseNetWorkSession *session, const json &js, Buffer &buf)
{
    int command = js.at("command");
    switch (command)
    {
    case 4001:
        AckTaskReq(session, js);
        return true;
        break;
    default:
        return DistributeMsg(session, js, buf);
        break;
    }
    return false;
}

bool FileTransManager::DistributeMsg(BaseNetWorkSession *session, const json &js, Buffer &buf)
{
    if (!js.contains("taskid") || !js.at("taskid").is_string())
        return false;

    string taskid = js["taskid"];
    UpdateTimeStamp(taskid);

    FileTransTaskContent *content = nullptr;
    if (!m_tasks.Find(taskid, content))
        return false;

    content->task->ProcessMsg(session, js, buf);

    return true;
}

void FileTransManager::AckTaskReq(BaseNetWorkSession *session, const json &js)
{
    if (!js.contains("fileid") || !js.at("fileid").is_string())
        return;
    if (!js.contains("taskid") || !js.at("taskid").is_string())
        return;
    if (!js.contains("type") || !js.at("type").is_number_unsigned())
        return;
    if (!js.contains("jwt") || !js.at("jwt").is_string())
        return;

    string jwtstr = js["jwt"];
    string fileid = js["fileid"];
    string taskid = js["taskid"];
    uint32_t type = js["type"];
    string token;
    if (!HandleLoginUser->Verfiy(session, jwtstr, token))
    {
        return;
    }

    json js_reply;
    FileRecord record;
    js_reply["command"] = 5001;
    js_reply["fileid"] = fileid;
    js_reply["type"] = type;
    if (FILERECORDSTORE->getRecord(fileid, record))
    {
        js_reply["result"] = record.status == FileStoreStatus::COMPLETED ? 1 : 0;
        js_reply["filesize"] = record.filesize;
        js_reply["suggest_chunksize"] = std::max((uint64_t)1, record.filesize / (uint64_t)10);
        js_reply["taskid"] = taskid;
        if (type == 1)
        {
            if (record.status != FileStoreStatus::COMPLETED)
            {
                FILERECORDSTORE->updateFileRecordStatus(fileid, FileStoreStatus::UPLOADING);
                AddDownloadTask(fileid, taskid, record.path, record.md5, record.filesize, session);
            }
        }
    }
    else
    {
        js_reply["result"] = -1;
        js_reply["filesize"] = 0;
        js_reply["suggest_chunksize"] = 0;
        js_reply["taskid"] = taskid;
    }

    // 先发送5001，然后启动Upload
    NetWorkHelper::SendMessagePackage(session, &js_reply);

    if (type == 2)
    {
        if (record.status == FileStoreStatus::COMPLETED)
        {
            AddUploadTask(fileid, taskid, record.path, record.md5, session);
        }
    }
}

bool FileTransManager::AddUploadTask(const string &fileid, const string &taskid, const string &filepath, const string &md5, BaseNetWorkSession *session)
{
    FileTransTaskContent *content = nullptr;
    if (m_tasks.Find(taskid, content))
        return false;

    FileTransferUploadTask *uploadtask = new FileTransferUploadTask(taskid, filepath, md5);
    uploadtask->BindErrorCallBack(std::bind(&FileTransManager::OnUploadError, this, std::placeholders::_1));
    uploadtask->BindFinishedCallBack(std::bind(&FileTransManager::OnUploadFinish, this, std::placeholders::_1));
    uploadtask->BindInterruptedCallBack(std::bind(&FileTransManager::OnUploadInterrupt, this, std::placeholders::_1));
    uploadtask->BindProgressCallBack(std::bind(&FileTransManager::OnUploadProgress, this, std::placeholders::_1, std::placeholders::_2));

    content = new FileTransTaskContent(fileid, uploadtask, session);
    bool result = m_tasks.Insert(taskid, content);
    if (!result)
        SAFE_DELETE(content);

    if (result)
        result = uploadtask->StartSendFile(session); // 如果返回false，task自动触发错误，无需额外处理

    return result;
}

bool FileTransManager::AddDownloadTask(const string &fileid, const string &taskid, const string &filepath, const string &md5, uint64_t filesize, BaseNetWorkSession *session)
{
    FileTransTaskContent *content = nullptr;
    if (m_tasks.Find(taskid, content))
        return false;

    FileTransferDownLoadTask *downloadtask = new FileTransferDownLoadTask(taskid);
    downloadtask->RegisterTransInfo(filepath, md5, filesize);
    downloadtask->BindErrorCallBack(std::bind(&FileTransManager::OnDownloadError, this, std::placeholders::_1));
    downloadtask->BindFinishedCallBack(std::bind(&FileTransManager::OnDownloadFinish, this, std::placeholders::_1));
    downloadtask->BindInterruptedCallBack(std::bind(&FileTransManager::OnDownloadInterrupt, this, std::placeholders::_1));
    downloadtask->BindProgressCallBack(std::bind(&FileTransManager::OnDownloadProgress, this, std::placeholders::_1, std::placeholders::_2));

    content = new FileTransTaskContent(fileid, downloadtask, session);
    bool result = m_tasks.Insert(taskid, content);
    if (!result)
        SAFE_DELETE(content);
    return result;
}

void FileTransManager::DeleteTask(const string &taskid)
{
    FileTransTaskContent *content = nullptr;
    auto guard = m_tasks.MakeLockGuard();
    if (!m_tasks.Find(taskid, content))
        return;

    m_tasks.Erase(taskid);
    SAFE_DELETE(content);
}

void FileTransManager::CleanExpireTask()
{
    int64_t currentTime = GetTimestampSeconds();
    int64_t expiredTime = currentTime - taskexpiredseconds;

    {
        auto guard = m_tasks.MakeLockGuard();
        std::map<std::string, FileTransTaskContent *> interruptTasks;
        m_tasks.EnsureCall([&](std::map<std::string, FileTransTaskContent *> &map) -> void
                           {
                        for(auto pair:map){
                            FileTransTaskContent * content = pair.second;
                            if(!content||!content->task)
                            {
                                interruptTasks[pair.first]= pair.second;
                                continue;
                            }
                            if(content->timestamp<expiredTime)
                            {
                                interruptTasks[pair.first]= pair.second;
                            }
                        } 
                        for(auto &pair:interruptTasks)
                        {
                            FileTransTaskContent * content = pair.second;
                            content->task->InterruptTrans(content->session);
                        } });
    }
}

void FileTransManager::UpdateTimeStamp(const string &taskid)
{
    FileTransTaskContent *content = nullptr;
    auto guard = m_tasks.MakeLockGuard();
    if (!m_tasks.Find(taskid, content))
        return;
    if (content)
        content->timestamp = GetTimestampSeconds();
}

void FileTransManager::OnUploadFinish(FileTransferUploadTask *task)
{
    string taskid = task->TaskId();
    DeleteTask(taskid);
}

void FileTransManager::OnUploadError(FileTransferUploadTask *task)
{
    string taskid = task->TaskId();
    {
        FileTransTaskContent *content = nullptr;
        auto guard = m_tasks.MakeLockGuard();
        if (!m_tasks.Find(taskid, content) && content)
            FILERECORDSTORE->updateFileRecordStatus(content->fileid, FileStoreStatus::SUSPEND);
    }
    DeleteTask(taskid);
}

void FileTransManager::OnUploadInterrupt(FileTransferUploadTask *task)
{
    string taskid = task->TaskId();
    {
        FileTransTaskContent *content = nullptr;
        auto guard = m_tasks.MakeLockGuard();
        if (!m_tasks.Find(taskid, content) && content)
            FILERECORDSTORE->updateFileRecordStatus(content->fileid, FileStoreStatus::SUSPEND);
    }
    DeleteTask(taskid);
}

void FileTransManager::OnUploadProgress(FileTransferUploadTask *task, uint32_t progress)
{
}

void FileTransManager::OnDownloadFinish(FileTransferDownLoadTask *task)
{
    string taskid = task->TaskId();
    FileTransTaskContent *content = nullptr;
    if (!m_tasks.Find(taskid, content))
        return;
    FILERECORDSTORE->updateFileRecordStatus(content->fileid, FileStoreStatus::COMPLETED);
    DeleteTask(taskid);
}

void FileTransManager::OnDownloadError(FileTransferDownLoadTask *task)
{
    string taskid = task->TaskId();
    DeleteTask(taskid);
}

void FileTransManager::OnDownloadInterrupt(FileTransferDownLoadTask *task)
{
    string taskid = task->TaskId();
    DeleteTask(taskid);
}

void FileTransManager::OnDownloadProgress(FileTransferDownLoadTask *task, uint32_t progress)
{
}

void FileTransManager::SetLoginUserManager(LoginUserManager *m)
{
    HandleLoginUser = m;
}

void FileTransManager::SessionClose(BaseNetWorkSession *session)
{
    auto guard = m_tasks.MakeLockGuard();
    std::map<std::string, FileTransTaskContent *> closeTasks;
    m_tasks.EnsureCall([&](std::map<std::string, FileTransTaskContent *> &map) -> void
                       {
                        for(auto pair:map){
                            FileTransTaskContent * content = pair.second;
                            if(!content||!content->task)
                            {
                                closeTasks[pair.first]= pair.second;
                                continue;
                            }
                            if(content->session == session)
                            {
                                closeTasks[pair.first]= pair.second;
                            }
                        } 
                        for(auto &pair : closeTasks)
                        {
                            DeleteTask(pair.first);
                        } });
}
