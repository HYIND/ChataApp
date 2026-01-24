#ifndef _AISUMMARYMODEL_H
#define _AISUMMARYMODEL_H

#include "LlamaClient.h"
#include "SafeStl.h"
#include "ChatItemListModel.h"

struct SummaryResult
{
    QString token;
    QString summary;
    int state;  //0执行中,1完成,2失败
};

class AISummaryModel:public QObject
{
    Q_OBJECT

public:
    AISummaryModel();
    ~AISummaryModel();

public slots:
    bool isLoaded() const;
    void addSummaryTask(QString goaltoken);
    int getSummaryState(QString goaltoken);
    QString getSummaryText(QString goaltoken);

public:
    void recvOutPutText(QString text,int operation);

signals:
    void summaryDone(QString goaltoken,QString summarydata);

private:
    void Need();
    void processTask();
    QString GetInputTextFromChatMsgs(QList<ChatMsg>& chatmsgs);
    void updateResult(const QString& goaltoken, const QString& text ,int state);

private:
    LlamaClient* m_llclient;

    CriticalSectionLock lock;
    int curstate;       //0,空闲；1执行中
    QString curtoken;

    SafeQueue<QString> tasks;
    std::vector<SummaryResult> results;
};

#endif
