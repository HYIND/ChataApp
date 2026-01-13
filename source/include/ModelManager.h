#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include <QObject>
#include <ChatItemListModel.h>
#include "ChatSessionModel.h"
#include "UserInfoModel.h"
#include "LoginModel.h"
#include "AIAssistantModel.h"
#include "AISummaryModel.h"

class ModelManager:public QObject
{
    Q_OBJECT
    Q_PROPERTY(ChatItemListModel* chatitemlistmodel READ chatitemlistmodel CONSTANT)

public:
    static ModelManager* Instance();
    ChatItemListModel* chatitemlistmodel() const { return m_chatitemlistmodel; }
    ChatSessionModel* chatsessionmodel() const { return m_chatsessionmodel; }
    UserInfoModel* userinfomodel() const { return m_userinfomodel; }
    LoginModel* loginmodel() const { return m_loginmodel; }
    AIAssistantModel* aiassistantModel() const {return m_aiassistantmodel;}
    AISummaryModel* aisummaryModel() const{return m_aisummarymodel;}

private:
    explicit ModelManager(QObject* parent = nullptr);

private:
    ChatItemListModel* m_chatitemlistmodel;
    ChatSessionModel* m_chatsessionmodel;
    UserInfoModel* m_userinfomodel;
    LoginModel* m_loginmodel;
    AIAssistantModel* m_aiassistantmodel;
    AISummaryModel* m_aisummarymodel;
};

#define MODELMANAGER ModelManager::Instance()
#define CHATITEMMODEL ModelManager::Instance()->chatitemlistmodel()
#define SESSIONMODEL ModelManager::Instance()->chatsessionmodel()
#define USERINFOMODEL ModelManager::Instance()->userinfomodel()
#define LOGINMODEL ModelManager::Instance()->loginmodel()
#define AIASSISTANTMODEL ModelManager::Instance()->aiassistantModel()
#define AISUMMARYMODEL ModelManager::Instance()->aisummaryModel()


#endif // MODELMANAGER_H
