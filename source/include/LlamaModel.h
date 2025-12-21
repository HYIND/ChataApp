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

struct llamaChatMsg{
    std::string role;
    std::string content;
    std::string reasoning_content;  // 思考内容（如果有）
    int64_t out_time;

    llamaChatMsg(std::string role,std::string content,std::string reasoning_content="",int64_t out_time = INT64_MAX);
    bool has_think_tags() const;
    std::string extract_thinking() const;
    std::string extract_content_without_thinking() const;
};

class LlamaModel :public QObject{
    Q_OBJECT
public:
    bool load(const std::string& model_path);
    bool processTask(const std::string& user_input);
    bool generate(const std::string& prompt, std::string& out,std::function<void()> outputchangecallback=nullptr);

    void runasyncprocess();

    ~LlamaModel();

public slots:
    bool isLoaded() const;
    void inputText(QString text);

signals:
    void outputText(QString text,bool isend);

private:
    std::string buildPrompt(bool add_generation_prompt = true,bool enable_thinking = true);
    bool preprocess(const std::string& prompt,llama_batch& batch);
    llama_sampler* getsampler();
    void resetcontext();

    void processToolCall(const std::string& string);
    bool shouldStopGeneration(llama_token& token, const std::string& current_output);

private:
    llama_model* m_model = nullptr;
    llama_context* m_ctx = nullptr;
    const llama_vocab* m_vocab = nullptr;
    std::vector<llamaChatMsg> m_chathistory;
    bool m_loaded = false;

    SafeQueue<std::string> m_inputtask;
    std::mutex m_quemutex;
    std::condition_variable m_queprocesscv;
};

#endif
