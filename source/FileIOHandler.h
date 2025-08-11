#pragma once

#include <QFile>
#include <QString>
#include "Buffer.h"
#include "CriticalSectionLock.h"

class FileIOHandler
{
public:
    enum OpenMode
    {
        READ_ONLY = QIODevice::ReadOnly,
        WRITE_ONLY = QIODevice::WriteOnly | QIODevice::Truncate,
        READ_WRITE = QIODevice::ReadWrite,
        APPEND = QIODevice::Append,
        TRUNCATE = QIODevice::Truncate
    };

public:
    FileIOHandler();
    explicit FileIOHandler(const QString& filepath, OpenMode mode);
    virtual ~FileIOHandler();

    // 禁止拷贝和赋值
    FileIOHandler(const FileIOHandler&) = delete;
    FileIOHandler& operator=(const FileIOHandler&) = delete;

public:
    // 文件操作接口
    bool Open(const QString& path, OpenMode mode);
    void Close();

    bool IsOpen() const;
    QString FilePath() const;

    qint64 Read(char* buf, qint64 bytesToRead);
    qint64 Read(Buffer& buf, qint64 bytesToRead);
    qint64 Write(const char* buf, qint64 bytesToWrite);
    qint64 Write(const Buffer& buf);
    qint64 Seek(qint64 offset = 0);

    bool Flush();
    qint64 GetSize() const;
    bool Truncate(qint64 size);

public:
    static bool Exists(const QString& path);
    static bool Remove(const QString& path);
    static bool CreateFolder(const QString& path);

private:
    bool CheckOpen() const;

private:
    QFile file_;
    OpenMode mode_;
    qint64 offset_;
    mutable CriticalSectionLock mutex_;
};
