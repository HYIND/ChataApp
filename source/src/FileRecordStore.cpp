#include "FileRecordStore.h"
#include <iostream>
#include <fstream>
#include "FileIOHandler.h"
#include "Timer.h"

using namespace std;

static constexpr int64_t msgexpiredseconds = 60 * 60 * 24 * 3; // 消息记录3天过期

static int64_t GetTimeStampSecond()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto second = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    return second;
}

inline std::string ConcatPath(const std::string &path1, const std::string &path2)
{
    std::string concatpath1, concatpath2;
    if (!path1.empty())
    {
        if (path1[path1.size() - 1] == '/')
            concatpath1.assign(path1, 0, path1.size() - 1);
        else
            concatpath1 = path1;
    }
    if (!path2.empty())
    {
        if (path2[0] == '/')
            concatpath2.assign(path2, 1, path2.size() - 1);
        else
            concatpath2 = path2;
    }

    return concatpath1 + '/' + concatpath2;
}

inline std::string GetFileRecordDirName()
{
    const std::string defaultdir_Path = "./filestore/";
    return defaultdir_Path;
}
inline std::string generatePath(const std::string &fileId)
{
    return ConcatPath(GetFileRecordDirName(), (fileId + ".dat"));
}

FileRecord::FileRecord(std::string id, FileStoreStatus s, std::string m, uint64_t size, uint64_t timestamp, const std::string &p)
    : fileid(id), status(s), md5(std::move(m)), path(std::move(p)), timestamp(timestamp), filesize(size) {}

FileRecordStore *FileRecordStore::Instance()
{
    static FileRecordStore *instance = new FileRecordStore();
    return instance;
}

FileRecordStore::FileRecordStore()
{
    FileIOHandler::CreateFolder(GetFileRecordDirName());
    filePath_ = ConcatPath(GetFileRecordDirName(), "FileRecord");
    loadRecords();
    static constexpr uint64_t cleaninterval = 30 * 1000, firstclean = 10 * 1000;
    auto task = TimerTask::CreateRepeat("CleanExpiredFileStoreTimer", cleaninterval, std::bind(&FileRecordStore::CleanExpiredFileStore, this), firstclean);
    task->Run();
}

bool FileRecordStore::addFileRecord(std::string fileId,
                                    std::string md5,
                                    uint64_t filesize)
{
    if (fileId == "")
        return false;

    static std::string defaultmd5 = "defaultmd5";
    if (md5 == "")
        md5 = defaultmd5;

    LockGuard lock(_lock);

    FileRecord *re = nullptr;
    if (records_.Find(fileId, re))
    {
        return false;
    }

    std::string actualPath = generatePath(fileId);
    uint64_t timestamp = GetTimeStampSecond();

    re = new FileRecord(fileId, FileStoreStatus::NOT_UPLOADED, md5, filesize, timestamp, actualPath);
    bool result = records_.Insert(fileId, re);
    if (!result)
    {
        SAFE_DELETE(re);
        return result;
    }
    saveRecords();

    return result;
}

// 删除文件记录
void FileRecordStore::deleteFileRecord(const std::string &fileId)
{
    LockGuard lock(_lock);

    FileRecord *record = nullptr;
    if (records_.Find(fileId, record))
    {
        records_.Erase(fileId);
        SAFE_DELETE(record);
        saveRecords();
    }
}

// 更新文件状态
bool FileRecordStore::updateFileRecordStatus(const std::string &fileId, FileStoreStatus newStatus)
{
    LockGuard lock(_lock);

    FileRecord *re = nullptr;
    if (!records_.Find(fileId, re))
        return false;

    re->status = newStatus;
    saveRecords();

    return true;
}

// 更新MD5值
bool FileRecordStore::updateFileRecordMd5(const std::string &fileId, const std::string &newMd5)
{
    LockGuard lock(_lock);

    FileRecord *re = nullptr;
    if (!records_.Find(fileId, re))
        return false;

    re->md5 = newMd5;
    saveRecords();

    return true;
}

// 获取记录（内部使用）
bool FileRecordStore::getRecord(const std::string &fileId, FileRecord &record)
{
    FileRecord *re = nullptr;
    if (!records_.Find(fileId, re))
        return false;

    record.fileid = re->fileid;
    record.status = re->status;
    record.path = re->path;
    record.md5 = re->md5;
    record.filesize = re->filesize;

    return true;
}

// 查询接口实现
std::string FileRecordStore::getPath(const std::string &fileId)
{
    FileRecord *re = nullptr;
    if (!records_.Find(fileId, re))
        return string{};

    return re->path;
}

FileStoreStatus FileRecordStore::getStatus(const std::string &fileId)
{
    FileRecord *re = nullptr;
    if (!records_.Find(fileId, re))
        return FileStoreStatus::NOT_UPLOADED;

    return re->status;
}

std::string FileRecordStore::getMd5(const std::string &fileId)
{
    FileRecord *re = nullptr;
    if (records_.Find(fileId, re))
        return string{};

    return re->md5;
}

// 从文件加载记录
void FileRecordStore::loadRecords()
{
    LockGuard lock(_lock);

    std::ifstream inFile(filePath_);
    if (!inFile.is_open())
    {
        std::ofstream outFile(filePath_);
        return;
    }

    std::string line;
    while (std::getline(inFile, line))
    {
        std::istringstream iss(line);
        std::string fileId, md5, path;
        uint64_t filesize, timestamp;
        int statusValue;

        // 新格式: fileId status md5 filesize timestamp path
        if (iss >> fileId >> statusValue >> md5 >> filesize >> timestamp >> std::ws &&
            std::getline(iss, path))
        {
            records_.Insert(
                fileId,
                new FileRecord(
                    fileId,
                    static_cast<FileStoreStatus>(statusValue),
                    std::move(md5),
                    filesize,
                    timestamp,
                    std::move(path)));
        }
    }
}

void FileRecordStore::saveRecords()
{
    LockGuard lock(_lock);

    std::ofstream outFile(filePath_);
    if (!outFile.is_open())
    {
        throw std::runtime_error("Failed to open file for writing");
    }

    records_.EnsureCall([&](std::map<std::string, FileRecord *> &map) -> void
                        {
        for (const auto &[fileId, record] : map)
       {
           outFile << fileId << " "
                   << static_cast<int>(record->status) << " "
                   << record->md5 << " "
                   << record->filesize << " "
                   << record->timestamp << " "
                   << record->path << "\n";
       } });
}

void FileRecordStore::CleanExpiredFileStore()
{
    LockGuard lock(_lock);

    int64_t currentTime = GetTimeStampSecond();
    int64_t expiredTime = currentTime - msgexpiredseconds;

    bool needclean = false;
    records_.EnsureCall([&](std::map<std::string, FileRecord *> &map) -> void
                        {
        for (auto it = map.begin();it!=map.end();)
       {
            FileRecord * record = it->second;
            if(record->timestamp< expiredTime)
            {
                if(!needclean)
                    needclean=true;
                it = map.erase(it);

                if(!record->path.empty())
                {
                    std::string path = record->path;
                    std::string chunks_path = record->path + "__chunks";
                    if(FileIOHandler::Exists(path))
                        FileIOHandler::Remove(path);
                    if(FileIOHandler::Exists(chunks_path))
                        FileIOHandler::Remove(chunks_path);
                }
                SAFE_DELETE(record);
            }
            else 
            {
                it++;
            }
       } });

    if (needclean)
        saveRecords();
}
