#include "LlamaToolHelper.h"
#include <QDebug>
#include <sstream>
#include "ModelManager.h"

std::vector<Tool> available_tools = {
    {"calculator", "Perform calculations between two numbers.", {{"expression", "55+55"}},300},
    {"get_time", "Get current Time.", {},10},
    {"get_user_info","Get information of all current user. You can see who is online, whose IP address, or whose name.",{},30},
    {"send_message_to_user","Send a message to the user whose by username. The message content is text. if you want to send content to other, you can use it",{{"username","username"},{"text","text"}},30}
};

std::string ToolCall::to_json() const {
    std::string json = R"({"name":")" + name + R"(","arguments":{)";
    bool first = true;
    for (const auto& [key, value] : arguments) {
        if (!first) json += ",";
        first = false;
        json += R"(")" + key + R"(":")" + value + R"(")";
    }
    json += "}}";
    return json;
}

ToolCall ToolCall::from_json(const std::string &jsonStr) {
    ToolCall call;
    // 简化的JSON解析，可以使用 nlohmann/json 库更好
    size_t name_pos = jsonStr.find(R"("name":")");
    if (name_pos != std::string::npos) {
        name_pos += 8; // 跳过 "name":"
        size_t name_end = jsonStr.find(R"(")", name_pos);
        call.name = jsonStr.substr(name_pos, name_end - name_pos);
    }

    size_t args_pos = jsonStr.find(R"("arguments":{)");
    if (args_pos != std::string::npos) {
        args_pos += 13; // 跳过 "arguments":{
        std::string args_str = jsonStr.substr(args_pos);
        args_str = args_str.substr(0, args_str.find('}'));

        // 解析参数
        size_t pos = 0;
        while (pos < args_str.length()) {
            size_t key_start = args_str.find(R"(")", pos);
            if (key_start == std::string::npos) break;
            size_t key_end = args_str.find(R"(")", key_start + 1);
            std::string key = args_str.substr(key_start + 1, key_end - key_start - 1);

            size_t value_start = args_str.find(R"(")", key_end + 1);
            if (value_start == std::string::npos) break;
            size_t value_end = args_str.find(R"(")", value_start + 1);
            std::string value = args_str.substr(value_start + 1, value_end - value_start - 1);

            call.arguments[key] = value;
            pos = value_end + 1;
        }
    }
    return call;
}

std::vector<Tool> ToolExecutor::gettools()
{
    return available_tools;
}

ToolResult ToolExecutor::execute(const ToolCall &call) {
    ToolResult result;

    if (call.name == "get_user_info") {
        result.result_str = executeGetUserInfo(call);
    } else if (call.name == "send_message_to_user") {
        result.result_str = executeSendMessage(call);
    } else if (call.name == "get_time") {
        result.result_str = executeGetTime(call);
    } else if (call.name == "calculator") {
        result.result_str = executeCalculator(call);
    } else {
        result.result_str = R"({"error": "Unknown tool: )" + call.name + R"("})";
        return result;
    }

    for(auto &tool : available_tools)
    {
        if(tool.name == call.name)
            result.ttl_second = tool.ttl_second;
    }

    return result;
}

std::string ToolExecutor::executeGetUserInfo(const ToolCall &call) {
    auto userinfos = CHATITEMMODEL->getAllUserInfo();

    if(userinfos.empty())
        return R"({"result": "No any user found!" })";

    std::string result = R"({"result":[)";
    for (int i=0; i < userinfos.size(); i++) {
        if(i!=0)
            result +=",";

        std::string des = R"({)";
        auto &user = userinfos[i];
        des +=
            R"("username":")"
            + user.name.toStdString()
            + R"(",)";
        des +=
            R"("ipaddress":")"
            + user.address.toStdString()
            + R"(",)";
        std::string onlinestr = user.isOnline?"yes":"no";
        des +=
            R"("isonline":")"
            + onlinestr
            + R"(")";
        des += R"(})";

        result+=des;
    }

    result += R"(]})";

    return result;
}

std::string ToolExecutor::executeSendMessage(const ToolCall &call) {
    QString name, content;
    {
        auto it = call.arguments.find("username");
        if (it == call.arguments.end()) {
            return R"({"error": "Missing username"})";
        }
        if(it->second.empty())
        {
            return R"({"error": "Empty username"})";
        }
        name = QString::fromUtf8(it->second);
    }
    {
        auto it = call.arguments.find("text");
        if (it == call.arguments.end()) {
            return R"({"error": "Missing text"})";
        }
        if(it->second.empty())
        {
            return R"({"error": "Empty text"})";
        }
        content = QString::fromUtf8(it->second);
    }

    QString token;
    if(!CHATITEMMODEL->findtokenbyname(name,token))
    {
        return R"({"error": "User not found"})";
    }
    if(!CHATITEMMODEL->isUseronline(token))
    {
        return R"({"error": "User not online"})";
    }

    SESSIONMODEL->sendMessage(token,content);

    return R"({"result": "Send success!" })";
}

std::string ToolExecutor::executeGetTime(const ToolCall &call) {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::string time_str = std::ctime(&now_time);
    time_str.pop_back(); // 移除换行符

    return R"({"result": "Current time:)" + time_str + R"("})";
}

std::string ToolExecutor::executeCalculator(const ToolCall& call) {
    auto it = call.arguments.find("expression");
    if (it == call.arguments.end()) {
        return R"({"error": "Missing expression argument"})";
    }

    // 简单的表达式计算（实际使用时应该用安全的计算库）
    std::string expr = it->second;
    try {
        // 这里使用简单解析，实际应该用数学表达式库
        double result = 0;
        if (expr.find('+') != std::string::npos) {
            size_t pos = expr.find('+');
            double a = std::stod(expr.substr(0, pos));
            double b = std::stod(expr.substr(pos + 1));
            result = a + b;
        } else if (expr.find('-') != std::string::npos) {
            size_t pos = expr.find('-');
            double a = std::stod(expr.substr(0, pos));
            double b = std::stod(expr.substr(pos + 1));
            result = a - b;
        } else if (expr.find('*') != std::string::npos) {
            size_t pos = expr.find('*');
            double a = std::stod(expr.substr(0, pos));
            double b = std::stod(expr.substr(pos + 1));
            result = a * b;
        } else if (expr.find('/') != std::string::npos) {
            size_t pos = expr.find('/');
            double a = std::stod(expr.substr(0, pos));
            double b = std::stod(expr.substr(pos + 1));
            if (b != 0) result = a / b;
            else return R"({"error": "Division by zero"})";
        } else {
            result = std::stod(expr);
        }

        return R"({"result": ")" + std::to_string(result) + R"("})";
    } catch (const std::exception& e) {
        return R"({"error": "Calculation error: )" + std::string(e.what()) + R"("})";
    }
}

std::string Tool::to_json() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"name\": \"" << name << "\", ";
    oss << "\"description\": \"" << description << "\", ";
    oss << "\"parameters\": {";

    bool first = true;
    for (const auto& [key, value] : parameters) {
        if (!first) oss << ", ";
        first = false;
        oss << "\"" << key << "\": \"" << value << "\"";
    }
    oss << "}";
    oss << "}";
    return oss.str();
}
