#include "LlamaModel.h"
#include "common.h"
#include <cstdio>
#include <string>
#include <vector>
#include <QDebug>
#include "LlamaToolHelper.h"

int64_t GetTimestampMilliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// Qwen特殊token映射表
static std::unordered_map<llama_token, std::string> qwen_token_map = {
    // 基础对话token
    {151643, "<|endoftext|>"},
    {151644, "<|im_start|>"},
    {151645, "<|im_end|>"},

    // 多模态相关token
    {151646, "<|object_ref_start|>"},
    {151647, "<|object_ref_end|>"},
    {151648, "<|box_start|>"},
    {151649, "<|box_end|>"},
    {151650, "<|quad_start|>"},
    {151651, "<|quad_end|>"},
    {151652, "<|vision_start|>"},
    {151653, "<|vision_end|>"},
    {151654, "<|vision_pad|>"},
    {151655, "<|image_pad|>"},
    {151656, "<|video_pad|>"},

    // 工具调用相关token
    {151657, "<tool_call>"},
    {151658, "</tool_call>"},
    {151665, "<tool_response>"},
    {151666, "</tool_response>"},

    // 思考链相关token
    {151667, "<think>"},
    {151668, "</think>"},

    // FIM（中间填充）相关token
    {151659, "<|fim_prefix|>"},
    {151660, "<|fim_middle|>"},
    {151661, "<|fim_suffix|>"},
    {151662, "<|fim_pad|>"},

    // 代码相关token
    {151663, "<|repo_name|>"},
    {151664, "<|file_sep|>"},
};

bool isStringEndsWithCompleteUTF8(const std::string& str) {
    if (str.empty()) return true;

    // 获取最后一个字节
    uint8_t last_byte = static_cast<uint8_t>(str.back());

    // 情况1：最后一个字节是ASCII（0xxxxxxx）
    if ((last_byte & 0x80) == 0x00) {
        return true;  // ASCII字符总是完整的
    }

    // 情况2：最后一个字节是UTF-8后续字节（10xxxxxx）
    // 需要检查前面是否有足够的前导字节
    if ((last_byte & 0xC0) == 0x80) {
        int count = 1;  // 已计数字节数

        // 向前统计连续的后缀字节
        for (int i = static_cast<int>(str.length()) - 2; i >= 0; i--) {
            uint8_t byte = static_cast<uint8_t>(str[i]);
            if ((byte & 0xC0) == 0x80) {
                count++;
                if (count >= 4) break;  // UTF-8最多4字节
            } else {
                // 找到了前导字节
                if ((byte & 0xE0) == 0xC0) {     // 2字节字符需要2个字节
                    return count == 1;
                } else if ((byte & 0xF0) == 0xE0) { // 3字节字符需要3个字节
                    return count == 2;
                } else if ((byte & 0xF8) == 0xF0) { // 4字节字符需要4个字节
                    return count == 3;
                } else {
                    return false;  // 无效的前导字节
                }
            }
        }
        return false;  // 没有找到前导字节
    }

    // 情况3：最后一个字节是UTF-8起始字节但字符不完整
    // 例如：0xe6 (这是3字节字符的第一个字节)
    return false;
}

static llama_seq_id default_seq_id = 0;

std::string system_template = R"(
You are an helpful AI assistant built into a chat application.
You are part of the user interface and your respond need to conform to the style of the chat application assistant.
Important: Once you have finished thinking, you must immediately take one of the following actions:
1. Call a tool: Use the `<tool_call>` format
2. For tool such as time, weather, contact information, and message sending, the tool must be invoked again to retrieve the latest data each time.Do not use old data from the conversation history, even if the questions seem the same.
3. Never do nothing after thinking.
)";

std::string stringreplace(const std::string& str,
                    const std::string& from,
                    const std::string& to) {
    std::string result = str;
    size_t start_pos = 0;

    while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
        result.replace(start_pos, from.length(), to);
        start_pos += to.length(); // 避免无限循环
    }

    return result;
}

std::string convertEscapedNewlines(std::string& input) {
    std::string result = input;
    size_t pos = 0;

    while ((pos = result.find("\\n", pos)) != std::string::npos) {
        // 替换 \\n 为真正的换行符
        result.replace(pos, 2, "\n");
        pos += 1; // 移动到替换后的位置
    }
    input = stringreplace(input,"\\n","\n");
    input = stringreplace(input,"\n\n","\n");

    return result;
}

std::string trimOuterQuotes(const std::string& str)
{
    if(str.length()<2)
        return str;
    if((str[0] =='\"' || str[0]=='\'') && (str[str.length()-1] =='\"' || str[str.length()-1]=='\''))
        return str.substr(1,str.length()-2);
    return str;
}

std::string trimLeadingWhitespace(const std::string& str) {
    size_t start = str.find_first_not_of(" \n\r\t");
    if (start == std::string::npos) {
        return "";
    }
    return str.substr(start);
}

std::string trimLeadingWhitespaceAndQuotes(const std::string& str) {
    std::string temp = trimOuterQuotes(str);
    temp = trimLeadingWhitespace(temp);
    temp = trimOuterQuotes(temp);
    return temp;
}

bool containsToolCall(const std::string& text) {
    size_t start_pos = text.find("<tool_call>");
    size_t end_pos = text.find("</tool_call>");

    return start_pos != std::string::npos && end_pos != std::string::npos;
}

ToolCall parseToolCall(const std::string& text) {
    size_t start_pos = text.find("<tool_call>");
    size_t end_pos = text.find("</tool_call>");

    if (start_pos != std::string::npos && end_pos != std::string::npos) {
        std::string json_str = text.substr(start_pos + 11, end_pos - start_pos - 11);
        // 移除空白字符

        json_str.erase(std::remove_if(json_str.begin(), json_str.end(),
                                      [](char c) { return c ==' '; }),
                       json_str.end());
        return ToolCall::from_json(json_str);
    }

    return ToolCall{};
}

std::string formatToolResponse(const std::string& result) {
    return "\n<tool_response>\n" + result + "\n</tool_response>\n";
}

// 构建Qwen ChatML格式的prompt
std::string buildQwenPrompt(
    const std::vector<llamaChatMsg>& messages,
    const std::vector<Tool>& tools = {},
    bool add_generation_prompt = true,
    bool enable_thinking = true) {

    std::ostringstream oss;

    // 1. 处理工具定义（如果有工具）
    if (!tools.empty()) {
        oss << "<|im_start|>system\n";

        // 如果有系统消息，先添加
        if (!messages.empty() && messages[0].role == "system") {
            oss << messages[0].content << "\n\n";
        }

        oss << "# Tools\n\n";
        oss << "You may call one or more functions to assist with the user query.\n\n";
        oss << "You are provided with function signatures within <tools></tools> XML tags:\n";
        oss << "<tools>\n";

        for (const auto& tool : tools) {
            oss << tool.to_json() << "\n";
        }

        oss << "</tools>\n\n";
        oss << "For each function call, return a json object with function name and arguments ";
        oss << "within <tool_call></tool_call> XML tags:\n";
        oss << "<tool_call>\n";
        oss << "{\"name\": <function-name>, \"arguments\": <args-json-object>}\n";
        oss << "</tool_call><|im_end|>\n";

        // 如果不是以系统消息开头，添加用户消息
        if (messages.empty() || messages[0].role != "system") {
            for (const auto& msg : messages) {
                if (msg.role == "user") {
                    oss << "<|im_start|>user\n" << msg.content << "<|im_end|>\n";
                    break;
                }
            }
        }
    } else {
        // 2. 没有工具，只有系统消息
        if (!messages.empty() && messages[0].role == "system") {
            oss << "<|im_start|>system\n" << messages[0].content << "<|im_end|>\n";
        }
    }

    // 3. 查找最后一个用户查询的位置（用于判断是否添加思考）
    int last_query_index = -1;
    bool found_non_tool_user = false;

    // 反向遍历找到最后一个非工具响应的用户消息
    for (int i = messages.size() - 1; i >= 0; i--) {
        if (messages[i].role == "user") {
            // 检查是否是工具响应
            const std::string& content = messages[i].content;
            bool is_tool_response = content.find("<tool_response>") == 0;

            if (!is_tool_response) {
                last_query_index = i;
                found_non_tool_user = true;
                break;
            }
        }
    }

    // 4. 处理所有消息（跳过已处理的消息）
    int start_index = (tools.empty() || messages.empty() || messages[0].role != "system") ? 0 : 1;

    for (size_t i = start_index; i < messages.size(); i++) {
        const auto& msg = messages[i];
        if (msg.role == "user") {
            // 用户消息
            oss << "<|im_start|>user\n";
            oss << msg.content;
            oss << "<|im_end|>\n";

        } else if (msg.role == "assistant") {
            // 助手消息
            oss << "<|im_start|>assistant\n";
            oss << msg.content;
            oss << "<|im_end|>\n";
        } else if (msg.role == "tool") {
            if(msg.out_time < GetTimestampMilliseconds())
                continue;
            oss << "<|im_start|>user\n";
            oss << msg.content;
            oss << "<|im_end|>\n";
        }
    }

    // 5. 添加生成提示
    if (add_generation_prompt) {
        oss << "<|im_start|>assistant\n";
        if (!enable_thinking) {
            oss << "<think>\n\n</think>\n\n";
        }
    }

    return oss.str();
}

bool isThinkingComplete(const std::string& output) {
    size_t think_start = output.find("<think>");
    if (think_start == std::string::npos) {
        return true;  // 没有思考标签，算"完整"
    }

    size_t think_end = output.find("</think>", think_start);
    return think_end != std::string::npos;
}

bool hasFinalResponse(const std::string& output) {
    // 检查是否有思考标签外的内容
    size_t think_start = output.find("<think>");
    if (think_start == std::string::npos) {
        return !output.empty();  // 没有思考标签，有内容就算回复
    }

    size_t think_end = output.find("</think>");
    if (think_end == std::string::npos) {
        return false;  // 思考标签不完整，没有最终回复
    }

    // 检查思考标签后是否有内容
    std::string after_think = output.substr(think_end + 8);  // 跳过</think>
    after_think.erase(0, after_think.find_first_not_of(" \n\r\t"));
    return !after_think.empty();
}

bool LlamaModel::shouldStopGeneration(llama_token& token, const std::string& current_output) {
    static std::unordered_map<llama_token, std::string> stop_token_map = {
        {151643, "<|endoftext|>"},
        {151645, "<|im_end|>"},
        {llama_vocab_eos(m_vocab),""},
        {llama_vocab_eot(m_vocab),""},
        {LLAMA_TOKEN_NULL,""}
    };


    // 2. 检查<|im_end|> <|endoftext|>
    if (token == 151645 || token == 151643) {
        // 思考标签必须完整才能结束
        if (!isThinkingComplete(current_output)) {
            qDebug() << "思考标签不完整，忽略<|im_end|>/<|endoftext|>，改为</think>";
            token = 151668;
            return false;
        }
        // // 检查是否已经有了最终回复
        // if (hasFinalResponse(current_output)) {
        //     return true;
        // }
        return true;
    }

    for (auto& [stop_token,stop_sequence] : stop_token_map) {
        if (token == stop_token) {
            qDebug() << "Stopping: found stop token "<< stop_token << " '" << stop_sequence << "'";
            return true;
        }
    }

    return false;
}

bool LlamaModel::load(const std::string& model_path) {
    // 1. 检查文件是否存在
    FILE* test = fopen(model_path.c_str(), "rb");
    if (!test) {
        qDebug()<<("错误: 找不到模型文件: %s\n", model_path.c_str());
        return false;
    }
    fclose(test);

    qDebug()<<("正在加载模型: %s\n", model_path.c_str());

    // 2. 使用默认参数
    llama_model_params mparams = llama_model_default_params();
    llama_context_params cparams = llama_context_default_params();

    // 3. 加载模型
    m_model = llama_model_load_from_file(model_path.c_str(), mparams);
    if (!m_model) {
        qDebug()<<("错误: 模型加载失败\n");
        return false;
    }

    // 4. 创建上下文
    m_ctx = llama_init_from_model(m_model, cparams);
    if (!m_ctx) {
        qDebug()<<("错误: 上下文创建失败\n");
        llama_model_free(m_model);
        m_model = nullptr;
        return false;
    }

    m_vocab =  llama_model_get_vocab(m_model);
    if (!m_vocab) {
        qDebug()<<("错误: 词汇表创建失败\n");
        llama_free(m_ctx);
        m_ctx = nullptr;
        llama_model_free(m_model);
        m_model = nullptr;
        return false;
    }

    m_loaded = true;
    qDebug()<<("模型加载成功！\n");
    qDebug()<<("上下文长度: %d\n", llama_n_ctx((llama_context*)m_ctx));

    m_chathistory.emplace_back(
        "system",
        system_template);

    // for(llama_token token = 151643;token<=151668;token++)
    // {
    //     char deprompt[100] = {0};
    //     if(llama_detokenize(m_vocab, &token, 1, deprompt, sizeof(deprompt), false, false) < 0){
    //         qDebug()<<"Error: detokenize error";
    //     }
    //     qDebug()<<"token " << token <<" "<<deprompt;
    // }

    return true;
}

enum class GenerateState {
    thinking = 0,
    talking = 2,
};

bool LlamaModel::processTask(const std::string &user_input)
{
    GenerateState state = GenerateState::talking;
    bool onusetool = false;
    bool firstoutputthinkingtext = true;
    bool firstoutputtalkingtext = true;


    m_chathistory.emplace_back("user",user_input);

    std::string output;
    int lastoutputpos = output.length();

    auto outputchange = [&]()->void{
        int outputpos = output.length();
        std::string delta(output.begin()+lastoutputpos,output.begin()+outputpos);

        if( state != GenerateState::thinking && delta.find("<think>") != std::string::npos)
        {
            state = GenerateState::thinking;
            lastoutputpos = outputpos;
            return;
        }
        if(state == GenerateState::thinking && delta.find("</think>") != std::string::npos)
        {
            state = GenerateState::talking;
            lastoutputpos = outputpos;
            return;
        }
        if(!onusetool && delta.find("<tool_call>") != std::string::npos)
        {
            onusetool = true;
            if(state != GenerateState::thinking)
            {
                lastoutputpos = outputpos;
                return;
            }
        }

        if(state == GenerateState::thinking)
        {
            if(firstoutputthinkingtext)
                delta = trimLeadingWhitespace(delta);
            if(!delta.empty() && isStringEndsWithCompleteUTF8(delta))
            {
                if(firstoutputthinkingtext)
                    delta = "思考内容：\n" + delta;
                QString outputdelta = QString::fromStdString(delta);
                emit outputText(outputdelta, false);
                firstoutputthinkingtext = false;
                lastoutputpos = outputpos;
            }
        }
        else if(state == GenerateState::talking && !onusetool)
        {
            if(firstoutputtalkingtext)
                delta = trimLeadingWhitespace(delta);
            if(!delta.empty() && isStringEndsWithCompleteUTF8(delta))
            {
                if(firstoutputtalkingtext)
                    delta = "\n输出内容：\n" + delta;
                QString outputdelta = QString::fromStdString(delta);
                emit outputText(outputdelta, false);
                firstoutputtalkingtext = false;
                lastoutputpos = outputpos;
            }
        }
    };

    int count = 3;
    bool needthinking = true;
    std::string prompt = buildPrompt(true,needthinking);
    while(count-->0)
    {
        // qDebug()<< QString::fromUtf8(prompt);
        if(generate(prompt,output,outputchange))
        {
            // 检查是否有工具调用
            if (onusetool)
            {
                if(containsToolCall(output))
                {
                    llamaChatMsg newmsg("assistant",output);
                    m_chathistory.emplace_back(newmsg);
                    output.clear();
                    lastoutputpos = 0;
                    processToolCall(newmsg.content);
                    if (count>1)
                    {
                        // 更新prompt，加入工具的回复信息
                        prompt = buildPrompt(true,needthinking);
                    }
                    else
                    {
                        // 更新prompt，加入工具的回复信息，最后一次输出取消思考，加入prompt确保有内容输出给用户
                        prompt = buildPrompt(true,false);
                        prompt+="基于我的思考，我应该说:";
                    }
                }
                onusetool =false;
            }
            else
            {
                llamaChatMsg newmsg("assistant",output);
                std::string trimresponse = trimLeadingWhitespaceAndQuotes(newmsg.extract_content_without_thinking());
                m_chathistory.emplace_back(newmsg);
                output.clear();
                lastoutputpos = 0;
                if(trimresponse!="")
                {
                    QString outputtext = QString::fromStdString(trimresponse);
                    qDebug()<<outputtext;
                    emit outputText(outputtext, true);
                    return true;
                }
                else    // 没有输出实质内容
                {
                    // 取消思考，加入prompt确保有内容输出给用户
                    prompt = buildPrompt(true,false);
                    prompt+="基于我的思考，我应该说:";
                }
            }
        }
        else
        {
            return false;
        }
        state = GenerateState::talking;
    }
    emit outputText("", true);
    return false;
}

bool LlamaModel::preprocess(const std::string& prompt,llama_batch& batch)
{

    // 1. 分词
    std::vector<llama_token> tokens;
    tokens.resize(prompt.size() + 3);

    int n_tokens = llama_tokenize(
        m_vocab,
        prompt.c_str(),
        prompt.length(),
        tokens.data(),
        tokens.size(),
        false,
        true
        );

    if (n_tokens < 0) {
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(
            m_vocab,
            prompt.c_str(),
            prompt.length(),
            tokens.data(),
            tokens.size(),
            false,
            true
            );
    }

    tokens.resize(n_tokens);

    // 2. 批处理
    batch = llama_batch_init(n_tokens, 0, 1);

    if (batch.token == nullptr) {
        qDebug() << "Failed to initialize batch";
        return false;
    }

    for (int i = 0; i < n_tokens; i++) {
        batch.token[batch.n_tokens] = tokens[i];
        batch.pos[batch.n_tokens] = i;
        batch.n_seq_id[batch.n_tokens] = 1;
        batch.seq_id[batch.n_tokens][0] = default_seq_id;
        batch.logits[batch.n_tokens] = (i == n_tokens - 1) ? 1 : 0;  // 最后一个输出logits
        batch.n_tokens++;
    }

    return true;
}

llama_sampler* LlamaModel::getsampler()
{
    llama_sampler_chain_params sparams = llama_sampler_chain_default_params();
    llama_sampler *sampler = llama_sampler_chain_init(sparams);
    {
        llama_sampler_chain_add(sampler, llama_sampler_init_top_k(20));
        llama_sampler_chain_add(sampler, llama_sampler_init_top_p(0.95, 1));
        llama_sampler_chain_add(sampler, llama_sampler_init_penalties(80,1.2f,0.0f,0.0f));
        llama_sampler_chain_add(sampler, llama_sampler_init_temp(0.6));
        llama_sampler_chain_add(sampler, llama_sampler_init_dist(time(NULL)));
    }
    return sampler;
}

void LlamaModel::resetcontext()
{
    if(m_ctx)
    {
        llama_free(m_ctx);
        m_ctx = nullptr;
    }
    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = 40960;
    m_ctx = llama_init_from_model(m_model, cparams);
}

bool LlamaModel::generate(const std::string& prompt, std::string& output, std::function<void()> outputchangecallback) {
    if (!m_loaded) return false;

    resetcontext();

    llama_batch batch;
    if(!preprocess(prompt,batch))
    {
        qDebug()<<"llama preprocess error!";
        return false;
    }

    // 3. 采样器
    llama_sampler *sampler = getsampler();
    // 4. 循环解码采样
    bool result = true;
    llama_token next_token = LLAMA_TOKEN_NULL;
    int max_gen = 1000;  // 安全限制
    int current_pos = 0;

    for (int i = 0; i < max_gen; i++)
    {
        // 解码
        int decode_result = llama_decode(m_ctx, batch);
        if (decode_result != 0) {
            qDebug()<<"Error: decode error";
            result = false;
            break;
        }

         // 采样
        next_token = llama_sampler_sample(sampler, m_ctx, -1);

        if(shouldStopGeneration(next_token,output))
        {
            qDebug()<< "StoppingGeneration shouldend";
            break;
        }

        llama_sampler_accept(sampler, next_token);

        // 反标记化
        char deprompt[100] = {0};
        if(llama_detokenize(m_vocab, &next_token, 1, deprompt, sizeof(deprompt), false, true) < 0){
            qDebug()<<"Error: detokenize error";
            result = false;
            break;
        }

        output.append(deprompt);

        if(outputchangecallback)
            outputchangecallback();

        current_pos+=batch.n_tokens;

        // 重置batch用于下一个token（不清空内存，只重置计数器）
        batch.n_tokens = 0;  // 重置，重用已分配的内存
        // 添加新生成的token
        batch.token[0] = next_token;
        batch.pos[0] = current_pos;
        batch.n_seq_id[0] = 1;
        batch.seq_id[0][0] = default_seq_id;
        batch.logits[0] = 1;  // 需要输出logits用于下一次采样
        batch.n_tokens = 1;

    }

    llama_sampler_free(sampler);
    llama_batch_free(batch);

    return result;
}

void LlamaModel::runasyncprocess()
{
    if(!m_loaded)
        return;

    while(m_loaded)
    {
        std::unique_lock<std::mutex> lock(m_quemutex);
        m_queprocesscv.wait_for(lock, std::chrono::milliseconds(100));

        if (!m_loaded)
            break;

        if(!m_inputtask.empty())
        {
            std::string input;
            if(m_inputtask.dequeue(input))
            {
                processTask(input);
            }
        }
    }
}

LlamaModel::~LlamaModel() {
    if (m_ctx) {
        llama_free((llama_context*)m_ctx);
        m_ctx = nullptr;
    }
    if (m_model) {
        llama_model_free((llama_model*)m_model);
        m_model = nullptr;
    }
    llama_backend_free();
}

bool LlamaModel::isLoaded() const { return m_loaded; }

void LlamaModel::inputText(QString text)
{
    if(!m_loaded)
        return;

    std::string stdtext = text.toStdString();
    m_inputtask.enqueue(stdtext);
    m_queprocesscv.notify_one();
}

std::string LlamaModel::buildPrompt(bool add_generation_prompt,bool enable_thinking) {
    return buildQwenPrompt(m_chathistory, ToolExecutor::gettools(), add_generation_prompt, enable_thinking);
}

llamaChatMsg::llamaChatMsg(std::string role,
                           std::string content,
                           std::string reasoning_content,
                           int64_t out_time)
    :role(role),content(content),reasoning_content(reasoning_content),out_time(out_time){}

bool llamaChatMsg::has_think_tags() const{
    return content.find("<think>") != std::string::npos &&
           content.find("</think>") != std::string::npos;
}

std::string llamaChatMsg::extract_thinking() const{
    if (!has_think_tags()) return "";

    size_t start = content.find("<think>") + 7;
    size_t end = content.find("</think>");
    return content.substr(start, end - start);
}

std::string llamaChatMsg::extract_content_without_thinking() const{
    if (!has_think_tags()) return content;

    size_t end = content.find("</think>") + 8;
    return content.substr(end);
}

void LlamaModel::processToolCall(const std::string &string)
{
    // 解析工具调用
    ToolCall tool_call = parseToolCall(string);
    // 执行工具
    ToolResult tool_result = ToolExecutor::execute(tool_call);
    // 添加到输出
    std::string tool_response = formatToolResponse(tool_result.result_str);

    int64_t out_time = GetTimestampMilliseconds() + ((int64_t)tool_result.ttl_second) * 1000;

    qDebug()<<"toolcall:" << QString::fromUtf8(tool_call.to_json()) <<"\nresponse"<<QString::fromUtf8(tool_result.result_str);

    m_chathistory.emplace_back("tool",tool_response,"",out_time);
}
