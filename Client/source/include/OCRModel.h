#ifndef _OCRMODEL_H
#define _OCRMODEL_H

#include "OCRProcess.h"
#include "SafeStl.h"

struct OCRResult
{
    QString fileid;
    std::shared_ptr<OCRTask> task;
};

class OCRModel:public QObject
{
    Q_OBJECT

public:
    OCRModel();
    ~OCRModel();

public slots:
    bool isLoaded() const;
    void addTask(QString fileid);
    OCRTaskResult getState(QString fileid);
    QVariantList getResult(QString fileid);

public:
    void recvtaskDone(std::shared_ptr<OCRTask> task);

signals:
    void ocrTaskDone(QString fileid);

private:
    void processTask();
    void updateResult(const QString &fileid, std::shared_ptr<OCRTask> task);

private:
    CriticalSectionLock lock;

    SafeQueue<QString> tasks;
    std::vector<OCRResult> ocrresults;
};

#endif
