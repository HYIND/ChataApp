#ifndef _OCRPROCESS_H
#define _OCRPROCESS_H

#include <QString>
#include <qobject.h>
#include "SafeStl.h"
#include <memory>
#include "tesseract/baseapi.h"

enum class OCRTaskResult
{
    None = -2,
    Error = -1,
    Running = 0,
    Success = 1
};

struct OCRBox
{
    int x;
    int y;
    int w;
    int h;
    int conf;

    QString text;
};

struct OCRTask
{
    QString filepath;
    std::vector<OCRBox> boxs;
    OCRTaskResult state;

    std::function<void(std::shared_ptr<OCRTask>)> callback;
};

class OCRProcess
{

public:
    static OCRProcess* Instance();
    ~OCRProcess();

private:
    OCRProcess();
    bool Init();

public:
    bool isLoaded() const;
    void addOCRTask(std::shared_ptr<OCRTask> task);

private:
    void processLoop();
    void processTask(std::shared_ptr<OCRTask> task);

private:
    bool m_isLoad;
    bool m_stoploop;
    SafeQueue<std::shared_ptr<OCRTask>> m_tasks;
    tesseract::TessBaseAPI *m_api;

    CriticalSectionLock m_looplock;
    ConditionVariable m_loopcv;
};

#endif
