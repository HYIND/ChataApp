#pragma once

#include "stdafx.h"

class FileIOHandler
{
public:
    enum OpenMode
    {
        READ_ONLY = O_RDONLY,
        WRITE_ONLY = O_WRONLY | O_CREAT | O_TRUNC,
        READ_WRITE = O_RDWR | O_CREAT,
        APPEND = O_WRONLY | O_APPEND | O_CREAT,
        TRUNCATE = O_WRONLY | O_TRUNC
    };

    enum SeekOrigin
    {
        BEGIN = SEEK_SET,
        CURRENT = SEEK_CUR,
        END = SEEK_END
    };

public:
    FileIOHandler();
    FileIOHandler(const std::string &filepath, OpenMode mode);
    virtual ~FileIOHandler();

public:
    // 文件操作接口
    bool Open(const std::string &path, OpenMode mode);
    void Close();

    bool IsOpen() const;
    std::string FilePath() const;

public:
    long Read(char *buf, size_t bytesToRead);
    long Read(Buffer &buf, size_t bytesToRead);
    long Write(const char *buf, size_t bytesToWrite);
    long Write(const Buffer &buf);
    long Seek(SeekOrigin origin, long offset = 0);

    bool Flush();
    long GetSize() const;
    bool Truncate(long size);

public:
    static bool Exists(const std::string &path);
    static bool Remove(const std::string &path);
    static bool CreateFolder(const std::string &path);

private:
    bool CheckOpen() const;

private:
    int fd = -1;
    std::string file_path;
    OpenMode mode;
    long offset;
    mutable CriticalSectionLock mutex;
    mode_t file_mode_ = 0644;
};