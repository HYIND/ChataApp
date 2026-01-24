#include "AISummaryModel.h"
#include "ModelManager.h"

AISummaryModel::AISummaryModel()
{
    m_llclient = nullptr;
    curstate = 0;
    curtoken = "";
}

AISummaryModel::~AISummaryModel()
{

}

bool AISummaryModel::isLoaded() const
{
    return LlamaModel::Instance()->isLoaded();
}

void AISummaryModel::addSummaryTask(QString goaltoken)
{
    Need();
    if(!LlamaModel::Instance()->isLoaded() || !m_llclient)
        return;

    tasks.enqueue(goaltoken);
    updateResult(goaltoken,"",0);

    processTask();
}

int AISummaryModel::getSummaryState(QString goaltoken)
{
    for(int i=0;i<results.size();i++)
    {
        if(results[i].token==goaltoken)
            return results[i].state;
    }

    return -1;
}

QString AISummaryModel::getSummaryText(QString goaltoken)
{
    for(int i=0;i<results.size();i++)
    {
        if(results[i].token==goaltoken && results[i].state == 1)
            return results[i].summary;
    }
    return "";
}

void AISummaryModel::processTask()
{
    LockGuard guard(lock);

    Need();
    if(!LlamaModel::Instance()->isLoaded() || !m_llclient)
        return;

    if(curstate == 0 && !tasks.empty())
    {
        QString exectoken;
        if(tasks.dequeue(exectoken) && !exectoken.isEmpty())
        {
            m_llclient->clearHistory();

            QList<ChatMsg> chatmsgs;
            if(!CHATITEMMODEL->getMsgsByToken(exectoken,chatmsgs))
            {
                updateResult(exectoken,"",2);
                return;
            }
            QString messagestext = GetInputTextFromChatMsgs(chatmsgs);

            m_llclient->inputText(messagestext,false);
            curstate = 1;
            curtoken = exectoken;
        }
    }
}

void AISummaryModel::recvOutPutText(QString text, int operation)
{
    if(operation == 0)
    {
        updateResult(curtoken,text,1);
        emit summaryDone(curtoken,text);

        m_llclient->clearHistory();
        curstate = 0;
        curtoken = "";
        processTask();
    }
}

void AISummaryModel::Need()
{
    if(!m_llclient)
    {
        if(LlamaModel::Instance()->isLoaded())
        {
            m_llclient = LlamaModel::Instance()->CreateNewClient(ProcessPriority::Last);
            m_llclient->bindOutPutTextCallback(std::bind(&AISummaryModel::recvOutPutText,this,std::placeholders::_1,std::placeholders::_2));
        }
    }
}

QString AISummaryModel::GetInputTextFromChatMsgs(QList<ChatMsg> &chatmsgs)
{
    QString result;

    // 添加总结提示词
    result.append(QString("以下是聊天记录，请帮我总结主要内容，其中我的用户名是[%1]""：\n\n").arg(USERINFOMODEL->username()));

    int textCount = 0;
    int mediaCount = 0;

    for (const auto& msg : chatmsgs) {
        switch (msg.type) {
        case MsgType::text: {
            QString timeStr = msg.time.toString("MM-dd hh:mm");
            QString line = QString("[%1] %2: %3\n")
                               .arg(timeStr)
                               .arg(msg.name)
                               .arg(msg.msg);
            result.append(line);
            textCount++;
            break;
        }
        case MsgType::picture: {
            QString line = QString("[%1] %2: %3\n")
                               .arg(msg.time.toString("MM-dd hh:mm"))
                               .arg(msg.name)
                               .arg("[发送了图片]");
            result.append(line);
            mediaCount++;
            break;
        }
        case MsgType::file: {
            QString line = QString("[%1] %2: %3\n")
                               .arg(msg.time.toString("MM-dd hh:mm"))
                               .arg(msg.name)
                               .arg(QString("[发送了文件: %1]").arg(msg.filename));
            result.append(line);
            mediaCount++;
            break;
        }
        }
    }

    // 添加统计信息
    result.append(QString("\n\n总计: %1条文字消息, %2条媒体消息")
                      .arg(textCount)
                      .arg(mediaCount));

    result.append(QString("\n不需要回复我\"好的\"这些回复词，直接简洁的告诉我你总结的主要内容就好！"));

    return result;
}

void AISummaryModel::updateResult(const QString &goaltoken, const QString &text, int state)
{
    for(int i=0;i<results.size();i++)
    {
        if(results[i].token==goaltoken)
        {
            results[i].summary = text;
            results[i].state = state;
            return;
        }
    }
    results.emplace_back(SummaryResult{goaltoken,text,state});
}

