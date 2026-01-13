#ifndef LLAMA_CLIENT_H
#define LLAMA_CLIENT_H

#include <vector>
#include <string>
#include <QObject>
#include "SafeStl.h"
#include "LlamaModel.h"

class LlamaClient :public QObject{
    Q_OBJECT
public:
    LlamaClient(const std::string& sessionid);
    ~LlamaClient();
    void Release();

public:
    void inputText(QString text,bool thinkingEnabled);
    void pauseGenerate();
    void continueGenerate();
    void reGenerate(bool thinkingEnabled);
    void clearHistory();

    void bindOutPutTextCallback(std::function<void(QString,int)> callback);
private:
    std::function<void(QString,int)> outputtextcallback;
    std::string m_sessionid;
};

#endif
