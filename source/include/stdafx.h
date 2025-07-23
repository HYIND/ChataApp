#pragma once

#include "Net/include/Core/NetCore.h"
#include "Net/include/Session/CustomWebSocketSession.h"
#include "Net/include/Session/CustomTcpSession.h"
#include "Net/include/Session/SessionListener.h"
#include "fmt/core.h"
#include <iostream>
#include "nlohmann/json.hpp"
#include <string.h>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <string>

using json = nlohmann::json;
