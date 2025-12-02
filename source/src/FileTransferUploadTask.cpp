#include "FileTransferUploadTask.h"
#include "NetWorkHelper.h"

FileTransferUploadTask::FileTransferUploadTask(const string &taskid, const string &filepath)
    : FileTransferTask(taskid, filepath)
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

        if (!NetWorkHelper::SendMessagePackage(session, &js_error))
            IsNetworkEnable = false;

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
        suggest_chunksize = GetSuggestChunsize(file_size);
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

    if (!NetWorkHelper::SendMessagePackage(session, &js))
        IsNetworkEnable = false;
}

void FileTransferUploadTask::AckTransReqResult(BaseNetWorkSession *session, const json &js)
{
    if (!ParseReqResult(js))
    {
        OccurError(session);
        return;
    }

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
    OccurProgressChange();
    SendNextChunkData(session);
}

void FileTransferUploadTask::RecvChunkMapAndSendNextData(BaseNetWorkSession *session, const json &js)
{
    if (!ParseChunkMap(js))
    {
        OccurError(session);
        return;
    }
    OccurProgressChange();
    SendNextChunkData(session);
}

bool FileTransferUploadTask::ParseReqResult(const json &js)
{
    if (js.contains("result") && js.at("result").is_number_unsigned())
    {
        uint32_t result = js["result"];
        return result != 0;
    }
    return false;
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
        {
            chunk_map = mergeChunks(chunkmap);
        }
    }
    return parseresult;
}

void FileTransferUploadTask::OccurError(BaseNetWorkSession *session)
{
    if (!IsError)
    {
        IsError = true;
        if (IsNetworkEnable)
            SendErrorInfo(session);
        OnError();
    }
}

void FileTransferUploadTask::OccurFinish()
{
    if (!IsFinished)
    {
        OccurProgressChange();
        IsFinished = true;
        OnFinished();
    }
}

void FileTransferUploadTask::OccurInterrupt()
{
    if (!IsInterrupted && !IsFinished)
    {
        IsInterrupted = true;
        OnInterrupted();
    }
}

void FileTransferUploadTask::OccurProgressChange()
{
    uint32_t progress = Progress();
    OnProgress(progress);
}

uint32_t FileTransferUploadTask::Progress()
{
    return CountProgress(chunk_map, file_size);
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

        if (!NetWorkHelper::SendMessagePackage(session, &js_success))
        {
            IsNetworkEnable = false;
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

        // js_data["data"] = json::binary(chunkdata.ToBinary());

        if (!NetWorkHelper::SendMessagePackage(session, &js_data, &chunkdata.buf))
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

    try
    {
        if (!NetWorkHelper::SendMessagePackage(session, &js_error))
            IsNetworkEnable = false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "SendErrorInfo error!" << e.what() << '\n';
    }
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

void FileTransferUploadTask::OnProgress(uint32_t progress)
{
    try
    {
        if (_callbackProgress)
            _callbackProgress(this, progress);
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

void FileTransferUploadTask::ProcessMsg(BaseNetWorkSession *session, const json &js, Buffer &buf)
{
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

            if (!NetWorkHelper::SendMessagePackage(session, &js_reply))
                IsNetworkEnable = false;
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

void FileTransferUploadTask::BindProgressCallBack(std::function<void(FileTransferUploadTask *, uint32_t)> callback)
{
    _callbackProgress = callback;
}
