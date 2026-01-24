
#include "FileIOHandler.h"
#include <system_error>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

FileIOHandler::FileIOHandler()
{
    _fd = -1;
    _offset = 0;
    _filemode = 0644;
}

FileIOHandler::FileIOHandler(const std::string &filepath, OpenMode mode)
    : FileIOHandler()
{
    Open(filepath, mode);
}

FileIOHandler::~FileIOHandler()
{
    Close();
}

bool FileIOHandler::Open(const std::string &path, OpenMode mode)
{
    LockGuard guard(_mutex);
    bool result = false;
    try
    {
        Close();

        _filepath = path;
        _mode = mode;
        _offset = 0;

        _fd = ::open(path.c_str(), mode, _filemode);
        if (_fd == -1)
            return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return true;
}

void FileIOHandler::Close()
{
    LockGuard guard(_mutex);
    try
    {
        if (_fd != -1)
        {
            ::close(_fd);
            _fd = -1;
        }
        _filepath.clear();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

bool FileIOHandler::IsOpen() const
{
    return _fd != -1;
}

std::string FileIOHandler::FilePath() const
{
    return _filepath;
}

long FileIOHandler::Read(char *buf, size_t bytesToRead)
{
    LockGuard guard(_mutex);
    long result = 0;
    try
    {
        if (!CheckOpen())
            return result;

        result = ::read(_fd, (void *)buf, bytesToRead);
        if (result < 0)
        {
            throw std::system_error(errno, std::system_category(), "Read failed");
        }
        _offset += result;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return result;
}

long FileIOHandler::Read(Buffer &buffer, size_t bytesToRead)
{
    LockGuard guard(_mutex);
    // 确保Buffer有足够空间
    if (buffer.Remain() < static_cast<int>(bytesToRead))
    {
        buffer.ReSize(buffer.Position() + bytesToRead);
    }
    int result = Read(buffer.Byte() + buffer.Position(), bytesToRead);
    return result;
}

long FileIOHandler::Read(Buffer &buffer)
{
    LockGuard guard(_mutex);
    if (GetSize() <= 0)
        return 0;

    size_t bytesToRead = GetSize();
    // 确保Buffer有足够空间
    if (buffer.Remain() < static_cast<uint64_t>(bytesToRead))
    {
        buffer.ReSize(buffer.Position() + bytesToRead);
    }
    Seek(SeekOrigin::BEGIN, 0);
    int result = Read(buffer.Byte() + buffer.Position(), bytesToRead);
    return result;
}

long FileIOHandler::Write(const char *buf, size_t bytesToWrite)
{
    LockGuard guard(_mutex);
    long result = 0;
    try
    {
        if (!CheckOpen())
            return result;
        result = ::write(_fd, (const void *)buf, bytesToWrite);
        if (result < 0)
        {
            throw std::system_error(errno, std::system_category(), "Write failed");
        }
        _offset += result;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return result;
}

long FileIOHandler::Write(const Buffer &buf)
{
    return Write(buf.Byte(), buf.Length());
}

long FileIOHandler::Seek(SeekOrigin origin, long offset)
{
    LockGuard guard(_mutex);
    long result = 0;
    try
    {
        if (!CheckOpen())
            return result;

        result = ::lseek(_fd, offset, origin);
        if (result == -1)
        {
            throw std::system_error(errno, std::system_category(), "Seek failed");
        }
        _offset = result;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return result;
}

bool FileIOHandler::Flush()
{
    LockGuard guard(_mutex);
    bool result = false;
    try
    {
        if (!CheckOpen())
            return false;
        result = ::fsync(_fd) == 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return result;
}

long FileIOHandler::GetSize() const
{
    LockGuard guard(_mutex);
    long result = 0;
    try
    {
        if (!CheckOpen())
            return result;

        struct stat64 st;
        if (::fstat64(_fd, &st) == -1)
        {
            throw std::system_error(errno, std::system_category(), "GetSize failed");
        }
        result = st.st_size;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return result;
}

bool FileIOHandler::Truncate(off_t size)
{
    LockGuard guard(_mutex);
    bool result = false;
    try
    {
        if (!CheckOpen())
            return result;

        result = ::ftruncate(_fd, size) == 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    return result;
}

bool FileIOHandler::Exists(const std::string &path)
{
    return ::access(path.c_str(), F_OK) == 0;
}

bool FileIOHandler::Remove(const std::string &path)
{
    return ::unlink(path.c_str()) == 0;
}

bool FileIOHandler::CreateFolder(const std::string &path)
{
    if (path.empty())
        return false;

    // 检查目录是否已存在
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
    {
        return S_ISDIR(st.st_mode); // 存在且是目录
    }

    // 递归创建目录
    size_t pos = 0;
    std::string dir;
    while ((pos = path.find_first_of('/', pos + 1)) != std::string::npos)
    {
        dir = path.substr(0, pos);
        if (dir.empty())
            continue; // 跳过根目录
        if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST)
        {
            return false; // 创建失败
        }
    }
    // 创建最后一层目录
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

bool FileIOHandler::ListFiles(const std::string &path, std::vector<std::string> &filepaths)
{
    DIR *dir = nullptr;
    struct dirent *entry = nullptr;
    struct stat fileStat;

    // 清空输出向量
    filepaths.clear();

    try
    {
        // 打开目录
        dir = ::opendir(path.c_str());
        if (!dir)
        {
            std::cerr << "Failed to open directory: " << path
                      << ", error: " << strerror(errno) << std::endl;
            return false;
        }

        // 读取目录中的所有条目
        while ((entry = ::readdir(dir)) != nullptr)
        {
            std::string filename(entry->d_name);

            // 跳过当前目录和父目录
            if (filename == "." || filename == "..")
                continue;

            // 构建完整路径
            std::string fullpath = path;
            if (!path.empty() && path.back() != '/')
                fullpath += '/';
            fullpath += filename;

            // 获取文件信息判断是否为普通文件
            if (::stat(fullpath.c_str(), &fileStat) == 0)
            {
                // 只添加普通文件，跳过目录
                if (S_ISREG(fileStat.st_mode))
                {
                    filepaths.push_back(fullpath);
                }
                // 如果也想包括符号链接文件，可以添加这个条件
                // else if (S_ISLNK(fileStat.st_mode))
                // {
                //     filepaths.push_back(fullpath);
                // }
            }
        }

        // 按文件名排序（可选）
        std::sort(filepaths.begin(), filepaths.end());

        // 关闭目录
        if (dir)
        {
            ::closedir(dir);
        }

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in ListFiles: " << e.what() << std::endl;

        if (dir)
        {
            ::closedir(dir);
        }

        return false;
    }
}

bool FileIOHandler::RenameFile(const std::string &oripath, const std::string &newpath)
{
    try
    {
        // 检查源文件是否存在
        if (!Exists(oripath))
        {
            std::cerr << "RenameFile Source file does not exist: " << oripath << std::endl;
            return false;
        }

        // 执行重命名
        int result = ::rename(oripath.c_str(), newpath.c_str());
        if (result == 0)
        {
            return true;
        }
        else
        {
            std::cerr << "RenameFile Fail! from " << oripath
                      << " to " << newpath
                      << ", error: " << strerror(errno) << std::endl;
            return false;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in RenameFile: " << e.what() << std::endl;
        return false;
    }
}

bool FileIOHandler::CheckOpen() const
{
    if (_fd == -1)
    {
        errno = EBADF;
        return false;
    }
    return true;
}