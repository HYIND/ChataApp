#pragma once

#include <iostream>
#include <map>
#include <vector>

struct Tool {
    std::string name;
    std::string description;
    std::map<std::string, std::string> parameters;
    uint32_t ttl_second = UINT32_MAX;

    std::string to_json() const;
};

struct ToolCall {
    std::string name;
    std::map<std::string, std::string> arguments;

    std::string to_json() const;
    static ToolCall from_json(const std::string& jsonStr);
};

struct ToolResult {
    std::string result_str;
    uint32_t ttl_second = 300;
};

class ToolExecutor {

public:
    static std::vector<Tool> gettools();
    static ToolResult execute(const ToolCall& call);

private:
    static std::string executeGetTime(const ToolCall& call);
    static std::string executeGetUserInfo(const ToolCall& call);
    static std::string executeSendMessage(const ToolCall& call);
    static std::string executeCalculator(const ToolCall& call);
};
