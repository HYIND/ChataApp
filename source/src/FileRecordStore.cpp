#include "FileRecordStore.h"
#include <iostream>
#include <fstream>
#include "FileIOHandler.h"

using namespace std;

inline std::string GetFileRecordDirName()
{
    const std::string defaultdir_Path = "./filestore/";
    return defaultdir_Path;
}
inline std::string generatePath(const std::string &fileId)
{
    return GetFileRecordDirName() + fileId + ".dat";
}

FileRecord::FileRecord(std::string id, FileStoreStatus s, std::string m, uint64_t size, std::string p)
    : fileid(id), status(s), md5(std::move(m)), path(std::move(p)), filesize(size) {}

FileRecordStore *FileRecordStore::Instance()
{
    static FileRecordStore *instance = new FileRecordStore();
    return instance;
}

FileRecordStore::FileRecordStore()
{
    FileIOHandler::CreateFolder(GetFileRecordDirName());
    filePath_ = GetFileRecordDirName() + "FileRecord";
    loadRecords();
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

    FileRecord *re = nullptr;
    if (records_.Find(fileId, re))
    {
        return false;
    }

    std::string actualPath = generatePath(fileId);

    re = new FileRecord{fileId, FileStoreStatus::NOT_UPLOADED, md5, filesize, actualPath};
    bool result = records_.Insert(fileId, re);
    if (!result)
    {
        SAFE_DELETE(re);
        return result;
    }
    saveRecords();

    FileRecord test;
    bool find = getRecord(fileId, test);

    return result;
}

// 删除文件记录
void FileRecordStore::deleteFileRecord(const std::string &fileId)
{
    records_.Erase(fileId);
    saveRecords();
}

// 更新文件状态
bool FileRecordStore::updateFileRecordStatus(const std::string &fileId, FileStoreStatus newStatus)
{
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
        uint64_t filesize;
        int statusValue;

        // 新格式: fileId status md5 filesize path
        if (iss >> fileId >> statusValue >> md5 >> filesize >> std::ws &&
            std::getline(iss, path))
        {
            records_.Insert(
                fileId,
                new FileRecord{
                    fileId,
                    static_cast<FileStoreStatus>(statusValue),
                    std::move(md5),
                    filesize,
                    std::move(path)});
        }
    }
}

void FileRecordStore::saveRecords()
{
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
                   << record->path << "\n";
       } });
}