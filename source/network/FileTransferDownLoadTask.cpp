#include "FileTransferDownLoadTask.h"
#include "ConnectManager.h"
#include "NetWorkHelper.h"

using namespace std;

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
				merged.back().range_right = max(merged.back().range_right, sorted[i].range_right);
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

QString getChunkFilePath(const QString& filepath)
{
	return filepath + "__chunks";
}

bool ExtractBinaryJson(json jsbinary, std::vector<uint8_t>& v)
{
	bool result = false;
	if (jsbinary.is_object() && jsbinary.contains("bytes"))
	{
		const auto& bytes_field = jsbinary["bytes"];
		if (bytes_field.is_array())
		{
			try
			{
				const size_t size = bytes_field.size();
				v.reserve(v.size() + size); // 预分配空间

				for (const auto& item : bytes_field)
				{
					if (!item.is_number())
					{
						v.clear();
						return false;
					}
					v.push_back(static_cast<uint8_t>(item.get<int>()));
				}
				result = true;
			}
			catch (const json::exception& e)
			{
				cout << "error when ExtractBinaryJson: " + std::string(e.what()) << endl;
				result = false;
			}
		}
	}
	return result;
}

FileTransferDownLoadTask::FileTransferDownLoadTask(const QString& taskid, const QString& filepath_dir)
	: FileTransferTask(taskid, filepath_dir)
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
		vector<FileTransferChunkInfo> chunkmap;
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
	return parseresult;
}

bool FileTransferDownLoadTask::ReadChunkFile()
{
	chunk_map.clear();
	QString ChunkFilePath = getChunkFilePath(file_path);

	bool hasexist = FileIOHandler::Exists(ChunkFilePath);

	IsChunkFileEnable = chunkfile_io.Open(ChunkFilePath, FileIOHandler::OpenMode::READ_WRITE);
	if (hasexist && IsChunkFileEnable)
	{
		Buffer buf;
		chunkfile_io.Seek();
		chunkfile_io.Read(buf, chunkfile_io.GetSize());
		string js_str(buf.Byte(), buf.Length());
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
		string filename = js.at("filename");

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

	//vector<uint8_t> bytes;
	//error = !ExtractBinaryJson(js["data"], bytes);
	//if (error)
	//{
	//	OccurError();
	//	return;
	//}
	//chunkdata.FromBinary(bytes);

	//if (chunkdata.buf.Length() < chunksize)
	//{
	//	error = true;
	//	OccurError();
	//	return;
	//}

	if (buf.Remaind() < chunksize)
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
            chunk_map.emplace_back(0, chunkdata.range_left, chunkdata.range_left + truthwritecount);
            chunk_map = mergeChunks(chunk_map);
			// displayTransferProgress(file_size, chunk_map);
        }
	}
	else
	{
		OccurError();
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
		js_reply["taskid"] = task_id.toStdString();
		js_reply["result"] = 1;
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
	if (CheckFinish())
	{
		file_io.Truncate(file_size);
		OccurFinish();
	}

	if (IsFinished)
	{
		json js_reply;
		js_reply["command"] = 8010;
		js_reply["taskid"] = task_id.toStdString();
		js_reply["result"] = IsFinished ? 1 : 0;

		NetWorkHelper::SendMessagePackage(&js_reply);
	}
	else
	{
		OccurError();
	}
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
		ReleaseSource();
		FileIOHandler::Remove(chunkfile_io.FilePath());
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
		ReleaseSource();
		FileIOHandler::Remove(chunkfile_io.FilePath());
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

void FileTransferDownLoadTask::RegisterTransInfo(const QString& filepath, uint64_t filesize)
{
	file_path = filepath;
	file_size = filesize;
	IsRegister = true;
}
