#include "ModelManager.h"

ModelManager *ModelManager::Instance()
{
    static ModelManager* m_instance = new ModelManager();
    return m_instance;
}

ModelManager::ModelManager(QObject* parent){
    m_chatitemlistmodel=new ChatItemListModel();
    m_chatsessionmodel = new ChatSessionModel();
    m_userinfomodel = new UserInfoModel();
    m_loginmodel = new LoginModel();
    m_aiassistantmodel = new AIAssistantModel();
    m_aisummarymodel = new AISummaryModel();
    m_ocrmodel = new OCRModel();
}
