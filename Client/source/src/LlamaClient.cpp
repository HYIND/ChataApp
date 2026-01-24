#include "LlamaClient.h"
#include <string>
#include <QDebug>

LlamaClient::LlamaClient(const std::string &sessionid)
{
    m_sessionid = sessionid;
}

LlamaClient::~LlamaClient()
{
    Release();
}

void LlamaClient::Release()
{
    if(!m_sessionid.empty())
    {
        LlamaModel::Instance()->CloseClient(m_sessionid);
        m_sessionid = "";
    }
}

void LlamaClient::inputText(QString text, bool thinkingEnabled)
{
    LlamaModel::Instance()->inputText(m_sessionid,text,thinkingEnabled);
}

void LlamaClient::pauseGenerate()
{
    LlamaModel::Instance()->pauseGenerate(m_sessionid);
}

void LlamaClient::continueGenerate()
{
    LlamaModel::Instance()->continueGenerate(m_sessionid);
}

void LlamaClient::reGenerate(bool thinkingEnabled)
{
    LlamaModel::Instance()->reGenerate(m_sessionid,thinkingEnabled);
}

void LlamaClient::clearHistory()
{
    LlamaModel::Instance()->clearHistory(m_sessionid);
}

void LlamaClient::bindOutPutTextCallback(std::function<void (QString, int)> callback)
{
    LlamaModel::Instance()->bindOutPutTextCallback(m_sessionid, callback);
}
