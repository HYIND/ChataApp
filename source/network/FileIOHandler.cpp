#include "FileIOHandler.h"
#include <QDebug>
#include <QDir>

FileIOHandler::FileIOHandler()
    : mode_(READ_ONLY), offset_(0)
{
}

FileIOHandler::FileIOHandler(const QString& filepath, OpenMode mode)
    : mode_(mode), offset_(0)
{
    Open(filepath, mode);
}

FileIOHandler::FileIOHandler(const std::string &filepath, OpenMode mode)
    : mode_(mode), offset_(0)
{
    Open(QString::fromStdString(filepath), mode);
}

FileIOHandler::~FileIOHandler()
{
    Close();
}

bool FileIOHandler::Open(const QString& path, OpenMode mode)
{
    mutex_.Enter();

    bool result = false;
    try {
        Close();

        mode_ = mode;
        offset_ = 0;

        file_.setFileName(path);
        if (file_.open(static_cast<QIODevice::OpenMode>(mode))) {
            result = true;
        } else {
            qWarning() << "Failed to open file:" << path << "Error:" << file_.errorString();
        }
    } catch (...) {
    }

    mutex_.Leave();
    return result;
}

void FileIOHandler::Close()
{
    mutex_.Enter();

    try {
        if (file_.isOpen()) {
            file_.close();
        }
        offset_ = 0;
    } catch (...) {
    }

    mutex_.Leave();
}

bool FileIOHandler::IsOpen() const
{
    mutex_.Enter();
    bool isOpen = file_.isOpen();
    mutex_.Leave();
    return isOpen;
}

QString FileIOHandler::FilePath() const
{
    mutex_.Enter();
    QString path = file_.fileName();
    mutex_.Leave();
    return path;
}

qint64 FileIOHandler::Read(char* buf, qint64 bytesToRead)
{
    mutex_.Enter();

    qint64 result = 0;
    try {
        if (!CheckOpen()) {
            mutex_.Leave();
            return 0;
        }

        result = file_.read(buf, bytesToRead);
        if (result == -1) {
            qWarning() << "Read failed:" << file_.errorString();
            result = 0;
        } else {
            offset_ += result;
        }
    } catch (...) {
    }

    mutex_.Leave();
    return result;
}


qint64 FileIOHandler::Read(Buffer& buffer, qint64 bytesToRead)
{
    // 确保Buffer有足够空间
    if (buffer.Remain() < static_cast<int>(bytesToRead))
    {
        buffer.ReSize(buffer.Position() + bytesToRead);
    }
    qint64 result = Read(buffer.Byte() + buffer.Position(), bytesToRead);
    return result;
}

qint64 FileIOHandler::Read(Buffer &buffer)
{
    LockGuard guard(mutex_);
    if (GetSize() <= 0)
        return 0;

    qint64 bytesToRead = GetSize();
    // 确保Buffer有足够空间
    if (buffer.Remain() < static_cast<uint64_t>(bytesToRead))
    {
        buffer.ReSize(buffer.Position() + bytesToRead);
    }
    Seek(0);
    qint64 result = Read(buffer.Byte() + buffer.Position(), bytesToRead);
    return result;
}

qint64 FileIOHandler::Write(const char* buf, qint64 bytesToWrite)
{
    mutex_.Enter();

    qint64 result = 0;
    try {
        if (!CheckOpen()) {
            mutex_.Leave();
            return 0;
        }

        result = file_.write(buf, bytesToWrite);
        if (result == -1) {
            qWarning() << "Write failed:" << file_.errorString();
            result = 0;
        } else {
            offset_ += result;
        }
    } catch (...) {
        mutex_.Leave();
        throw;
    }

    mutex_.Leave();
    return result;
}

qint64 FileIOHandler::Write(const Buffer& buf)
{
    return Write(buf.Byte(), buf.Length());
}

qint64 FileIOHandler::Seek(qint64 offset)
{
    mutex_.Enter();

    qint64 result = -1;
    try {
        if (!CheckOpen()) {
            mutex_.Leave();
            return -1;
        }

        if (file_.seek(offset)) {
            offset_ = file_.pos();
            result = offset_;
        } else {
            qWarning() << "Seek failed:" << file_.errorString();
        }
    } catch (...) {
    }

    mutex_.Leave();
    return result;
}

bool FileIOHandler::Flush()
{
    mutex_.Enter();

    bool result = false;
    try {
        if (!CheckOpen()) {
            mutex_.Leave();
            return false;
        }
        result = file_.flush();
    } catch (...) {
    }

    mutex_.Leave();
    return result;
}

qint64 FileIOHandler::GetSize() const
{
    mutex_.Enter();

    qint64 size = 0;
    try {
        if (file_.isOpen()) {
            size = file_.size();
        }
    } catch (...) {
    }

    mutex_.Leave();
    return size;
}

bool FileIOHandler::Truncate(qint64 size)
{
    mutex_.Enter();

    bool result = false;
    try {
        if (!CheckOpen()) {
            mutex_.Leave();
            return false;
        }
        result = file_.resize(size);
    } catch (...) {
    }

    mutex_.Leave();
    return result;
}

bool FileIOHandler::Exists(const QString& path)
{
    return QFile::exists(path);
}

bool FileIOHandler::Remove(const QString& path)
{
    return QFile::remove(path);
}

bool FileIOHandler::CheckOpen() const
{
    if (!file_.isOpen()) {
        qWarning() << "File is not open";
        return false;
    }
    return true;
}

bool FileIOHandler::CreateFolder(const QString& path) {
    QDir dir;
    if (dir.exists(path))
        return true;
    return dir.mkpath(path);
}
