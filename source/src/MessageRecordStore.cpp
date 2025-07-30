#include "MessageRecordStore.h"
#include "FileIOHandler.h"
#include "LoginUserManager.h"
#include <sys/stat.h>
#include <unistd.h>

std::string GetFileDirName()
{
    const std::string dir_path = "./chatrecord/";

    // 检查目录是否存在 (F_OK检查存在性，R_OK|W_OK检查读写权限)
    if (access(dir_path.c_str(), F_OK | R_OK | W_OK) != 0)
    {
        // 创建目录 (0755权限: rwxr-xr-x)
        if (mkdir(dir_path.c_str(), 0755) != 0)
        {
            // 创建失败处理
            std::cerr << "Failed to create directory: " << dir_path
                      << " Error: " << strerror(errno) << std::endl;
        }
    }

    return dir_path;
}

std::string GetSessionFilePath(const std::string &srctoken,
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
    return GetFileDirName() + filename;
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
        int totallen = srctokenlen + goaltokenlen + namelen + sizeof(msg.time) + iplen + sizeof(msg.port) + sizeof(msg.type) + msglen;

        buf.ReSize(totallen);

        buf.Write(&srctokenlen, sizeof(srctokenlen));
        buf.Write(&goaltokenlen, sizeof(goaltokenlen));
        buf.Write(&namelen, sizeof(namelen));
        buf.Write(&iplen, sizeof(iplen));
        buf.Write(&msglen, sizeof(msglen));

        buf.Write(msg.srctoken);
        buf.Write(msg.goaltoken);
        buf.Write(msg.name);
        buf.Write(&(msg.time), sizeof(msg.time));
        buf.Write(msg.ip);
        buf.Write(&(msg.port), sizeof(msg.port));
        buf.Write(&(msg.type), sizeof(msg.type));
        buf.Write(msg.msg);

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
        buf.Read(&srctokenlen, sizeof(srctokenlen));
        buf.Read(&goaltokenlen, sizeof(goaltokenlen));
        buf.Read(&namelen, sizeof(namelen));
        buf.Read(&iplen, sizeof(iplen));
        buf.Read(&msglen, sizeof(msglen));

        int totallen = srctokenlen + goaltokenlen + namelen + sizeof(msg.time) + iplen + sizeof(msg.port) + sizeof(msg.type) + msglen;

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

        return msg;
    };

private:
    static string DeserializeToString(Buffer &buf, int size)
    {
        char *ch = new char[size];
        buf.Read(ch, size);
        return string(ch, size);
    };
};

MessageRecordStore *MessageRecordStore::Instance()
{
    static MessageRecordStore *m_instance = new MessageRecordStore();
    return m_instance;
}

bool MessageRecordStore::StoreMsg(const MsgRecord &msg)
{
    if (!enable)
        return true;

    bool result = false;
    if (msg.srctoken == "" || msg.goaltoken == "")
        return result;

    string filepath = GetSessionFilePath(msg.srctoken, msg.goaltoken);

    FileIOHandler handler;
    if (handler.Open(filepath, FileIOHandler::OpenMode::APPEND))
    {
        Buffer buf = MsgSerializeHelper::SerializeMsg(msg);
        int buflen = buf.Length();

        int writecount = 0;
        writecount += handler.Write(&buflen, sizeof(buflen));
        writecount += handler.Write(buf);

        result = writecount == (sizeof(buflen) + buf.Length());
    }

    vector<MsgRecord> re = FetchMsg(msg.srctoken, msg.goaltoken);

    return result;
}

vector<MsgRecord> MessageRecordStore::FetchMsg(const string &srctoken, const string &goaltoken)
{
    vector<MsgRecord> result;

    if (goaltoken == "")
        return result;
    if (srctoken == "" && goaltoken != LoginUserManager::PublicChatToken())
        return result;

    string filepath = GetSessionFilePath(srctoken, goaltoken);

    if (!FileIOHandler::Exists(filepath))
        return result;

    FileIOHandler handler(filepath, FileIOHandler::OpenMode::READ_ONLY);

    long remind = handler.GetSize();

    while (remind > sizeof(int))
    {
        int buflen = 0;
        handler.Read(&buflen, sizeof(buflen));
        remind -= sizeof(buflen);

        if (remind < buflen)
            break;

        Buffer buf;
        handler.Read(buf, buflen);
        remind -= buflen;

        MsgRecord msgrecord = MsgSerializeHelper::DeserializeMsg(buf);

        result.emplace_back(msgrecord);
    }

    return result;
}

void MessageRecordStore::SetEnable(bool value)
{
    enable = value;
}
