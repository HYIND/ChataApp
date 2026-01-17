#pragma once

#include "stdafx.h"
#include "FileIOHandler.h"

using namespace std;

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
    void FromBinary(const std::vector<uint8_t> &datas);
};

std::string getFilenameFromPath(const std::string &filepath);
std::vector<FileTransferChunkInfo> mergeChunks(const std::vector<FileTransferChunkInfo> &chunks);
std::vector<FileTransferChunkInfo> getUntransferredChunks(const std::vector<FileTransferChunkInfo> &transferredChunks, uint64_t totalFileSize);
uint32_t CountProgress(const std::vector<FileTransferChunkInfo> &chunks, uint64_t totalFileSize);
uint64_t GetSuggestChunsize(uint64_t file_size);

class FileTransferTask
{
public:
    FileTransferTask(const string &taskid, const string &filepath, const string &md5);
    virtual ~FileTransferTask();

    const FileIOHandler &FileHandler();
    const string TaskId();

public:
    virtual void ProcessMsg(BaseNetWorkSession *session, const json &js, Buffer &buf) = 0;
    virtual void ReleaseSource() = 0;
    virtual void InterruptTrans(BaseNetWorkSession *session) = 0;

protected:
    virtual void OnError() = 0;
    virtual void OnFinished() = 0;
    virtual void OnInterrupted() = 0;
    virtual void OnProgress(uint32_t progress) = 0;

    virtual void OccurError(BaseNetWorkSession *session) = 0;
    virtual void OccurFinish() = 0;
    virtual void OccurInterrupt() = 0;
    virtual void OccurProgressChange() = 0;

    virtual uint32_t Progress() = 0;

protected:
    string file_path;
    string _md5;
    string task_id;
    FileIOHandler file_io;
    uint64_t file_size = 0;
    vector<FileTransferChunkInfo> chunk_map;

    bool IsError = false;
    bool IsFinished = false;
    bool IsInterrupted = false;

    bool IsFileEnable = false;
    bool IsTransFinish = false;

    bool IsNetworkEnable = true;

    CriticalSectionLock InterruptedLock;
};
