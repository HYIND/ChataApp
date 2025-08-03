#include "FileTransferUploadTask.h"

std::string getFilenameFromPath(const std::string &filepath)
{
    // 处理 Windows 和 Unix 风格的路径分隔符
    size_t pos1 = filepath.find_last_of('/');
    size_t pos2 = filepath.find_last_of('\\');

    size_t pos = (pos1 == std::string::npos) ? pos2 : ((pos2 == std::string::npos) ? pos1 : std::max(pos1, pos2));

    if (pos != std::string::npos)
    {
        return filepath.substr(pos + 1);
    }
    return filepath;
}

FileTransferUploadTask::FileTransferUploadTask(const string &filepath, const string &taskid)
    : FileTransferTask(filepath, taskid)
{
    ParseFile();
}
FileTransferUploadTask::~FileTransferUploadTask()
{
}

void FileTransferUploadTask::ReleaseSource()
{
    file_io.Close();
    file_size = 0;
    chunk_map.clear();
    // file_path.clear();
    // task_id.clear();
}

void FileTransferUploadTask::InterruptTrans(BaseNetWorkSession *session)
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

bool FileTransferUploadTask::ParseFile()
{
    ReleaseSource();
    IsFileEnable = file_io.Open(file_path, FileIOHandler::OpenMode::READ_ONLY);
    if (IsFileEnable)
    {
        file_size = file_io.GetSize();
        suggest_chunksize = std::max((uint64_t)1, file_size / (uint64_t)20);
    }
    else
    {
        file_size = 0;
    }
    return IsFileEnable;
}

void FileTransferUploadTask::SendTransReq(BaseNetWorkSession *session)
{
    if (!IsFileEnable)
        return;

    json js;
    js["command"] = 7000;
    js["taskid"] = task_id;
    js["filename"] = getFilenameFromPath(file_path);
    js["filesize"] = file_size;

    Buffer buf = js.dump();
    session->AsyncSend(buf);
}

void FileTransferUploadTask::AckTransReqResult(BaseNetWorkSession *session, const json &js)
{
    if (!ParseSuggestChunkSize(js))
    {
        OccurError(session);
        return;
    }
    if (!ParseChunkMap(js))
    {
        OccurError(session);
        return;
    }
    SendNextChunkData(session);
}

void FileTransferUploadTask::RecvChunkMapAndSendNextData(BaseNetWorkSession *session, const json &js)
{
    if (!ParseChunkMap(js))
    {
        OccurError(session);
        return;
    }
    SendNextChunkData(session);
}

bool FileTransferUploadTask::ParseSuggestChunkSize(const json &js)
{
    if (js.contains("suggest_chunsize") && js.at("suggest_chunsize").is_number_unsigned())
    {
        suggest_chunksize = js["suggest_chunsize"];
        return true;
    }
    else
        return false;
}

bool FileTransferUploadTask::ParseChunkMap(const json &js)
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

void FileTransferUploadTask::OccurError(BaseNetWorkSession *session)
{
    if (!IsError)
    {
        IsError = true;
        OnError();
        SendErrorInfo(session);
    }
}

void FileTransferUploadTask::OccurFinish()
{
    if (!IsFinished)
    {
        IsFinished = true;
        OnFinished();
        ReleaseSource();
    }
}

void FileTransferUploadTask::OccurInterrupt()
{
    if (!IsInterrupted && !IsFinished)
    {
        IsInterrupted = true;
        OnInterrupted();
        ReleaseSource();
    }
}

void FileTransferUploadTask::SendNextChunkData(BaseNetWorkSession *session)
{
    vector<FileTransferChunkInfo> untrans_chunks = getUntransferredChunks(chunk_map, file_size);
    if (untrans_chunks.empty()) // 发送完毕
    {
        json js_success;
        js_success["command"] = 7010;
        js_success["taskid"] = task_id;
        js_success["result"] = 1;

        Buffer buf_data = js_success.dump();
        if (!session->AsyncSend(buf_data))
        {
            OccurError(session);
            return;
        }
    }
    else
    {
        FileTransferChunkInfo &chunkinfo = untrans_chunks[0];
        uint64_t size = chunkinfo.range_right - chunkinfo.range_left + 1;

        uint64_t nextchunksize = std::min(suggest_chunksize, size);

        FileTransferChunkData chunkdata;
        chunkdata.range_left = chunkinfo.range_left;
        chunkdata.range_right = chunkinfo.range_left + nextchunksize - 1;

        file_io.Seek(FileIOHandler::SeekOrigin::BEGIN, chunkdata.range_left);
        file_io.Read(chunkdata.buf, nextchunksize);

        json js_data;
        js_data["command"] = 7001;
        js_data["taskid"] = task_id;
        js_data["chunk_size"] = nextchunksize;
        json js_range = json::array();
        js_range.emplace_back(chunkdata.range_left);
        js_range.emplace_back(chunkdata.range_right);
        js_data["range"] = js_range;

        js_data["data"] = json::binary(chunkdata.ToBinary());

        Buffer buf_data = js_data.dump();
        if (!session->AsyncSend(buf_data))
        {
            OccurError(session);
            return;
        }
    }
}

void FileTransferUploadTask::SendErrorInfo(BaseNetWorkSession *session)
{
    json js_error;
    js_error["command"] = 7080;
    js_error["taskid"] = task_id;
    js_error["result"] = 0;

    Buffer buf = js_error.dump();
    session->AsyncSend(buf);
}

void FileTransferUploadTask::RecvPeerError(const json &js)
{
    if (!IsError)
    {
        IsError = true;
        OnError();
    }
}

void FileTransferUploadTask::RecvPeerFinish(const json &js)
{
    OccurFinish();
}

void FileTransferUploadTask::RecvPeerInterrupt(const json &js)
{
    if (!IsFinished)
        OccurInterrupt();
}

void FileTransferUploadTask::OnError()
{
    try
    {
        ReleaseSource();
        if (_callbackError)
            _callbackError(this);
    }
    catch (const std::exception &e)
    {
    }
}

void FileTransferUploadTask::OnFinished()
{
    try
    {
        ReleaseSource();
        if (_callbackFinieshed)
            _callbackFinieshed(this);
    }
    catch (const std::exception &e)
    {
    }
}

void FileTransferUploadTask::OnInterrupted()
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

bool FileTransferUploadTask::StartSendFile(BaseNetWorkSession *session)
{
    if (IsFileEnable)
        SendTransReq(session);

    return IsFileEnable;
}

void FileTransferUploadTask::ProcessMsg(BaseNetWorkSession *session, const Buffer &buf)
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
        if (command != 8000 && command != 8001 && command != 8010 && command != 7080 && command != 7070)
            return;

        InterruptedLock.Enter();

        if (IsFinished)
        {
            json js_reply;
            js_reply["command"] = 8010;
            js_reply["taskid"] = task_id;
            js_reply["result"] = IsFinished ? 1 : 0;

            Buffer buf = js_reply.dump();
            session->AsyncSend(buf);
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
        if (command == 8000)
        {
            AckTransReqResult(session, js);
        }
        if (command == 8001)
        {
            RecvChunkMapAndSendNextData(session, js);
        }
        if (command == 8010)
        {
            RecvPeerFinish(js);
        }
        if (command == 7070)
        {
            RecvPeerInterrupt(js);
        }

        InterruptedLock.Leave();
    }
}

void FileTransferUploadTask::BindErrorCallBack(std::function<void(FileTransferUploadTask *)> callback)
{
    _callbackError = callback;
}

void FileTransferUploadTask::BindFinishedCallBack(std::function<void(FileTransferUploadTask *)> callback)
{
    _callbackFinieshed = callback;
}

void FileTransferUploadTask::BindInterruptedCallBack(std::function<void(FileTransferUploadTask *)> callback)
{
    _callbackInterrupted = callback;
}
