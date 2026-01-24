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

QString getFilenameFromPath(const QString &qfilepath)
{
    const std::string filepath = qfilepath.toStdString();

    // 处理 Windows 和 Unix 风格的路径分隔符
    size_t pos1 = filepath.find_last_of('/');
    size_t pos2 = filepath.find_last_of('\\');

    size_t pos = (pos1 == std::string::npos) ? pos2 : ((pos2 == std::string::npos) ? pos1 : max(pos1, pos2));

    if (pos != std::string::npos)
    {
        return QString::fromStdString(filepath.substr(pos + 1));
    }
    return QString::fromStdString(filepath);
}

uint64_t GetSuggestChunsize(uint64_t file_size)
{
    const uint64_t KB = 1024;
    const uint64_t MB = KB * 1024;

    const uint64_t minchunksize = 500 * KB;
    const uint64_t maxchunksize = 2 * MB;

    if (file_size <= minchunksize)
        return max((uint64_t)1, file_size);

    else
    {
        return min(maxchunksize, file_size / (uint64_t)20);
    }
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
            current_right = max(current_right, sorted[i].range_right);
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

uint32_t CountProgress(const std::vector<FileTransferChunkInfo> &chunks, uint64_t totalFileSize)
{
    if (chunks.empty())
        return 0;

    std::vector<FileTransferChunkInfo> sorted = chunks;
    std::sort(sorted.begin(), sorted.end(), compareChunk);

    std::vector<FileTransferChunkInfo> merged;
    uint64_t current_left = sorted[0].range_left;
    uint64_t current_right = sorted[0].range_right;

    uint64_t hastrans = 0;

    for (size_t i = 1; i < sorted.size(); ++i)
    {
        if (sorted[i].range_left <= current_right + 1)
        {
            // 重叠或连续，合并
            current_right = max(current_right, sorted[i].range_right);
        }
        else
        {
            hastrans += (current_right - current_left + 1);
            // 不连续，保存当前区间
            current_left = sorted[i].range_left;
            current_right = sorted[i].range_right;
        }
    }
    hastrans += (current_right - current_left + 1);

    uint32_t progress = (uint32_t)(((float)hastrans / (float)totalFileSize) * 100);

    progress = max(progress, (uint32_t)0);
    progress = min(progress, (uint32_t)100);

    return progress;
}

FileTransferTask::FileTransferTask(const QString &taskid, const QString &filepath, const QString &md5)
{
    file_path = filepath;
    _md5 = md5;
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

const QString FileTransferTask::TaskId()
{
    return task_id;
}

const QString FileTransferTask::FilePath()
{
    return file_path;
}
