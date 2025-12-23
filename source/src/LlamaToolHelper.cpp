#include "LlamaToolHelper.h"
#include <QDebug>
#include <sstream>
#include "ModelManager.h"
#include "nlohmann/json.hpp"



std::vector<Tool> available_tools = {
    {"calculator",
     "Perform calculations between two number. "
     "number1 is the first number. "
     "number2 is the second number. "
     "all number must be enclosed in double quotes. operation can be one of +-*/ operator.",
     {{"number1","number"},{"number2","number"},{"operation","operators"}},
     300},
    {"get_time", "Get current Time.", {},7},
    {"get_user_info","Get information of all current user. You can see who is online, whose IP address, or whose name.",{},15},
    {"send_message_to","Send a message to receiver. The message content is text and the name is name of receiver. if you want to send content to other, you can use it",{{"receiver","name"},{"text","text"}},20}
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

    try {
        auto j = nlohmann::json::parse(jsonStr);

        // 解析 name
        if (j.contains("name") && j["name"].is_string()) {
            call.name = j["name"].get<std::string>();
        }

        // 解析 arguments
        if (j.contains("arguments") && j["arguments"].is_object()) {
            auto args_obj = j["arguments"];
            for (auto it = args_obj.begin(); it != args_obj.end(); ++it) {
                if (it.value().is_string()) {
                    call.arguments[it.key()] = it.value().get<std::string>();
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "ToolCall::from_json JSON parse error: " << e.what() << std::endl;
        return ToolCall();
    }

    return call;
}

std::vector<Tool> ToolExecutor::gettools()
{
    return available_tools;
}

ToolResult ToolExecutor::execute(const ToolCall &call) {
    ToolResult result;
    std::string callresult;
    if (call.name == "get_user_info") {
        callresult = executeGetUserInfo(call);
    } else if (call.name == "send_message_to") {
        callresult = executeSendMessage(call);
    } else if (call.name == "get_time") {
        callresult = executeGetTime(call);
    } else if (call.name == "calculator") {
        callresult = executeCalculator(call);
    } else {
       callresult = R"({"error": "Unknown tool: )" + call.name + R"("})";
    }

    result.result_str=  R"({"toolresult": {"callname":"name","callresult":)" + callresult +R"(}})";

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
        auto it = call.arguments.find("receiver");
        if (it == call.arguments.end()) {
            return R"({"error": "Missing receiver name"})";
        }
        if(it->second.empty())
        {
            return R"({"error": "Empty receiver name "})";
        }
        name = QString::fromStdString(it->second);
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
        content = QString::fromStdString(it->second);
    }

    QString token;
    if(!CHATITEMMODEL->findtokenbyname(name,token))
    {
        return R"({"error": "receiver not found"})";
    }
    if(!CHATITEMMODEL->isUseronline(token))
    {
        return R"({"error": "receiver not online"})";
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

// std::string ToolExecutor::executeCalculator(const ToolCall& call) {
//     auto it = call.arguments.find("expression");
//     if (it == call.arguments.end()) {
//         return R"({"error": "Missing expression argument"})";
//     }

//     // 简单的表达式计算（实际使用时应该用安全的计算库）
//     std::string expr = it->second;
//     try {
//         // 这里使用简单解析，实际应该用数学表达式库
//         double result = 0;
//         if (expr.find('+') != std::string::npos) {
//             size_t pos = expr.find('+');
//             double a = std::stod(expr.substr(0, pos));
//             double b = std::stod(expr.substr(pos + 1));
//             result = a + b;
//         } else if (expr.find('-') != std::string::npos) {
//             size_t pos = expr.find('-');
//             double a = std::stod(expr.substr(0, pos));
//             double b = std::stod(expr.substr(pos + 1));
//             result = a - b;
//         } else if (expr.find('*') != std::string::npos) {
//             size_t pos = expr.find('*');
//             double a = std::stod(expr.substr(0, pos));
//             double b = std::stod(expr.substr(pos + 1));
//             result = a * b;
//         } else if (expr.find('/') != std::string::npos) {
//             size_t pos = expr.find('/');
//             double a = std::stod(expr.substr(0, pos));
//             double b = std::stod(expr.substr(pos + 1));
//             if (b != 0) result = a / b;
//             else return R"({"error": "Division by zero"})";
//         } else {
//             result = std::stod(expr);
//         }

//         return R"({"result": ")" + std::to_string(result) + R"("})";
//     } catch (const std::exception& e) {
//         return R"({"error": "Calculation error: )" + std::string(e.what()) + R"("})";
//     }
// }

std::string ToolExecutor::executeCalculator(const ToolCall& call) {
    std::string strnumber1,strnumber2,operation;
    {
        auto it = call.arguments.find("number1");
        if (it == call.arguments.end()) {
            return R"({"error": "Missing number1 argument"})";
        }
        strnumber1 = it->second;
    }
    {
        auto it = call.arguments.find("number2");
        if (it == call.arguments.end()) {
            return R"({"error": "Missing number2 argument"})";
        }
        strnumber2 = it->second;
    }
    {
        auto it = call.arguments.find("operation");
        if (it == call.arguments.end()) {
            return R"({"error": "Missing operation argument"})";
        }
        operation = it->second;
    }

    // 简单的表达式计算（实际使用时应该用安全的计算库）
    try {
        // 这里使用简单解析，实际应该用数学表达式库
        double result = 0;
        double a = std::stod(strnumber1);
        double b = std::stod(strnumber2);
        if (operation.find('+') != std::string::npos) {
            result = a + b;
        } else if (operation.find('-') != std::string::npos) {
            result = a - b;
        } else if (operation.find('*') != std::string::npos) {
            result = a * b;
        } else if (operation.find('/') != std::string::npos) {
            if (b != 0) result = a / b;
            else return R"({"error": "Division by zero"})";
        } else {
           return R"({"error": "Calculation error: unknown error!"})";
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
