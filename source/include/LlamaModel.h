#ifndef LLAMA_MODEL_H
#define LLAMA_MODEL_H

#include <vector>
#include <string>
#include <QObject>
#include "SafeStl.h"
#include <condition_variable>
#include <mutex>
#include "llama-cpp.h"
#include "LlamaToolHelper.h"

struct llamaChatMsg;
struct AITask;

struct llamaChatMsg{
    std::string role;
    std::string content;
    std::string reasoning_content;  // 思考内容（如果有）
    int64_t out_time;
    bool usetool;
    bool usethinking;

    int tokensize;
    std::vector<llama_token> tokens;

    llamaChatMsg(std::string role,std::string content,std::string reasoning_content="",int64_t out_time = INT64_MAX, bool usetool = false);
    bool has_think_tags() const;
    std::string extract_thinking() const;
    std::string extract_content_without_thinking() const;
};

enum class GenerateState {
    none = -1,
    thinking = 0,
    toolusing = 1,
    talking = 2,
};

enum class TaskState
{
    Normal = 0,
    ManualPause = 1,         //手动暂停
    MaxGenTokenPause = 2,    //单次生成token限制暂停
    MaxGenCountPause = 3,   //单任务循环多次生成次数限制暂停
};

struct AITask
{
    std::string userinput; //用于增量更新
    std::vector<llamaChatMsg> toolsmsg; //用于增量更新
    std::string extraprompt;  //用于增量更新
    std::string output;

    GenerateState state = GenerateState::talking;
    GenerateState laststate = GenerateState::none;

    bool onusetool = false;
    bool firstoutputthinkingtext = true;
    bool firstoutputtalkingtext = true;
    bool firstoutputtoolusingtext = true;

    int generatecount = 3;
    bool needthinking = true;
    int lastoutputpos = 0;

    llama_token next_token = LLAMA_TOKEN_NULL;
    int current_gen = 0;

    TaskState taskstate = TaskState::Normal;
};

class LlamaModel :public QObject{
    Q_OBJECT
public:
    bool load(const std::string& model_path);
    bool processTask(AITask &user_input);
    bool generate(AITask& task,std::function<void()> outputchangecallback=nullptr);

    void runasyncprocess();

    ~LlamaModel();

public slots:
    bool isLoaded() const;
    void inputText(QString text,bool thinkingEnabled);
    void pauseGenerate();
    void continueGenerate();
    void reGenerate(bool thinkingEnabled);
    void clearHistory();

signals:
    void outputText(QString text,int operation); //0结束 1输出内容 2暂停

private:
    std::string buildQwenPrompt(bool add_generation_prompt = true,bool enable_thinking = true);
    bool preprocess(AITask& task);
    llama_sampler* getsampler();
    void resetcontext();
    void clearcontext();

    void processToolCall(AITask& task, const std::string& string);
    bool shouldStopGeneration(llama_token& token, const std::string& current_output);

    void ClearPausedTask();

    int GetBatchToProcess(llama_batch& batch, int ori_batch_size);

private:
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    const llama_vocab* m_vocab = nullptr;
    std::vector<llamaChatMsg> m_chathistory;
    bool m_loaded = false;

    SafeQueue<AITask> m_inputtask;
    std::mutex m_quemutex;
    std::condition_variable m_queprocesscv;

    bool shouldpause = false;

    llama_sampler *m_sampler = nullptr;
    std::vector<llama_token> m_curtokens;
    int m_todecodepos;
};

#endif
