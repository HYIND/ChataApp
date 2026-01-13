#include "AIAssistantModel.h"

AIAssistantModel::AIAssistantModel()
{
    m_llclient = nullptr;
}

AIAssistantModel::~AIAssistantModel()
{
    if(m_llclient)
    {
        m_llclient->Release();
        delete m_llclient;
        m_llclient = nullptr;
    }
}

bool AIAssistantModel::isLoaded() const
{
    return LlamaModel::Instance()->isLoaded();
}

void AIAssistantModel::inputText(QString text, bool thinkingEnabled)
{
    Need();
    if(!LlamaModel::Instance()->isLoaded() || !m_llclient)
        return;

    m_llclient->inputText(text,thinkingEnabled);
}

void AIAssistantModel::pauseGenerate()
{
    Need();
    if(!LlamaModel::Instance()->isLoaded() || !m_llclient)
        return;

    m_llclient->pauseGenerate();
}

void AIAssistantModel::continueGenerate()
{
    Need();
    if(!LlamaModel::Instance()->isLoaded() || !m_llclient)
        return;

    m_llclient->continueGenerate();
}

void AIAssistantModel::reGenerate(bool thinkingEnabled)
{
    Need();
    if(!LlamaModel::Instance()->isLoaded() || !m_llclient)
        return;

    m_llclient->reGenerate(thinkingEnabled);
}

void AIAssistantModel::clearHistory()
{
    Need();
    if(!LlamaModel::Instance()->isLoaded() || !m_llclient)
        return;

    m_llclient->clearHistory();
}

void AIAssistantModel::recvOutPutText(QString text, int operation)
{
    emit outputText(text,operation);
}

void AIAssistantModel::Need()
{
    if(!m_llclient)
    {
        if(LlamaModel::Instance()->isLoaded())
        {
            m_llclient = LlamaModel::Instance()->CreateNewClient();
            m_llclient->bindOutPutTextCallback(std::bind(&AIAssistantModel::recvOutPutText,this,std::placeholders::_1,std::placeholders::_2));
        }
    }
}
