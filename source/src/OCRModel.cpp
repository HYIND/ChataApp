#include "OCRModel.h"
#include "FileTransManager.h"
#include <QVariantList>
#include <QVariantMap>

OCRModel::OCRModel()
{
}

OCRModel::~OCRModel()
{

}

bool OCRModel::isLoaded() const
{
    return OCRProcess::Instance()->isLoaded();
}

void OCRModel::addTask(QString fileid)
{
    if(!isLoaded())
        return;

    tasks.enqueue(fileid);
    updateResult(fileid,nullptr);

    processTask();
}

OCRTaskResult OCRModel::getState(QString fileid)
{
    for(int i=0;i<ocrresults.size();i++)
    {
        if(ocrresults[i].fileid==fileid)
        {
            if(ocrresults[i].task)
                return ocrresults[i].task->state;
            else
                return OCRTaskResult::Running;
        }
    }

    return OCRTaskResult::None;
}

QVariantList OCRModel::getResult(QString fileid)
{
    QVariantList results;

    for(int i=0;i<ocrresults.size();i++)
    {
        if(ocrresults[i].fileid==fileid && ocrresults[i].task && ocrresults[i].task->state == OCRTaskResult::Success)
        {
            auto &boxes = ocrresults[i].task->boxs;
            for (const auto& box : boxes) {
                QVariantMap item;
                item["x"] = box.x;
                item["y"] = box.y;
                item["width"] = box.w;
                item["height"] = box.h;
                item["text"] = box.text;
                item["conf"] = box.conf;
                results.append(item);
            }
            break;
        }
    }
    return results;
}


void OCRModel::processTask()
{
    LockGuard guard(lock);

    if(!isLoaded())
        return;

    while(!tasks.empty())
    {
        QString execfileid;
        if(tasks.dequeue(execfileid))
        {
            auto task = std::make_shared<OCRTask>();
            updateResult(execfileid,task);
            if(execfileid.isEmpty())
            {
                task->state = OCRTaskResult::Error;
                continue;
            }
            else
            {
                task->state = OCRTaskResult::Running;
                task->callback = std::bind(&OCRModel::recvtaskDone,this,std::placeholders::_1);
                task->filepath = FILETRANSMANAGER->FindDownloadPathByFileId(execfileid);
                if(task->filepath.isEmpty() || task->filepath == "")
                {
                    task->state = OCRTaskResult::Error;
                    continue;
                }
                OCRProcess::Instance()->addOCRTask(task);
            }
        }
    }
}

void OCRModel::recvtaskDone(std::shared_ptr<OCRTask> task)
{
    for(int i=0;i<ocrresults.size();i++)
    {
        if(ocrresults[i].task == task && task->state == OCRTaskResult::Success)
        {
            emit ocrTaskDone(ocrresults[i].fileid);
            break;
        }
    }
    processTask();
}

void OCRModel::updateResult(const QString &fileid, std::shared_ptr<OCRTask> task)
{
    for(int i=0;i<ocrresults.size();i++)
    {
        if(ocrresults[i].fileid==fileid)
        {
            ocrresults[i].task = task;
            return;
        }
    }
    ocrresults.emplace_back(OCRResult{fileid,task});
}

