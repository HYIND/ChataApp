#include "FileTransferTask.h"

std::vector<uint8_t> FileTransferChunkData::ToBinary()
{
    std::vector<uint8_t> result;
    result.resize(buf.Length());
    memcpy(result.data(), buf.Data(), buf.Length());
    return result;
}

void FileTransferChunkData::FromBinary(const std::vector<uint8_t> &datas)
{
    buf.ReSize(datas.size());
    memcpy(buf.Data(), datas.data(), datas.size());
}

// 比较函数，用于排序
bool compareChunk(const FileTransferChunkInfo &a, const FileTransferChunkInfo &b)
{
    return a.range_left < b.range_left;
}

// 合并重叠或连续的区间
std::vector<FileTransferChunkInfo> mergeChunks(const std::vector<FileTransferChunkInfo> &chunks)
{
    if (chunks.empty())
        return {};

    std::vector<FileTransferChunkInfo> sorted = chunks;
    std::sort(sorted.begin(), sorted.end(), compareChunk);

    std::vector<FileTransferChunkInfo> merged;
    uint64_t current_left = sorted[0].range_left;
    uint64_t current_right = sorted[0].range_right;
    int index = 0;

    for (size_t i = 1; i < sorted.size(); ++i)
    {
        if (sorted[i].range_left <= current_right + 1)
        {
            // 重叠或连续，合并
            current_right = std::max(current_right, sorted[i].range_right);
        }
        else
        {
            // 不连续，保存当前区间
            merged.emplace_back(index, current_left, current_right);
            index++;
            current_left = sorted[i].range_left;
            current_right = sorted[i].range_right;
        }
    }
    merged.emplace_back(index, current_left, current_right);

    return merged;
}

// 获取未传输的 chunk 区间
std::vector<FileTransferChunkInfo> getUntransferredChunks(
    const std::vector<FileTransferChunkInfo> &transferredChunks,
    uint64_t totalFileSize)
{
    // 合并已传输的区间
    auto merged = mergeChunks(transferredChunks);

    std::vector<FileTransferChunkInfo> untransferred;

    // 处理第一个区间之前的部分
    uint64_t prev_end = 0;
    if (!merged.empty())
    {
        if (merged[0].range_left > 0)
        {
            untransferred.emplace_back(0, 0, merged[0].range_left - 1);
        }
        prev_end = merged[0].range_right + 1;
    }
    else
    {
        // 没有任何已传输的 chunk，整个文件都未传输
        untransferred.emplace_back(0, 0, totalFileSize - 1);
        return untransferred;
    }

    // 处理中间的空隙
    for (size_t i = 1; i < merged.size(); ++i)
    {
        if (merged[i].range_left > prev_end)
        {
            untransferred.emplace_back(0, prev_end, merged[i].range_left - 1);
        }
        prev_end = merged[i].range_right + 1;
    }

    // 处理最后一个区间之后的部分
    if (prev_end < totalFileSize)
    {
        untransferred.emplace_back(0, prev_end, totalFileSize - 1);
    }

    return untransferred;
}

FileTransferTask::FileTransferTask(const string &filepath, const string &taskid)
{
    file_path = filepath;
    task_id = taskid;
}
FileTransferTask::~FileTransferTask()
{
    file_io.Close();
}

const FileIOHandler &FileTransferTask::FileHandler()
{
    return file_io;
}
