#include "FileTransferDownLoadTask.h"

void displayTransferProgress(uint64_t totalSize, const std::vector<FileTransferChunkInfo> &transferredChunks, int barWidth = 160)
{
    // 合并重叠或连续的 chunk
    std::vector<FileTransferChunkInfo> merged;
    if (!transferredChunks.empty())
    {
        auto sorted = transferredChunks;
        std::sort(sorted.begin(), sorted.end(), [](const FileTransferChunkInfo &a, const FileTransferChunkInfo &b)
                  { return a.range_left < b.range_left; });

        merged.push_back(sorted[0]);
        for (size_t i = 1; i < sorted.size(); ++i)
        {
            if (sorted[i].range_left <= merged.back().range_right + 1)
            {
                merged.back().range_right = std::max(merged.back().range_right, sorted[i].range_right);
            }
            else
            {
                merged.push_back(sorted[i]);
            }
        }
    }

    // 计算总传输量
    uint64_t totalTransferred = 0;
    for (const auto &chunk : merged)
    {
        totalTransferred += chunk.range_right - chunk.range_left + 1;
    }

    // 计算百分比
    double percent = totalSize > 0 ? (static_cast<double>(totalTransferred)) / totalSize * 100.0 : 0.0;

    // 显示进度条
    std::cout << "[";
    int pos = static_cast<int>(barWidth * percent / 100.0);

    // 绘制详细传输区域
    for (int i = 0; i < barWidth; ++i)
    {
        uint64_t start = i * totalSize / barWidth;
        uint64_t end = (i + 1) * totalSize / barWidth - 1;

        bool isTransferred = false;
        for (const auto &chunk : merged)
        {
            if (chunk.range_left <= end && chunk.range_right >= start)
            {
                isTransferred = true;
                break;
            }
        }

        if (isTransferred)
        {
            std::cout << "="; // 已传输部分
        }
        else
        {
            std::cout << " "; // 未传输部分
        }
    }

    bool flushenable = false;
    if (flushenable)
    {
        std::cout << "] " << static_cast<int>(percent) << "%\r";
        std::cout.flush();
    }
    else
    {
        std::cout << "] " << static_cast<int>(percent) << "%\n";
    }

    // sleep(1);
}

std::string getChunkFilePath(const std::string &filepath)
{
    return filepath + "__chunks";
}

bool ExtractBinaryJson(json jsbinary, std::vector<uint8_t> &v)
{
    bool result = false;
    std::vector<uint8_t> bytes;
    if (jsbinary.is_object() && jsbinary.contains("bytes"))
    {
        const auto &bytes_field = jsbinary["bytes"];
        if (bytes_field.is_array())
        {
            // 确保数组元素是数字
            // 注意：get<vector<uint8_t>>() 会自动处理数字到 uint8_t 的转换，并进行类型检查
            try
            {
                bytes = bytes_field.get<std::vector<uint8_t>>();
                result = true;
            }
            catch (const json::exception &e)
            {
                cout << "error when ExtractBinaryJson: " + std::string(e.what()) << endl;
            }
        }
    }
    if (result)
        v.insert(v.end(), bytes.begin(), bytes.end());
    return result;
}

FileTransferDownLoadTask::FileTransferDownLoadTask(const string &filepath, const string &taskid, uint64_t length)
    : FileTransferTask(filepath, taskid)
{
}
FileTransferDownLoadTask::~FileTransferDownLoadTask()
{
}

void FileTransferDownLoadTask::ReleaseSource()
{
    file_io.Close();
    chunk_map.clear();
    chunkfile_io.Close();
    // file_path.clear();
    // task_id.clear();
}

void FileTransferDownLoadTask::InterruptTrans(BaseNetWorkSession *session)
{
    InterruptedLock.Enter();

    if (!IsFinished)
    {
        json js_error;
        js_error["command"] = 7070;
        js_error["taskid"] = task_id;
        js_error["result"] = 0;

        Buffer buf = js_error.dump();
        session->AsyncSend(buf);

        OccurInterrupt();
    }

    InterruptedLock.Leave();
}

bool FileTransferDownLoadTask::WriteToChunkFile()
{
    if (!IsChunkFileEnable)
        return false;

    json js_chunkfile;
    json js_chunkmap = json::array();
    for (auto &chunkinfo : chunk_map)
    {
        json js_info;
        js_info["index"] = chunkinfo.index;

        json js_range = json::array();
        js_range.emplace_back(chunkinfo.range_left);
        js_range.emplace_back(chunkinfo.range_right);
        js_info["range"] = js_range;
        js_chunkmap.emplace_back(js_info);
    }
    js_chunkfile["chunk_map"] = js_chunkmap;

    Buffer buf = js_chunkfile.dump();

    chunkfile_io.Seek(FileIOHandler::SeekOrigin::BEGIN);
    long writecount = chunkfile_io.Write(buf);
    if (writecount != buf.Length())
        return false;

    if (chunkfile_io.GetSize() > buf.Length())
        chunkfile_io.Truncate(buf.Length());

    return true;
}

bool FileTransferDownLoadTask::ParseChunkMap(const json &js)
{
    bool parseresult = true;
    if (js.contains("chunk_map") && js.at("chunk_map").is_array())
    {
        vector<FileTransferChunkInfo> chunkmap;
        json js_chunkmap = js.at("chunk_map");
        for (auto &js_chunk : js_chunkmap)
        {
            if (!js_chunk.is_object() ||
                !js_chunk.contains("index") ||
                !js_chunk.contains("range") ||
                !js_chunk.at("index").is_number() ||
                !js_chunk.at("range").is_array())
            {
                parseresult = false;
                break;
            }
            json js_range = js_chunk.at("range");
            if (!js_range.at(0).is_number() || !js_range.at(1).is_number())
            {
                parseresult = false;
                break;
            }

            int index = js_chunk["index"];
            uint64_t left = js_range.at(0);
            uint64_t right = js_range.at(1);

            FileTransferChunkInfo info(index, left, right);
            chunkmap.emplace_back(info);
        }
        if (parseresult)
            chunk_map = chunkmap;
    }
    return parseresult;
}

bool FileTransferDownLoadTask::ReadChunkFile()
{
    chunk_map.clear();
    string ChunkFilePath = getChunkFilePath(file_path);

    bool hasexist = FileIOHandler::Exists(ChunkFilePath);

    IsChunkFileEnable = chunkfile_io.Open(ChunkFilePath, FileIOHandler::OpenMode::READ_WRITE);
    if (hasexist && IsChunkFileEnable)
    {
        Buffer buf;
        chunkfile_io.Seek(FileIOHandler::SeekOrigin::BEGIN);
        chunkfile_io.Read(buf, chunkfile_io.GetSize());
        string js_str(buf.Byte(), buf.Length());
        json js;
        try
        {
            js = json::parse(js_str);
        }
        catch (...)
        {
            cout << fmt::format("json::parse error : {}\n", js_str);
        }

        ParseChunkMap(js);
    }
    return IsChunkFileEnable;
}

bool FileTransferDownLoadTask::ParseFile()
{
    ReleaseSource();
    IsFileEnable = file_io.Open(file_path, FileIOHandler::OpenMode::READ_WRITE);
    ReadChunkFile();
    return IsFileEnable;
}

bool FileTransferDownLoadTask::CheckFinish()
{
    return getUntransferredChunks(chunk_map, file_size).empty();
}

void FileTransferDownLoadTask::OccurError(BaseNetWorkSession *session)
{
    if (!IsError)
    {
        IsError = true;
        SendErrorInfo(session);
        OnError();
    }
}

void FileTransferDownLoadTask::OccurFinish()
{
    if (!IsFinished)
    {
        IsFinished = true;
        OnFinished();
        ReleaseSource();
    }
}

void FileTransferDownLoadTask::OccurInterrupt()
{
    if (!IsInterrupted && !IsFinished)
    {
        IsInterrupted = true;
        OnInterrupted();
        ReleaseSource();
    }
}

void FileTransferDownLoadTask::AckTransReq(BaseNetWorkSession *session, const json &js)
{
    if (js.contains("filesize") && js.at("filesize").is_number_unsigned() && js.contains("filename") && js.at("filename").is_string())
    {
        uint64_t filesize = js.at("filesize");
        string filename = js.at("filename");

        file_size = filesize;
        file_path = "/home/h/ChatServer/MainServer/bin/downloadtest/" + filename;
    }
    else
    {
        OccurError(session);
        return;
    }

    ParseFile();

    json js_reply;
    js_reply["command"] = 8000;
    js_reply["taskid"] = task_id;
    js_reply["result"] = IsFileEnable;
    js_reply["filesize"] = file_size;
    js_reply["result"] = IsFileEnable == true ? 1 : 0;
    js_reply["suggest_chunsize"] = std::max((uint64_t)1, file_size / (uint64_t)10);

    json js_chunkmap = json::array();
    for (auto &chunkinfo : chunk_map)
    {
        json js_info;
        js_info["index"] = chunkinfo.index;

        json js_range = json::array();
        js_range.emplace_back(chunkinfo.range_left);
        js_range.emplace_back(chunkinfo.range_right);
        js_info["range"] = js_range;
        js_chunkmap.emplace_back(js_info);
    }
    displayTransferProgress(file_size, chunk_map);

    js_reply["chunk_map"] = js_chunkmap;

    Buffer buf = js_reply.dump();
    if (!session->AsyncSend(buf))
        OccurError(session);
}

void FileTransferDownLoadTask::RecvChunkDataAndAck(BaseNetWorkSession *session, const json &js)
{
    bool error = false;
    if (!js.contains("chunk_size") || !js.at("chunk_size").is_number_unsigned())
        error = true;
    if (!js.contains("data") || !js.at("data").is_object())
        error = true;
    if (!js.contains("range") || !js.at("range").is_array())
    {
        if (!js.at("range").at(0).is_number_unsigned() || js.at("range").at(1).is_number_unsigned())
            error = true;
    }

    if (error)
    {
        OccurError(session);
        return;
    }

    FileTransferChunkData chunkdata;
    uint64_t chunksize = 0;

    chunksize = js["chunk_size"];
    chunkdata.range_left = js.at("range").at(0);
    chunkdata.range_right = js.at("range").at(1);

    vector<uint8_t> bytes;
    error = !ExtractBinaryJson(js["data"], bytes);
    if (error)
    {
        OccurError(session);
        return;
    }
    chunkdata.FromBinary(bytes);

    if (chunkdata.buf.Length() < chunksize)
    {
        error = true;
        OccurError(session);
        return;
    }

    if (IsFileEnable)
    {
        file_io.Seek(FileIOHandler::SeekOrigin::BEGIN, chunkdata.range_left);
        long writecount = file_io.Write(chunkdata.buf.Byte(), chunksize);
        if (writecount != chunksize)
            error = true;

        if (!error)
        {
            chunk_map.emplace_back(0, chunkdata.range_left, chunkdata.range_right);
            chunk_map = mergeChunks(chunk_map);
            displayTransferProgress(file_size, chunk_map);
        }
    }
    else
    {
        OccurError(session);
        return;
    }

    if (IsChunkFileEnable)
    {
        WriteToChunkFile();
    }

    json js_reply;
    bool finished = CheckFinish();
    if (finished)
    {
        file_io.Truncate(file_size);

        js_reply["command"] = 8010;
        js_reply["taskid"] = task_id;
        js_reply["result"] = 1;
    }
    else
    {
        js_reply["command"] = 8001;
        js_reply["taskid"] = task_id;
        js_reply["chunk_size"] = chunksize;
        json js_range = json::array();
        js_range.emplace_back(chunkdata.range_left);
        js_range.emplace_back(chunkdata.range_right);
        js_reply["range"] = js_range;
        js_reply["result"] = error == true ? 1 : 0;
        json js_chunkmap = json::array();
        for (auto &chunkinfo : chunk_map)
        {
            json js_info;
            js_info["index"] = chunkinfo.index;

            json js_range = json::array();
            js_range.emplace_back(chunkinfo.range_left);
            js_range.emplace_back(chunkinfo.range_right);
            js_info["range"] = js_range;
            js_chunkmap.emplace_back(js_info);
        }

        js_reply["chunk_map"] = js_chunkmap;
    }
    Buffer buf = js_reply.dump();
    if (!session->AsyncSend(buf))
    {
        OccurError(session);
        return;
    }
    if (finished)
        OccurFinish();
}

void FileTransferDownLoadTask::AckRecvFinished(BaseNetWorkSession *session, const json &js)
{
    if (CheckFinish())
    {
        file_io.Truncate(file_size);
        OccurFinish();
    }

    if (IsFinished)
    {
        json js_reply;
        js_reply["command"] = 8010;
        js_reply["taskid"] = task_id;
        js_reply["result"] = IsFinished ? 1 : 0;

        Buffer buf = js_reply.dump();
        session->AsyncSend(buf);
    }
    else
    {
        OccurError(session);
    }
}

void FileTransferDownLoadTask::SendErrorInfo(BaseNetWorkSession *session)
{
    json js_error;
    js_error["command"] = 7080;
    js_error["taskid"] = task_id;
    js_error["result"] = 0;

    Buffer buf = js_error.dump();
    session->AsyncSend(buf);
}

void FileTransferDownLoadTask::RecvPeerError(const json &js)
{
    if (!IsError)
    {
        IsError = true;
        OnError();
    }
}

void FileTransferDownLoadTask::RecvPeerInterrupt(const json &js)
{
    if (!IsFinished)
        OccurInterrupt();
}

void FileTransferDownLoadTask::OnError()
{
    try
    {
        FileIOHandler::Remove(chunkfile_io.FilePath());
        ReleaseSource();
        if (_callbackError)
            _callbackError(this);
    }
    catch (const std::exception &e)
    {
    }
}

void FileTransferDownLoadTask::OnFinished()
{
    try
    {
        FileIOHandler::Remove(chunkfile_io.FilePath());
        ReleaseSource();
        if (_callbackFinieshed)
            _callbackFinieshed(this);
    }
    catch (const std::exception &e)
    {
    }
}

void FileTransferDownLoadTask::OnInterrupted()
{
    try
    {
        ReleaseSource();
        if (_callbackInterrupted)
            _callbackInterrupted(this);
    }
    catch (const std::exception &e)
    {
    }
}

void FileTransferDownLoadTask::ProcessMsg(BaseNetWorkSession *session, const Buffer &buf)
{
    string str(buf.Byte(), buf.Length());
    json js;
    try
    {
        js = json::parse(str);
    }
    catch (...)
    {
        cout << fmt::format("json::parse error : {}\n", str);
    }
    if (js.contains("taskid"))
    {
        string taskid = js.at("taskid");
        if (taskid != task_id)
            return;
    }

    if (js.contains("command"))
    {
        int command = js.at("command");
        if (command != 7000 && command != 7001 && command != 7010 && command != 7080 && command != 7070)
            return;

        InterruptedLock.Enter();

        if (IsFinished)
        {
            json js_success;
            js_success["command"] = 7010;
            js_success["taskid"] = task_id;
            js_success["result"] = 1;

            Buffer buf_data = js_success.dump();
            if (!session->AsyncSend(buf_data))
                OccurError(session);
            return;
        }

        if (IsInterrupted)
        {
            return;
        }

        if (IsError)
        {
            return SendErrorInfo(session);
        }
        if (command == 7080)
        {
            RecvPeerError(js);
        }
        if (command == 7000)
        {
            AckTransReq(session, js);
        }
        if (command == 7001)
        {
            RecvChunkDataAndAck(session, js);
        }
        if (command == 7010)
        {
            AckRecvFinished(session, js);
        }
        if (command == 7070)
        {
            RecvPeerInterrupt(js);
        }

        InterruptedLock.Leave();
    }
}

void FileTransferDownLoadTask::BindErrorCallBack(std::function<void(FileTransferDownLoadTask *)> callback)
{
    _callbackError = callback;
}

void FileTransferDownLoadTask::BindFinishedCallBack(std::function<void(FileTransferDownLoadTask *)> callback)
{
    _callbackFinieshed = callback;
}

void FileTransferDownLoadTask::BindInterruptedCallBack(std::function<void(FileTransferDownLoadTask *)> callback)
{
    _callbackInterrupted = callback;
}
