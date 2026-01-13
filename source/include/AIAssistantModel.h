#ifndef _AIASSISTANTMODEL_H
#define _AIASSISTANTMODEL_H

#include "LlamaClient.h"


class AIAssistantModel:public QObject
{
    Q_OBJECT

public:
    AIAssistantModel();
    ~AIAssistantModel();

public slots:
    bool isLoaded() const;
    void inputText(QString text,bool thinkingEnabled);
    void pauseGenerate();
    void continueGenerate();
    void reGenerate(bool thinkingEnabled);
    void clearHistory();

    void recvOutPutText(QString text,int operation);

signals:
    void outputText(QString text,int operation);

private:
    void Need();

private:
    LlamaClient* m_llclient;
};

#endif
