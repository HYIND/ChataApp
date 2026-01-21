#include "MessageRecordStore.h"
#include "FileIOHandler.h"
#include "LoginUserManager.h"
#include <sys/stat.h>
#include <unistd.h>
#include "Timer.h"

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

inline std::string GetMessageRecordDirName()
{
    const std::string dir_path = "./chatrecord/";
    return dir_path;
}

inline std::string GetSessionFilePath(const std::string &srctoken,
                                      const std::string &goaltoken)
{
    string filename;
    if (LoginUserManager::IsPublicChat(goaltoken))
        filename = "publicchat_record";
    else
    {
        auto tokens = {srctoken, goaltoken};
        std::vector<std::string> sorted(tokens);
        std::sort(sorted.begin(), sorted.end());
        filename = sorted[0] + "_" + sorted[1] + "_record";
    }
    return ConcatPath(GetMessageRecordDirName(), filename);
}

class MsgSerializeHelper
{
public:
    static Buffer SerializeMsg(const MsgRecord &msg)
    {

        Buffer buf;

        int srctokenlen = msg.srctoken.length();
        int goaltokenlen = msg.goaltoken.length();
        int namelen = msg.name.length();
        int iplen = msg.ip.length();
        int msglen = msg.msg.length();
        int filenamelen = msg.filename.length();
        int md5len = msg.md5.length();
        int fileidlen = msg.fileid.length();
        int totallen = srctokenlen + goaltokenlen +
                       namelen + sizeof(msg.time) + iplen + sizeof(msg.port) +
                       sizeof(msg.type) + msglen + filenamelen + sizeof(msg.filesize) + md5len + fileidlen;

        buf.ReSize(totallen);

        buf.Write(&srctokenlen, sizeof(srctokenlen));
        buf.Write(&goaltokenlen, sizeof(goaltokenlen));
        buf.Write(&namelen, sizeof(namelen));
        buf.Write(&iplen, sizeof(iplen));
        buf.Write(&msglen, sizeof(msglen));
        buf.Write(&filenamelen, sizeof(filenamelen));
        buf.Write(&md5len, sizeof(md5len));
        buf.Write(&fileidlen, sizeof(fileidlen));

        buf.Write(msg.srctoken);
        buf.Write(msg.goaltoken);
        buf.Write(msg.name);
        buf.Write(&(msg.time), sizeof(msg.time));
        buf.Write(msg.ip);
        buf.Write(&(msg.port), sizeof(msg.port));
        buf.Write(&(msg.type), sizeof(msg.type));
        buf.Write(msg.msg);
        buf.Write(msg.filename);
        buf.Write(&(msg.filesize), sizeof(msg.filesize));
        buf.Write(msg.md5);
        buf.Write(msg.fileid);

        return buf;
    };
    // 消息反序列化
    static MsgRecord DeserializeMsg(Buffer &buf)
    {
        MsgRecord msg;

        int srctokenlen = 0;
        int goaltokenlen = 0;
        int namelen = 0;
        int iplen = 0;
        int msglen = 0;
        int filenamelen = 0;
        int md5len = 0;
        int fileidlen = 0;
        buf.Read(&srctokenlen, sizeof(srctokenlen));
        buf.Read(&goaltokenlen, sizeof(goaltokenlen));
        buf.Read(&namelen, sizeof(namelen));
        buf.Read(&iplen, sizeof(iplen));
        buf.Read(&msglen, sizeof(msglen));
        buf.Read(&filenamelen, sizeof(filenamelen));
        buf.Read(&md5len, sizeof(md5len));
        buf.Read(&fileidlen, sizeof(fileidlen));

        int totallen = srctokenlen + goaltokenlen +
                       namelen + sizeof(msg.time) + iplen + sizeof(msg.port) +
                       sizeof(msg.type) + msglen + filenamelen + sizeof(msg.filesize) + md5len + fileidlen;

        if (buf.Length() < totallen)
            return MsgRecord{};

        msg.srctoken = DeserializeToString(buf, srctokenlen);
        msg.goaltoken = DeserializeToString(buf, goaltokenlen);
        msg.name = DeserializeToString(buf, namelen);
        buf.Read(&(msg.time), sizeof(msg.time));
        msg.ip = DeserializeToString(buf, iplen);
        buf.Read(&(msg.port), sizeof(msg.port));
        buf.Read(&(msg.type), sizeof(msg.type));
        msg.msg = DeserializeToString(buf, msglen);
        msg.filename = DeserializeToString(buf, filenamelen);
        buf.Read(&(msg.filesize), sizeof(msg.filesize));
        msg.md5 = DeserializeToString(buf, md5len);
        msg.fileid = DeserializeToString(buf, fileidlen);

        return msg;
    };

private:
    static string DeserializeToString(Buffer &buf, int size)
    {
        char *ch = new char[size];
        buf.Read(ch, size);
        string result(ch, size);
        SAFE_DELETE_ARRAY(ch);
        return result;
    };
};

MessageRecordStore *MessageRecordStore::Instance()
{
    static MessageRecordStore *m_instance = new MessageRecordStore();
    return m_instance;
}

MessageRecordStore::MessageRecordStore()
{
    FileIOHandler::CreateFolder(GetMessageRecordDirName());
    static constexpr uint64_t cleaninterval = 30 * 1000, firstclean = 10 * 1000;
    CleanExpiredTask = TimerTask::CreateRepeat("CleanExpiredMsgTimer", cleaninterval, std::bind(&MessageRecordStore::CleanExpiredMsg, this), firstclean);
    CleanExpiredTask->Run();
}

bool MessageRecordStore::StoreMsg(const MsgRecord &msg)
{
    if (!enable)
        return true;

    bool result = false;
    if (msg.srctoken == "" || msg.goaltoken == "")
        return result;

    if (!FileIOHandler::Exists(GetMessageRecordDirName()))
        FileIOHandler::CreateFolder(GetMessageRecordDirName());

    string filepath = GetSessionFilePath(msg.srctoken, msg.goaltoken);

    std::unique_lock<std::shared_mutex> filelock(GetFileMutex(filepath));

    FileIOHandler handler;
    if (handler.Open(filepath, FileIOHandler::OpenMode::APPEND))
    {
        Buffer buf = MsgSerializeHelper::SerializeMsg(msg);
        int buflen = buf.Length();

        int writecount = 0;
        writecount += handler.Write((char *)(&buflen), sizeof(buflen));
        writecount += handler.Write(buf);
        writecount += handler.Write((char *)(&buflen), sizeof(buflen));

        result = writecount == (sizeof(buflen) + buf.Length() + sizeof(buflen));
    }

    return result;
}

vector<MsgRecord> MessageRecordStore::FetchAllMsg(const string &srctoken, const string &goaltoken)
{
    vector<MsgRecord> result;

    if (goaltoken == "")
        return result;
    if (srctoken == "" && goaltoken != LoginUserManager::PublicChatToken())
        return result;

    string filepath = GetSessionFilePath(srctoken, goaltoken);

    if (!FileIOHandler::Exists(filepath))
        return result;

    std::shared_lock<std::shared_mutex> filelock(GetFileMutex(filepath));

    FileIOHandler handler(filepath, FileIOHandler::OpenMode::READ_ONLY);

    long remind = handler.GetSize();

    while (remind > sizeof(int))
    {
        int beginbuflen = 0;
        handler.Read((char *)(&beginbuflen), sizeof(beginbuflen));
        remind -= sizeof(beginbuflen);

        if (remind < beginbuflen)
            break;

        Buffer buf;
        handler.Read(buf, beginbuflen);
        remind -= beginbuflen;

        MsgRecord msgrecord = MsgSerializeHelper::DeserializeMsg(buf);

        result.emplace_back(msgrecord);

        int lastbuflen = 0;
        if (remind < sizeof(lastbuflen))
            break;
        handler.Read((char *)(&lastbuflen), sizeof(lastbuflen));
        remind -= sizeof(lastbuflen);
    }

    return result;
}

// 读取末尾的N条消息
vector<MsgRecord> MessageRecordStore::FetchLastMsg(const string &srctoken, const string &goaltoken, uint32_t count)
{
    vector<MsgRecord> result;

    if (goaltoken == "")
        return result;
    if (srctoken == "" && goaltoken != LoginUserManager::PublicChatToken())
        return result;

    string filepath = GetSessionFilePath(srctoken, goaltoken);

    if (!FileIOHandler::Exists(filepath))
        return result;

    std::shared_lock<std::shared_mutex> filelock(GetFileMutex(filepath));

    FileIOHandler handler(filepath, FileIOHandler::OpenMode::READ_ONLY);

    long remind = handler.GetSize();
    long offset = remind;

    while (remind > sizeof(int) && count > 0)
    {
        int lastbuflen = 0;

        offset -= sizeof(lastbuflen);
        handler.Seek(FileIOHandler::SeekOrigin::BEGIN, offset);

        handler.Read((char *)(&lastbuflen), sizeof(lastbuflen));
        remind -= sizeof(lastbuflen);

        if (remind < lastbuflen)
            break;

        Buffer buf;
        offset -= lastbuflen;
        handler.Seek(FileIOHandler::SeekOrigin::BEGIN, offset);

        handler.Read(buf, lastbuflen);
        remind -= lastbuflen;

        MsgRecord msgrecord = MsgSerializeHelper::DeserializeMsg(buf);

        int beginbuflen = 0;
        if (remind < sizeof(beginbuflen))
            break;

        offset -= sizeof(beginbuflen);
        handler.Seek(FileIOHandler::SeekOrigin::BEGIN, offset);

        handler.Read((char *)(&beginbuflen), sizeof(beginbuflen));
        remind -= sizeof(beginbuflen);

        if (beginbuflen == lastbuflen)
        {
            result.emplace_back(msgrecord);
            count--;
        }
    }
    std::reverse(result.begin(), result.end());
    return result;
}

void MessageRecordStore::SetEnable(bool value)
{
    enable = value;
}

std::shared_mutex &MessageRecordStore::GetFileMutex(const std::string &filepath)
{
    return _fileMutexes[filepath];
}

void MessageRecordStore::CleanExpiredMsg()
{
    if (!enable)
        return;

    try
    {
        int64_t currentTime = GetTimeStampSecond();

        int64_t expiredTime = currentTime - msgexpiredseconds;

        std::string recordDir = GetMessageRecordDirName();

        std::vector<std::string> files;
        if (FileIOHandler::ListFiles(recordDir, files))
        {
            for (const auto &filepath : files)
            {
                // 只处理以"_record"结尾的文件
                if (filepath.find("_record") == std::string::npos)
                    continue;

                // 打开文件读取所有消息
                std::vector<MsgRecord> readMessages;

                std::unique_lock<std::shared_mutex> filelock(GetFileMutex(filepath));

                FileIOHandler handler(filepath, FileIOHandler::OpenMode::READ_ONLY);

                bool needclean = false;

                long remind = handler.GetSize();
                long offset = remind;

                while (remind > sizeof(int))
                {
                    int lastbuflen = 0;

                    offset -= sizeof(lastbuflen);
                    handler.Seek(FileIOHandler::SeekOrigin::BEGIN, offset);

                    handler.Read((char *)(&lastbuflen), sizeof(lastbuflen));
                    remind -= sizeof(lastbuflen);

                    if (remind < lastbuflen)
                        break;

                    Buffer buf;
                    offset -= lastbuflen;
                    handler.Seek(FileIOHandler::SeekOrigin::BEGIN, offset);

                    handler.Read(buf, lastbuflen);
                    remind -= lastbuflen;

                    MsgRecord msgrecord = MsgSerializeHelper::DeserializeMsg(buf);

                    if (msgrecord.time < expiredTime)
                    {
                        needclean = true;
                        break;
                    }

                    int beginbuflen = 0;
                    if (remind < sizeof(beginbuflen))
                        break;

                    offset -= sizeof(beginbuflen);
                    handler.Seek(FileIOHandler::SeekOrigin::BEGIN, offset);

                    handler.Read((char *)(&beginbuflen), sizeof(beginbuflen));
                    remind -= sizeof(beginbuflen);

                    if (beginbuflen == lastbuflen)
                    {
                        readMessages.emplace_back(msgrecord);
                    }
                }
                std::reverse(readMessages.begin(), readMessages.end());

                handler.Close();

                if (!needclean)
                    break;

                // 如果有消息被删除，需要重写文件
                if (readMessages.size() > 0)
                {
                    // 创建新文件
                    FileIOHandler newHandler;
                    if (newHandler.Open(filepath + ".tmp", FileIOHandler::OpenMode::WRITE_ONLY))
                    {
                        int writecount = 0, buflencount = 0;
                        // 写入未过期的消息
                        for (const auto &msg : readMessages)
                        {
                            Buffer buf = MsgSerializeHelper::SerializeMsg(msg);
                            int buflen = buf.Length();

                            writecount += newHandler.Write((char *)(&buflen), sizeof(buflen));
                            writecount += newHandler.Write(buf);
                            writecount += newHandler.Write((char *)(&buflen), sizeof(buflen));

                            buflencount += (sizeof(buflen) + buflen + sizeof(buflen));
                        }
                        newHandler.Close();

                        if (writecount == buflencount)
                        {
                            FileIOHandler::Remove(filepath);
                            FileIOHandler::RenameFile(filepath + ".tmp", filepath); // 用新文件替换旧文件
                        }
                        else
                        {
                            FileIOHandler::Remove(filepath + ".tmp");
                        }
                    }
                }
                else
                {
                    FileIOHandler::Remove(filepath); // 所有消息都已过期，删除文件
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        // 可以在这里添加日志记录
        std::cerr << "CleanExpiredMsg error: " << e.what() << std::endl;
    }
}
