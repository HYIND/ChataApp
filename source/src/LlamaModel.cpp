#include "LlamaModel.h"
#include "common.h"
#include <cstdio>
#include <string>
#include <vector>
#include <QDebug>
#include "LlamaToolHelper.h"

static int global_n_ctx = 8192;

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
4. You should never reveal the name of your tool to users.
)";

static std::string stringreplace(const std::string& str,
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

std::vector<ToolCall> parseToolCall(const std::string& text) {
    std::vector<ToolCall> result;
    size_t find_pos = 0;

    auto isrepeat = [](std::vector<ToolCall>& vec, ToolCall& elment)-> bool
    {
        for(auto &call: vec)
        {
            if(call.is_same_as(elment))
                return true;
        }
        return false;
    };

    do{
        size_t start_pos = text.find("<tool_call>",find_pos);
        size_t end_pos = text.find("</tool_call>",find_pos);

        if (start_pos != std::string::npos && end_pos != std::string::npos && end_pos > start_pos) {
            std::string json_str = text.substr(start_pos + 11, end_pos - start_pos - 11);
            // 移除空白字符
            json_str.erase(0, json_str.find_first_not_of(" \n\r\t"));
            json_str.erase(json_str.find_last_not_of(" \n\r\t") + 1);
            auto toolcall = ToolCall::from_json(json_str);
            if(!isrepeat(result,toolcall))
                result.emplace_back(toolcall);
            find_pos = end_pos + sizeof("</tool_call>") - 1;
        }
        else
        {
            break;
        }
    }  while(true);

    return result;
}

std::string formatToolResponse(const std::string& result) {
    return "\n<tool_response>\n" + result + "\n</tool_response>\n";
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

static auto gettokenize = [](const std::string& prompt,const llama_vocab* vocab)->std::vector<llama_token>{
    if(!vocab)
        return {};

    std::vector<llama_token> tokens;
    tokens.resize(prompt.size() + 3);

    int n_tokens = llama_tokenize(
        vocab,
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
            vocab,
            prompt.c_str(),
            prompt.length(),
            tokens.data(),
            tokens.size(),
            false,
            true
            );
    }

    if(n_tokens!=tokens.size())
        tokens.resize(n_tokens);

    return tokens;
};

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

void LlamaModel::ClearPausedTask()
{
    if(m_inputtask.empty())
        return;

    AITask quetask;
    while(m_inputtask.front(quetask))
    {
        if(quetask.taskstate != TaskState::Normal)
        {
            if(!quetask.output.empty())
            {
                llamaChatMsg newmsg("assistant",quetask.output);
                m_chathistory.emplace_back(newmsg);
            }
            m_inputtask.dequeue(quetask);
        }
    }
    std::string endstr = "<|im_end|>\n";
    auto endtokens = gettokenize(endstr,m_vocab);
    m_curtokens.insert(m_curtokens.end(),endtokens.begin(),endtokens.end());
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
    cparams.n_ctx = global_n_ctx;

    // 3. 加载模型
    m_model = llama_model_load_from_file(model_path.c_str(), mparams);
    if (!m_model) {
        qDebug()<<("错误: 模型加载失败\n");
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

    std::ostringstream oss;
    oss << "<|im_start|>system\n";
    oss << system_template << "\n\n";
    const std::vector<Tool>& tools = ToolExecutor::gettools();
    if(!tools.empty())
    {
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
        oss << "</tool_call>";
    }
    oss << "<|im_end|>\n";

    m_chathistory.emplace_back(
        "system",
        oss.str());


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

bool LlamaModel::processTask(AITask &task)
{

    if(task.taskstate == TaskState::Normal)
    {
        llamaChatMsg msg("user",task.userinput);
        msg.usethinking = task.needthinking;
        m_chathistory.emplace_back(msg);
    }

    int lastoutputpos = task.output.length();

    auto outputchange = [&]()->void{
        int outputpos = task.output.length();
        std::string trimstring(task.output.begin()+lastoutputpos,task.output.begin()+outputpos);

        auto slipttext = [](const std::string& text)-> std::vector<std::string> {
            static std::vector<std::string> keystip = {
                "<think>",
                "</think>",
                "<tool_call>",
                "</tool_call>"
            };

            std::vector<std::string> paragraphs;
            size_t pos = 0;

            while (pos < text.length()) {
                // 查找下一个关键标签的位置
                size_t next_tag_pos = std::string::npos;
                std::string found_tag;

                for (const auto& tag : keystip) {
                    size_t tag_pos = text.find(tag, pos);
                    if (tag_pos != std::string::npos) {
                        if (next_tag_pos == std::string::npos || tag_pos < next_tag_pos) {
                            next_tag_pos = tag_pos;
                            found_tag = tag;
                        }
                    }
                }

                if (next_tag_pos == std::string::npos) {
                    // 没有更多标签，添加剩余部分
                    if (pos < text.length()) {
                        std::string remaining = text.substr(pos);
                        if (!remaining.empty()) {
                            paragraphs.push_back(remaining);
                        }
                    }
                    break;
                }

                // 添加标签前的内容（如果有）
                if (next_tag_pos > pos) {
                    std::string before_tag = text.substr(pos, next_tag_pos - pos);
                    if (!before_tag.empty()) {
                        paragraphs.push_back(before_tag);
                    }
                }

                // 添加标签本身
                paragraphs.push_back(found_tag);

                // 更新位置到标签结束处
                pos = next_tag_pos + found_tag.length();
            }

            return paragraphs;
        };

        std::vector<std::string> texts = slipttext(trimstring);

        for(int i=0; i<texts.size(); i++)
        {
            std::string delta = texts[i];
            uint32_t oristrlen = delta.length();
            if(task.state != GenerateState::thinking && delta.find("<think>") != std::string::npos)
            {
                if(task.laststate == GenerateState::none)
                    delta = "-----------------think start-----------------\n\n";
                else
                    delta = "\n\n-----------------think start-----------------\n\n";
                QString outputdelta = QString::fromStdString(delta);
                emit outputText(outputdelta, 1);

                task.laststate = task.state;
                task.state = GenerateState::thinking;
                task.firstoutputthinkingtext = true;

                lastoutputpos += oristrlen;
                continue;
            }
            if(task.state == GenerateState::thinking && delta.find("</think>") != std::string::npos)
            {
                delta = "\n\n-----------------think over-----------------\n\n";
                QString outputdelta = QString::fromStdString(delta);
                emit outputText(outputdelta, 1);

                task.laststate = task.state;
                task.state = GenerateState::talking;

                lastoutputpos += oristrlen;
                continue;
            }
            if(delta.find("<tool_call>") != std::string::npos)
            {
                if(!task.onusetool)
                    task.onusetool = true;

                if(
                    (task.state == GenerateState::thinking && task.firstoutputthinkingtext) ||
                    (task.state == GenerateState::talking && task.firstoutputtalkingtext)
                    )
                    delta = "------------toolcall start------------\n";
                else
                {
                    delta = "\n------------toolcall start------------\n";
                }
                QString outputdelta = QString::fromStdString(delta);
                emit outputText(outputdelta, 1);

                task.laststate = task.state;
                task.state = GenerateState::toolusing;
                task.firstoutputtoolusingtext = true;

                lastoutputpos += oristrlen;
                continue;
            }
            if(task.state == GenerateState::toolusing && delta.find("</tool_call>") != std::string::npos)
            {
                task.state = task.laststate;
                task.laststate = GenerateState::toolusing;

                delta = "\n------------toolcall over------------\n";
                QString outputdelta = QString::fromStdString(delta);
                emit outputText(outputdelta, 1);

                lastoutputpos += oristrlen;
                continue;
            }

            if(task.state == GenerateState::thinking)
            {
                if(task.firstoutputthinkingtext)
                    delta = trimLeadingWhitespace(delta);
                if(!delta.empty())
                {
                    if(i == texts.size()-1 && !isStringEndsWithCompleteUTF8(delta))
                        break;
                    QString outputdelta = QString::fromStdString(delta);
                    emit outputText(outputdelta, 1);
                    task.firstoutputthinkingtext = false;
                    lastoutputpos += oristrlen;
                }
            }
            else if(task.state == GenerateState::talking)
            {
                if(task.firstoutputtalkingtext)
                    delta = trimLeadingWhitespace(delta);
                if(!delta.empty())
                {
                    if(i == texts.size()-1 && !isStringEndsWithCompleteUTF8(delta))
                        break;
                    QString outputdelta = QString::fromStdString(delta);
                    emit outputText(outputdelta, 1);
                    task.firstoutputtalkingtext = false;
                    lastoutputpos += oristrlen;
                }
            }
            else if(task.state == GenerateState::toolusing)
            {
                if(task.firstoutputtoolusingtext)
                    delta = trimLeadingWhitespace(delta);
                if(!delta.empty())
                {
                    if(i == texts.size()-1 && !isStringEndsWithCompleteUTF8(delta))
                        break;
                    QString outputdelta = QString::fromStdString(delta);
                    emit outputText(outputdelta, 1);
                    task.firstoutputtalkingtext = false;
                    lastoutputpos += oristrlen;
                }
            }
        }
    };

    task.generatecount = 3;
    while(task.generatecount-->0)
    {
        if(generate(task,outputchange))
        {
            // 检查是否有工具调用
            if (task.onusetool)
            {
                // qDebug()<<QString::fromStdString(task.output);
                std::string trimresponse = trimLeadingWhitespaceAndQuotes(task.output);
                if(containsToolCall(trimresponse))
                {
                    llamaChatMsg newmsg("assistant",task.output);
                    m_chathistory.emplace_back(newmsg);
                    task.output.clear();
                    lastoutputpos = 0;

                    trimresponse = stringreplace(trimresponse,"\\n","\n");
                    processToolCall(task,trimresponse);
                    // if (task.generatecount<=1)
                    // {
                    //     task.needthinking = false;
                    //     // 更新prompt，加入工具的回复信息，最后一次输出取消思考，加入prompt确保有内容输出给用户
                    //     task.extraprompt = "基于我的思考，我应该说:";
                    // }
                    // else
                    {
                        if(task.needthinking)
                            task.extraprompt = "接下来我要查询刚刚的工具调用情况，然后执行下一步动作.";
                        else
                            task.extraprompt = "基于刚刚的工具调用情况，我应该说:";
                    }
                }
                task.onusetool = false;
            }
            else
            {
                llamaChatMsg newmsg("assistant",task.output);
                std::string trimresponse = trimLeadingWhitespaceAndQuotes(newmsg.extract_content_without_thinking());
                m_chathistory.emplace_back(newmsg);
                task.output.clear();
                lastoutputpos = 0;
                if(trimresponse!="")
                {
                    QString outputtext = QString::fromStdString(trimresponse);
                    // qDebug()<<outputtext;
                    emit outputText(outputtext, 0);
                    return true;
                }
                // else    // 没有输出实质内容
                // {
                //     task.needthinking = false;
                //     // 取消思考，加入prompt确保有内容输出给用户
                //     task.extraprompt = "基于我的思考，我应该说:";
                // }
            }
        }
        else
        {
            if(task.taskstate != TaskState::Normal)
            {
                task.generatecount++;
                m_inputtask.enqueue(task);
                emit outputText("", 2);
            }
            return true;
        }
        task.state = GenerateState::talking;
    }
    if(task.generatecount <= 0)
    {
        task.taskstate = TaskState::MaxGenCountPause;
        m_inputtask.enqueue(task);
        shouldpause = true;
        emit outputText("", 2);
        return true;
    }
    emit outputText("", 0);
    return false;
}

// 取出下一个将要处理的batch
int LlamaModel::GetBatchToProcess(llama_batch& batch, int ori_batch_size)
{
    if(!m_ctx)
        return -1;

    int newbatchlen = ori_batch_size;

    uint32_t curtokenssize = m_curtokens.size();
    uint32_t newpos = min(m_todecodepos + llama_n_batch(m_ctx) , curtokenssize);

    uint32_t batchlen = newpos - m_todecodepos;
    if(ori_batch_size < batchlen)
    {
        if(ori_batch_size>0)
            llama_batch_free(batch);
        batch = llama_batch_init(batchlen, 0, 1);
        newbatchlen = batchlen;
    }
    else
    {
        batch.n_tokens = 0;
    }

    if (batch.token == nullptr) {
        qDebug() << "Failed to initialize batch";
        return false;
    }

    for (int index = m_todecodepos; index < newpos; index++) {
        batch.token[batch.n_tokens] = m_curtokens[index];
        batch.pos[batch.n_tokens] = index;
        batch.n_seq_id[batch.n_tokens] = 1;
        batch.seq_id[batch.n_tokens][0] = default_seq_id;
        batch.logits[batch.n_tokens] = (index == curtokenssize - 1) ? 1 : 0;  // 最后一个输出logits
        batch.n_tokens++;
    }

    return newbatchlen;
}

// 对新加入的任务做解析，将新的prompt进行token化，加入到要处理的m_curtokens中
bool LlamaModel::preprocess(AITask& task)
{
    if(!m_ctx)
    {
        // ctx不存在时，直接重建prompt
        resetcontext();
        std::string prompt = buildQwenPrompt(true,task.needthinking);
        std::ostringstream oss;
        if(!task.toolsmsg.empty())
            task.toolsmsg.clear();
        if(!task.extraprompt.empty())
        {
            prompt += task.extraprompt;
            task.extraprompt = "";
        }

        m_curtokens = gettokenize(prompt,m_vocab);
        return true;
    }

    // ctx存在时，判断ctx是否达到窗口大小，若达到需要重置
    // 若ctx仍有冗余，则增量处理新的prompt

    int n_ctx = llama_n_ctx(m_ctx);
    int safety_margin = 200;
    int window_size = n_ctx - 1000 - safety_margin;

    auto getaddprmpt = [](AITask& task)->std::string
    {
        std::ostringstream oss;

        for(auto& msg : task.toolsmsg)
        {
            if (msg.role == "user" || msg.role == "tool") {
                oss << "<|im_start|>user\n";
                oss << msg.content;
                oss << "<|im_end|>\n";
            }
        }
        task.toolsmsg.clear();

        oss << "<|im_start|>assistant\n";
        if (!task.needthinking) {
            oss << "<think>\n\n</think>\n\n";
        }
        if(!task.extraprompt.empty())
        {
            if (!task.needthinking)
                oss << task.extraprompt;
            else
            {
                oss << "<think>\n" << task.extraprompt;
                task.output += "<think>\n";
            }
            task.extraprompt = "";
        }

        return oss.str();
    };

    std::string addprompt = getaddprmpt(task);
    std::vector<llama_token> addtokens = gettokenize(addprompt,m_vocab);
    int newtokensuse = addtokens.size() + m_curtokens.size();

    if(newtokensuse < window_size)    //新的token加入仍然不会使得ctx超过上下文限制，直接增量处理
    {
        m_curtokens.insert(m_curtokens.end(),addtokens.begin(),addtokens.end());
    }
    else   //新的token加入导致ctx超过上下文限制，重置ctx，重建prompt
    {
        resetcontext();
        const std::string prompt = buildQwenPrompt(true,task.needthinking);
        m_curtokens = gettokenize(prompt,m_vocab);
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

void LlamaModel::clearcontext()
{
    m_curtokens.clear();
    m_todecodepos = 0;
    if(m_ctx)
    {
        llama_free(m_ctx);
        m_ctx = nullptr;
    }
    if(m_sampler)
    {
        llama_sampler_free(m_sampler);
        m_sampler = nullptr;
    }
}

void LlamaModel::resetcontext()
{
    m_curtokens.clear();
    m_todecodepos = 0;

    if(m_ctx)
    {
        llama_free(m_ctx);
        m_ctx = nullptr;
    }
    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = global_n_ctx;
    m_ctx = llama_init_from_model(m_model, cparams);
    qDebug()<<("resetctx 上下文长度: %d\n", llama_n_ctx((llama_context*)m_ctx));

    if(m_sampler)
    {
        llama_sampler_free(m_sampler);
        m_sampler = nullptr;
    }
    m_sampler = getsampler();
}

bool LlamaModel::generate(AITask& task, std::function<void()> outputchangecallback) {
    if (!m_loaded) return false;

    if(task.taskstate == TaskState::Normal || task.taskstate == TaskState::MaxGenCountPause)
    {
        if(!preprocess(task))
        {
            qDebug()<<"llama preprocess error!";
            return false;
        }
        task.firstoutputthinkingtext = true;
        task.firstoutputtalkingtext = true;
    }
    else /*if(task.taskstate == TaskState::ManualPause || task.taskstate == TaskState::MaxGenTokenPause)*/
    {
        task.current_gen = 0;
    }

    if(task.taskstate != TaskState::Normal)
        task.taskstate = TaskState::Normal;


    // qDebug() << "Starting generate with:";
    // qDebug() << "  n_ctx =" << llama_n_ctx(m_ctx);
    // qDebug() << "  n_batch =" << llama_n_batch(m_ctx);
    // qDebug() << "  prompt length =" << task.prompt.length();

    // 4. 循环解码采样
    bool result = false;
    static int max_gen = 800;  // 安全限制

    llama_batch batch = llama_batch_init(llama_n_batch(m_ctx) , 0, 1);
    if (batch.token == nullptr) {
        qDebug() << "Failed to initialize batch";
        return false;
    }
    int btachlen = llama_n_batch(m_ctx);

    for (; task.current_gen < max_gen; task.current_gen++)
    {
        if(shouldpause)
        {
            result = false;
            task.taskstate = TaskState::ManualPause;
            break;
        }

        btachlen = GetBatchToProcess(batch,btachlen);
        if(btachlen <= 0)
        {
            result = false;
            break;
        }

        // 解码
        int decode_result = llama_decode(m_ctx, batch);
        if (decode_result != 0) {
            qDebug()<<"Error: decode error";
            result = false;
            break;
        }

        m_todecodepos += batch.n_tokens;

        if(batch.logits[batch.n_tokens - 1] == 1)
        {
             // 采样
            task.next_token = llama_sampler_sample(m_sampler, m_ctx, -1);

            bool shouldstop = shouldStopGeneration(task.next_token,task.output);

            llama_sampler_accept(m_sampler, task.next_token);

            m_curtokens.emplace_back(task.next_token);

            if(shouldstop)
            {
                qDebug()<< "StoppingGeneration shouldend";
                result = true;
                break;
            }

            // 反标记化
            char deprompt[100] = {0};
            if(llama_detokenize(m_vocab, &task.next_token, 1, deprompt, sizeof(deprompt), false, true) < 0){
                qDebug()<<"Error: detokenize error";
                result = false;
                break;
            }

            task.output.append(deprompt);

            if(outputchangecallback)
                outputchangecallback();
        }
    }

    if(!result && task.current_gen >= max_gen)
    {
        shouldpause = true;
        task.taskstate = TaskState::MaxGenTokenPause;
    }

    llama_batch_free(batch);

    if (task.taskstate == TaskState::Normal || task.taskstate == TaskState::MaxGenCountPause)
    {
        task.next_token = LLAMA_TOKEN_NULL;
        task.current_gen = 0;
    }

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

        if(!m_inputtask.empty() && !shouldpause)
        {
            AITask task;
            if(m_inputtask.dequeue(task))
            {
                processTask(task);
            }
        }
    }
}

LlamaModel::~LlamaModel() {
    if (m_ctx) {
        llama_free((llama_context*)m_ctx);
        m_ctx = nullptr;
    }
    if(m_sampler)
    {
        llama_sampler_free(m_sampler);
        m_sampler = nullptr;
    }
    if (!m_vocab) {
        m_vocab = nullptr;
    }
    if (m_model) {
        llama_model_free((llama_model*)m_model);
        m_model = nullptr;
    }
    llama_backend_free();
}

bool LlamaModel::isLoaded() const { return m_loaded; }

void LlamaModel::inputText(QString text, bool thinkingEnabled)
{
    if(!m_loaded)
        return;


    if (!m_inputtask.empty())
        ClearPausedTask();

    AITask task;
    task.userinput = text.toStdString();
    task.toolsmsg.emplace_back("user",text.toStdString());
    task.needthinking = thinkingEnabled;
    m_inputtask.enqueue(task);

    shouldpause = false;
    m_queprocesscv.notify_one();
}

std::string LlamaModel::buildQwenPrompt(bool add_generation_prompt,bool enable_thinking) {
    // 构建Qwen ChatML格式的prompt
    std::vector<llamaChatMsg>& messages = m_chathistory;

    if(messages.empty())
        return "";

    std::ostringstream oss;

    auto buildtokens = [&]
        (const std::string& prompt,std::vector<llama_token>& tokens,int& tokensize)
        -> void
    {
        if(!m_vocab)
            return;

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

        if(tokens.size() != n_tokens)
            tokens.resize(n_tokens);

        tokensize = n_tokens;
    };

    for (auto &msg : messages)
    {
        if(msg.tokensize == -1)
        {
            std::string prompt = "<|im_start|>";
            if(msg.role == "system")
            {
                prompt += "system\n";
                prompt += msg.content;
            }
            else if(msg.role == "user" || msg.role == "tool")
            {
                prompt += "user\n";
                prompt += msg.content;
            }
            else if(msg.role == "assistant")
            {
                prompt += "assistant\n";
                if(!msg.usetool)
                    prompt += msg.extract_content_without_thinking();
                else
                    prompt += msg.content;
            }
            prompt += "<|im_end|>\n";
            buildtokens(prompt,msg.tokens,msg.tokensize);
        }
    }

    int n_ctx = llama_n_ctx(m_ctx);          // 模型总容量
    int safety_margin = 200;
    int window_size = min(n_ctx / 2, n_ctx - 1000 - safety_margin);  // 滑动窗口大小（记忆量）

    if(window_size < 0)
        return "";

    int curtokensize = 0;

    // 1. 如果有系统消息，先添加
    if (messages[0].role == "system") {
        oss << "<|im_start|>system\n" << messages[0].content << "<|im_end|>\n";
        curtokensize += messages[0].tokensize;
    }

    // 2. 在模型可容纳的范围内加入历史消息
    int startIndex = 0;
    for (int i = messages.size() - 1; i >= 0; i--) {
        if(i == 0 && messages[i].role == "system")
            break;

        const auto& msg = messages[i];
        if(curtokensize + msg.tokensize > window_size)
        {
            startIndex = i + 1;
            break;  //达到滑动窗口大小，舍弃前面的记忆
        }
        curtokensize += msg.tokensize;
    }

    for (int i = startIndex; i < messages.size(); i++) {
        if(i == 0 && messages[i].role == "system")
            continue;

        const auto& msg = messages[i];
        if (msg.role == "user") {
            // 用户消息
            oss << "<|im_start|>user\n";
            oss << msg.content;
            oss << "<|im_end|>\n";
        } else if (msg.role == "assistant") {
            // 助手消息
            oss << "<|im_start|>assistant\n";
            if(!msg.usetool)
                oss << msg.extract_content_without_thinking();
            else
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


    // 3. 添加生成提示
    if (add_generation_prompt) {
        oss << "<|im_start|>assistant\n";
        if (!enable_thinking) {
            oss << "<think>\n\n</think>\n\n";
        }
    }

    return oss.str();
}

llamaChatMsg::llamaChatMsg(std::string role,
                           std::string content,
                           std::string reasoning_content,
                           int64_t out_time,
                           bool usetool)
    :role(role),content(content),reasoning_content(reasoning_content),out_time(out_time),usetool(usetool),tokensize(-1),usethinking(false){}

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

void LlamaModel::processToolCall(AITask& task, const std::string &string)
{
    // 解析工具调用
    std::vector<ToolCall> tool_calls = parseToolCall(string);

    auto gettoolsoutput = [](std::vector<std::string> results)-> std::string
    {
        std::string str = "\n\n-----------toolcall result-----------\n";
        for(auto &result :results)
        {
            str += result;
            str += "\n";
        }
        str+= "---------toolcall result over---------\n";
        return str;
    };

    std::vector<std::string> results;
    for(auto &tool_call:tool_calls)
    {
        ToolResult tool_result = ToolExecutor::execute(tool_call);
        results.emplace_back(tool_result.result_str);
        std::string tool_response = formatToolResponse(tool_result.result_str);
        int64_t out_time = GetTimestampMilliseconds() + ((int64_t)tool_result.ttl_second) * 1000;
        qDebug()<<"toolcall:" << QString::fromUtf8(tool_call.to_json()) <<"\nresponse"<<QString::fromUtf8(tool_result.result_str);
        m_chathistory.emplace_back("tool", tool_response, "", out_time, true);
        task.toolsmsg.emplace_back("tool", tool_response, "", out_time, true);
    }
    if(!results.empty())
        task.output += gettoolsoutput(results);
}

void LlamaModel::pauseGenerate()
{
    shouldpause = true;
}

void LlamaModel::continueGenerate()
{
    shouldpause = false;
    m_queprocesscv.notify_one();
}

// 清除掉最后一次用户输入以后的历史记录，然后继续生成
void LlamaModel::reGenerate(bool thinkingEnabled)
{
    if(!m_inputtask.empty())
        ClearPausedTask();

    std::string user_input_text;
    bool thinking = false;
    for(int i = m_chathistory.size() - 1; i >= 0; i--)
    {
        if(i == 0)
            break;
        if(m_chathistory[i].role != "user"){
            m_chathistory.pop_back();
        }
        else
        {
            user_input_text = m_chathistory[i].content;
            thinking = thinkingEnabled;
            m_chathistory.pop_back();
            break;
        }
    }

    AITask task;
    task.userinput = user_input_text;
    task.toolsmsg.emplace_back("user",user_input_text);
    task.needthinking = thinking;

    clearcontext();
    m_inputtask.enqueue(task);
    shouldpause = false;
    m_queprocesscv.notify_one();
}

void LlamaModel::clearHistory()
{
    clearcontext();
    m_chathistory.clear();
    AITask quetask;
    m_inputtask.clear();
    shouldpause = false;

    std::ostringstream oss;
    oss << "<|im_start|>system\n";
    oss << system_template << "\n\n";
    const std::vector<Tool>& tools = ToolExecutor::gettools();
    if(!tools.empty())
    {
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
        oss << "</tool_call>";
    }
    oss << "<|im_end|>\n";

    m_chathistory.emplace_back(
        "system",
        oss.str());
}

