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

std::vector<FileTransferChunkInfo> mergeChunks(const std::vector<FileTransferChunkInfo> &chunks);
std::vector<FileTransferChunkInfo> getUntransferredChunks(const std::vector<FileTransferChunkInfo> &transferredChunks, uint64_t totalFileSize);

class FileTransferTask
{
public:
    FileTransferTask(const string &filepath, const string &taskid);
    virtual ~FileTransferTask();

    const FileIOHandler &FileHandler();

protected:
    virtual void OnError() = 0;
    virtual void OnFinished() = 0;
    virtual void OnInterrupted() = 0;

protected:
    string file_path;
    string task_id;
    FileIOHandler file_io;
    uint64_t file_size = 0;
    vector<FileTransferChunkInfo> chunk_map;

    bool IsError = false;
    bool IsFinished = false;
    bool IsInterrupted = false;

    CriticalSectionLock InterruptedLock;
};
