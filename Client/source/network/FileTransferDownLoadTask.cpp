#include "FileTransferDownLoadTask.h"
#include "ConnectManager.h"
#include "NetWorkHelper.h"
#include "MD5Helper.h"

void displayTransferProgress(uint64_t totalSize, const std::vector<FileTransferChunkInfo>& transferredChunks, int barWidth = 160)
{
	// 合并重叠或连续的 chunk
	std::vector<FileTransferChunkInfo> merged;
	if (!transferredChunks.empty())
	{
		auto sorted = transferredChunks;
		std::sort(sorted.begin(), sorted.end(), [](const FileTransferChunkInfo& a, const FileTransferChunkInfo& b)
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
	for (const auto& chunk : merged)
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
		for (const auto& chunk : merged)
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

    bool flushenable = true;
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

QString getChunkFilePath(const QString& filepath)
{
	return filepath + "__chunks";
}

QString getMD5CheckPointFilePath(const QString& filepath)
{
    return filepath + "__check";
}

MD5CheckPoint::MD5CheckPoint()
{
    progress = 0;
}

FileTransferDownLoadTask::FileTransferDownLoadTask(const QString& taskid, const QString& filepath, const QString& md5)
    : FileTransferTask(taskid, filepath, md5)
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

void FileTransferDownLoadTask::InterruptTrans()
{
	InterruptedLock.Enter();

	if (!IsFinished)
	{
		json js_error;
		js_error["command"] = 7070;
		js_error["taskid"] = task_id.toStdString();
		js_error["result"] = 0;

		NetWorkHelper::SendMessagePackage(&js_error);

		OccurInterrupt();
	}

	InterruptedLock.Leave();
}


void FileTransferDownLoadTask::WriteToMD5CheckFile()
{
    uint32_t curprogress = Progress();
    uint32_t lastcheckprogress = _MD5CheckPoint.progress;
    if (curprogress - lastcheckprogress < 20)
        return;
    else
    {
        json js_checkfile;
        {
            json js_md5checkpoint;
            {
                json js_md5state;
                std::shared_ptr<MD5SnapShot> shot = std::make_shared<MD5SnapShot>(_asyncmd5.SnapShot());
                js_md5state["status0"] = shot->status[0];
                js_md5state["status1"] = shot->status[1];
                js_md5state["status2"] = shot->status[2];
                js_md5state["status3"] = shot->status[3];
                js_md5state["count"] = shot->count;

                {
                    json cache;
                    {
                        json data_array = json::array();

                        uint8_t *ptr = (uint8_t *)(shot->cacheBuffer.Data());
                        uint64_t length = shot->cacheBuffer.Length();
                        for (uint64_t i = 0; i < length; i++)
                            data_array.push_back(ptr[i]);

                        cache["data"] = data_array;
                        cache["size"] = length;
                    }
                    js_md5state["cache"] = cache;
                }

                js_md5state["isfinish"] = shot->isfinish;
                js_md5state["md5string"] = shot->md5string;

                _MD5CheckPoint.progress = curprogress;
                _MD5CheckPoint.snap = shot;

                js_md5checkpoint["md5state"] = js_md5state;
                js_md5checkpoint["checkprogress"] = curprogress;
            }
            js_checkfile["md5checkpoint"] = js_md5checkpoint;
        }

        Buffer buf(js_checkfile.dump());

        QString CheckPointFilePath = getMD5CheckPointFilePath(file_path);
        FileIOHandler md5checkfile_io(CheckPointFilePath, FileIOHandler::OpenMode::WRITE_ONLY);

        md5checkfile_io.Seek(0);
        long writecount = md5checkfile_io.Write(buf);
        if (writecount != buf.Length())
        {
            FileIOHandler::Remove(CheckPointFilePath);
            return;
        }

        if (md5checkfile_io.GetSize() > buf.Length())
            md5checkfile_io.Truncate(buf.Length());

        return;
    }
}

bool FileTransferDownLoadTask::WriteToChunkFile()
{
	if (!IsChunkFileEnable)
		return false;

	json js_chunkfile;
	json js_chunkmap = json::array();
	for (auto& chunkinfo : chunk_map)
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

	Buffer buf(js_chunkfile.dump());

	chunkfile_io.Seek();
	long writecount = chunkfile_io.Write(buf);
	if (writecount != buf.Length())
		return false;

	if (chunkfile_io.GetSize() > buf.Length())
		chunkfile_io.Truncate(buf.Length());

	return true;
}

bool FileTransferDownLoadTask::ParseChunkMap(const json& js)
{
	bool parseresult = true;
	if (js.contains("chunk_map") && js.at("chunk_map").is_array())
	{
        std::vector<FileTransferChunkInfo> chunkmap;
		json js_chunkmap = js.at("chunk_map");
		for (auto& js_chunk : js_chunkmap)
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
    else
        parseresult = false;

	return parseresult;
}

bool FileTransferDownLoadTask::ParseMd5CheckPoint(const json &js)
{
    bool success = true;
    if (js.contains("md5checkpoint") && js.at("md5checkpoint").is_object())
    {
        json js_md5checkpoint = js["md5checkpoint"];
        if (js_md5checkpoint.contains("md5state") && js_md5checkpoint.at("md5state").is_object() &&
            js_md5checkpoint.contains("checkprogress") && js_md5checkpoint.at("checkprogress").is_number_unsigned())
        {
            std::shared_ptr<MD5SnapShot> shot = std::make_shared<MD5SnapShot>();
            auto js_md5state = js_md5checkpoint["md5state"];
            uint32_t progress = js_md5checkpoint["checkprogress"];

            static std::vector<std::string> status_list{"status0", "status1", "status2", "status3"};

            if (success)
            {
                for (int i = 0; i < status_list.size(); i++)
                {
                    if (js_md5state.contains(status_list[i]) && js_md5state.at(status_list[i]).is_number_unsigned())
                        shot->status[i] = js_md5state[status_list[i]];
                    else
                    {
                        success = false;
                        break;
                    }
                }
            }
            if (success)
            {
                if (js_md5state.contains("count") && js_md5state.at("count").is_number_unsigned())
                    shot->count = js_md5state["count"];
                else
                    success = false;
            }
            if (success)
            {
                if (js_md5state.contains("cache") && js_md5state.at("cache").is_object())
                {
                    json js_cache = js_md5state["cache"];
                    if (js_cache.contains("data") && js_cache.at("data").is_array() && js_cache.contains("size") && js_cache.at("size").is_number_unsigned())
                    {
                        json js_data = js_cache["data"];
                        uint64_t size = js_cache["size"];
                        std::vector<uint8_t> data = js_data.get<std::vector<uint8_t>>();
                        if (size == data.size())
                            shot->cacheBuffer.Write(data.data(), data.size());
                        else
                            success = false;
                    }
                    else
                        success = false;
                }
                else
                    success = false;
            }
            if (success)
            {
                if (js_md5state.contains("isfinish") && js_md5state.at("isfinish").is_boolean())
                    shot->isfinish = js_md5state["isfinish"];
                else
                    success = false;
            }
            if (success)
            {
                _asyncmd5.LoadSnapShot(*shot);
                _MD5CheckPoint.progress = progress;
                _MD5CheckPoint.snap = shot;
            }
        }
        else
            success = false;
    }
    else
        success = false;

    return success;
}

bool FileTransferDownLoadTask::ReadChunkFile()
{
	chunk_map.clear();
	QString ChunkFilePath = getChunkFilePath(file_path);

    bool exist = FileIOHandler::Exists(ChunkFilePath);

	IsChunkFileEnable = chunkfile_io.Open(ChunkFilePath, FileIOHandler::OpenMode::READ_WRITE);
    if (exist && IsChunkFileEnable)
	{
		Buffer buf;
		chunkfile_io.Seek();
		chunkfile_io.Read(buf, chunkfile_io.GetSize());
        std::string js_str(buf.Byte(), buf.Length());
		json js;
		try
		{
			js = json::parse(js_str);
		}
		catch (...)
		{
		}

		ParseChunkMap(js);
	}
	return IsChunkFileEnable;
}


bool FileTransferDownLoadTask::ReadMD5ChcekPointFile()
{
    QString CheckPointFilePath = getMD5CheckPointFilePath(file_path);

    bool exist = FileIOHandler::Exists(CheckPointFilePath);
    if (!exist)
        return false;

    FileIOHandler md5checkfile_io(CheckPointFilePath, FileIOHandler::OpenMode::READ_ONLY);
    bool isopen = md5checkfile_io.IsOpen();
    if (isopen)
    {
        Buffer buf;
        md5checkfile_io.Seek(0);
        md5checkfile_io.Read(buf, md5checkfile_io.GetSize());
        md5checkfile_io.Close();
        std::string js_str(buf.Byte(), buf.Length());
        json js;
        try
        {
            js = json::parse(js_str);
        }
        catch (...)
        {
        }
        ParseMd5CheckPoint(js);
    }
    return isopen;
}

bool FileTransferDownLoadTask::ParseFile()
{
	ReleaseSource();
	IsFileEnable = file_io.Open(file_path, FileIOHandler::OpenMode::READ_WRITE);
	ReadChunkFile();
    ReadMD5ChcekPointFile();
    return IsFileEnable && IsChunkFileEnable;
}

bool FileTransferDownLoadTask::CheckTransFinish()
{
	return getUntransferredChunks(chunk_map, file_size).empty();
}

void FileTransferDownLoadTask::AsyncMD5Update()
{
    static constexpr uint64_t max_read_block_size = 5 * 1024 * 1024;
    uint64_t hasuploadsize = _asyncmd5.Count();
    for (auto &chunk : chunk_map)
    {
        if (hasuploadsize >= chunk.range_left && hasuploadsize <= chunk.range_right)
        {
            uint64_t readpostion = hasuploadsize;
            uint64_t readsize = min((chunk.range_right + 1) - readpostion, max_read_block_size);
            Buffer buf;
            file_io.Seek(readpostion);
            file_io.Read(buf, readsize);
            _asyncmd5.Update(buf);
            break;
        }
    }
}

std::string FileTransferDownLoadTask::AsyncMD5Final()
{
    static constexpr uint64_t max_read_block_size = 5 * 1024 * 1024;
    uint64_t totalsize = file_io.GetSize();
    uint64_t hasuploadsize = _asyncmd5.Count();
    if (totalsize > hasuploadsize)
    {
        uint64_t remainsize = totalsize - hasuploadsize;
        while (remainsize > 0)
        {
            uint64_t readpostion = totalsize - remainsize;
            uint64_t readsize = min(remainsize, (uint64_t)max_read_block_size);
            Buffer buf;
            file_io.Seek(readpostion);
            file_io.Read(buf, readsize);
            _asyncmd5.UpdateSync(buf);
            remainsize -= readsize;
        }
    }
    return _asyncmd5.Final();
}

void FileTransferDownLoadTask::OccurError()
{
	if (!IsError)
	{
		IsError = true;
		SendErrorInfo();
		OnError();
	}
}

void FileTransferDownLoadTask::OccurFinish()
{
	if (!IsFinished)
	{
		OccurProgressChange();
		IsFinished = true;
		OnFinished();
	}
}

void FileTransferDownLoadTask::OccurInterrupt()
{
	if (!IsInterrupted && !IsFinished)
	{
		IsInterrupted = true;
		OnInterrupted();
	}
}

void FileTransferDownLoadTask::OccurProgressChange()
{
	uint32_t progress = Progress();
	OnProgress(progress);
}

uint32_t FileTransferDownLoadTask::Progress()
{
	return CountProgress(chunk_map, file_size);
}

void FileTransferDownLoadTask::RecvTransReq(const json& js)
{
	if (js.contains("filesize") && js.at("filesize").is_number_unsigned() && js.contains("filename") && js.at("filename").is_string())
	{
		uint64_t filesize = js.at("filesize");
		QString filename = QString::fromStdString(js.at("filename"));

		file_size = filesize;
		file_path = file_path + filename;
	}
	else
	{
		OccurError();
		return;
	}

	ParseFile();

	json js_reply;
	js_reply["command"] = 8000;
	js_reply["taskid"] = task_id.toStdString();
	js_reply["result"] = IsFileEnable;
	js_reply["filesize"] = file_size;
	js_reply["result"] = IsFileEnable == true ? 1 : 0;
	js_reply["suggest_chunsize"] = GetSuggestChunsize(file_size);

	json js_chunkmap = json::array();
	for (auto& chunkinfo : chunk_map)
	{
		json js_info;
		js_info["index"] = chunkinfo.index;

		json js_range = json::array();
		js_range.emplace_back(chunkinfo.range_left);
		js_range.emplace_back(chunkinfo.range_right);
		js_info["range"] = js_range;
		js_chunkmap.emplace_back(js_info);
	}
	// displayTransferProgress(file_size, chunk_map);

	js_reply["chunk_map"] = js_chunkmap;

	if (!NetWorkHelper::SendMessagePackage(&js_reply))
		OccurError();

	OccurProgressChange();
}

void FileTransferDownLoadTask::AckTransReq(const json& js)
{
	bool ackresult = false;
	if (js.contains("filesize") && js.at("filesize").is_number_unsigned() && js.contains("filename") && js.at("filename").is_string())
	{
		uint64_t filesize = js.at("filesize");
        std::string filename = js.at("filename");

		ackresult = ((file_size == filesize) /* && (getFilenameFromPath(file_path) == filename) */);
	}
	else
	{
		OccurError();
		return;
	}

	if (ackresult)
		ParseFile();

	json js_reply;
	js_reply["command"] = 8000;
	js_reply["taskid"] = task_id.toStdString();
	js_reply["filesize"] = file_size;
	js_reply["result"] = (ackresult && IsFileEnable) == true ? 1 : 0;
	js_reply["suggest_chunsize"] = GetSuggestChunsize(file_size);

	if (ackresult)
	{
		json js_chunkmap = json::array();
		for (auto& chunkinfo : chunk_map)
		{
			json js_info;
			js_info["index"] = chunkinfo.index;

			json js_range = json::array();
			js_range.emplace_back(chunkinfo.range_left);
			js_range.emplace_back(chunkinfo.range_right);
			js_info["range"] = js_range;
			js_chunkmap.emplace_back(js_info);
		}
		// displayTransferProgress(file_size, chunk_map);

		js_reply["chunk_map"] = js_chunkmap;
	}

	if (!NetWorkHelper::SendMessagePackage(&js_reply))
		OccurError();

	if (ackresult)
		OccurProgressChange();
}

void FileTransferDownLoadTask::RecvChunkDataAndAck(const json& js, Buffer& buf)
{
	bool error = false;
	if (!js.contains("chunk_size") || !js.at("chunk_size").is_number_unsigned())
		error = true;
	//if (!js.contains("data") || !js.at("data").is_object())
	//	error = true;
	if (!js.contains("range") || !js.at("range").is_array())
	{
		if (!js.at("range").at(0).is_number_unsigned() || js.at("range").at(1).is_number_unsigned())
			error = true;
	}

	if (error)
	{
		OccurError();
		return;
	}

	FileTransferChunkData chunkdata;
	uint64_t chunksize = 0;

	chunksize = js["chunk_size"];
	chunkdata.range_left = js.at("range").at(0);
	chunkdata.range_right = js.at("range").at(1);

    if (buf.Remain() < chunksize)
	{
		error = true;
		OccurError();
		return;
	}

	chunkdata.buf.Append(buf, chunksize);

	if (IsFileEnable)
	{
		file_io.Seek(chunkdata.range_left);
        long truthwritecount = file_io.Write(chunkdata.buf.Byte(), chunksize);
        if (truthwritecount < 0)
        {
            error = true;
            OccurError();
            return;
        }
        else
        {
            if (truthwritecount > chunksize)
            {
                error = true;
                OccurError();
            }
        }

		if (!error)
		{
            chunk_map.emplace_back(0, chunkdata.range_left, chunkdata.range_right);
            chunk_map = mergeChunks(chunk_map);
			// displayTransferProgress(file_size, chunk_map);
            AsyncMD5Update();
        }
	}
	else
	{
		OccurError();
		return;
	}

	if (IsChunkFileEnable)
		WriteToChunkFile();
    WriteToMD5CheckFile();

	json js_reply;
    bool finished = CheckTransFinish();
	if (finished)
	{
		file_io.Truncate(file_size);
        if (_md5 != QString::fromStdString(AsyncMD5Final()))
        {
            OccurError();
            return;
        }
        else
        {
            js_reply["command"] = 8010;
            js_reply["taskid"] = task_id.toStdString();
            js_reply["result"] = 1;
        }
	}
	else
	{
		js_reply["command"] = 8001;
		js_reply["taskid"] = task_id.toStdString();
		js_reply["chunk_size"] = chunksize;
		json js_range = json::array();
		js_range.emplace_back(chunkdata.range_left);
		js_range.emplace_back(chunkdata.range_right);
		js_reply["range"] = js_range;
		js_reply["result"] = error == true ? 1 : 0;
		json js_chunkmap = json::array();
		for (auto& chunkinfo : chunk_map)
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
	if (!NetWorkHelper::SendMessagePackage(&js_reply))
	{
		OccurError();
		return;
	}
	if (finished)
		OccurFinish();
	else
	{
		OccurProgressChange();
	}
}

void FileTransferDownLoadTask::AckRecvFinished(const json& js)
{
    IsFinished = CheckTransFinish();
    if (IsFinished)
	{
        if (!IsFileEnable)
            IsFileEnable = file_io.Open(file_path, FileIOHandler::OpenMode::READ_WRITE);
        if (IsFileEnable)
            file_io.Truncate(file_size);
	}

	if (IsFinished)
	{
        if (_md5 != QString::fromStdString(AsyncMD5Final()))
        {
            OccurError();
            return;
        }
        else
        {
            json js_reply;
            js_reply["command"] = 8010;
            js_reply["taskid"] = task_id.toStdString();
            js_reply["result"] = IsFinished ? 1 : 0;

            NetWorkHelper::SendMessagePackage(&js_reply);
        }
	}

    if (IsFinished)
        OccurFinish();
	else
		OccurError();
}

void FileTransferDownLoadTask::SendErrorInfo()
{
	json js_error;
	js_error["command"] = 7080;
	js_error["taskid"] = task_id.toStdString();
	js_error["result"] = 0;

	NetWorkHelper::SendMessagePackage(&js_error);
}

void FileTransferDownLoadTask::RecvPeerError(const json& js)
{
	if (!IsError)
	{
		IsError = true;
		OnError();
	}
}

void FileTransferDownLoadTask::RecvPeerInterrupt(const json& js)
{
	if (!IsFinished)
		OccurInterrupt();
}

void FileTransferDownLoadTask::OnError()
{
	try
	{
        auto file_path = file_io.FilePath();
        auto chunkfile_path = chunkfile_io.FilePath();
        auto checkpoint_path = getMD5CheckPointFilePath(file_path);
        ReleaseSource();
        if (FileIOHandler::Exists(file_path))
            FileIOHandler::Remove(file_path);
        if (FileIOHandler::Exists(chunkfile_path))
            FileIOHandler::Remove(chunkfile_path);
        if (FileIOHandler::Exists(checkpoint_path))
            FileIOHandler::Remove(checkpoint_path);
		if (_callbackError)
			_callbackError(this);
	}
	catch (const std::exception& e)
	{
	}
}

void FileTransferDownLoadTask::OnFinished()
{
	try
	{
        auto chunkfile_path = chunkfile_io.FilePath();
        auto checkpoint_path = getMD5CheckPointFilePath(file_path);
        ReleaseSource();
        if (FileIOHandler::Exists(chunkfile_path))
            FileIOHandler::Remove(chunkfile_path);
        if (FileIOHandler::Exists(checkpoint_path))
            FileIOHandler::Remove(checkpoint_path);
		if (_callbackFinieshed)
			_callbackFinieshed(this);
	}
	catch (const std::exception& e)
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
	catch (const std::exception& e)
	{
	}
}

void FileTransferDownLoadTask::OnProgress(uint32_t progress)
{
	try
	{
		if (_callbackProgress)
			_callbackProgress(this, progress);
	}
	catch (const std::exception& e)
	{
	}
}

void FileTransferDownLoadTask::ProcessMsg(const json& js, Buffer& buf)
{
	if (js.contains("taskid"))
	{
        std::string taskid = js.at("taskid");
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
			js_success["taskid"] = task_id.toStdString();
			js_success["result"] = 1;

			if (!NetWorkHelper::SendMessagePackage(&js_success))
				OccurError();
			return;
		}

		if (IsInterrupted)
		{
			return;
		}

		if (IsError)
		{
			return SendErrorInfo();
		}
		if (command == 7080)
		{
			RecvPeerError(js);
		}
		if (command == 7000)
		{
			if (IsRegister)
				AckTransReq(js);
			else
				RecvTransReq(js);
		}
		if (command == 7001)
		{
			RecvChunkDataAndAck(js, buf);
		}
		if (command == 7010)
		{
			AckRecvFinished(js);
		}
		if (command == 7070)
		{
			RecvPeerInterrupt(js);
		}

		InterruptedLock.Leave();
	}
}

void FileTransferDownLoadTask::BindErrorCallBack(std::function<void(FileTransferDownLoadTask*)> callback)
{
	_callbackError = callback;
}

void FileTransferDownLoadTask::BindFinishedCallBack(std::function<void(FileTransferDownLoadTask*)> callback)
{
	_callbackFinieshed = callback;
}

void FileTransferDownLoadTask::BindInterruptedCallBack(std::function<void(FileTransferDownLoadTask*)> callback)
{
	_callbackInterrupted = callback;
}

void FileTransferDownLoadTask::BindProgressCallBack(std::function<void(FileTransferDownLoadTask*, uint32_t)> callback)
{
	_callbackProgress = callback;
}

void FileTransferDownLoadTask::RegisterTransInfo(const QString& filepath, const QString& md5, uint64_t filesize)
{
	file_path = filepath;
    _md5 = md5;
	file_size = filesize;
	IsRegister = true;
}
