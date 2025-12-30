cmake_minimum_required(VERSION 3.16)

project(ChatServer C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../bin)

include_directories(../ThirdParty)
include_directories(../ThirdParty/Net/include)
include_directories(../ThirdParty/publicShare)
include_directories(../ThirdParty/jwt-cpp)
include_directories(../source/include)
# include_directories(${OPENSSL_INCLUDE_DIR})

aux_source_directory(../source/src DIR_SRC)

IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread -lfmt -pthread")
ENDIF()

# 检查库文件是否存在（调试用）
message(STATUS "Checking libnet.so at: ${CMAKE_SOURCE_DIR}/../bin/libnet.so")
message(STATUS "Checking libpublic.so at: ${CMAKE_SOURCE_DIR}/../bin/libpublic.so")

find_package(OpenSSL REQUIRED)

# 定义导入库（绝对路径 + 检查存在性）
find_library(NET_LIB net PATHS ${CMAKE_SOURCE_DIR}/../bin NO_DEFAULT_PATH)
find_library(PUBLIC_LIB public PATHS ${CMAKE_SOURCE_DIR}/../bin NO_DEFAULT_PATH)
find_library(FMT_LIB fmt PATHS /usr/lib /usr/local/lib)
find_library(URING_LIB uring PATHS /usr/lib /usr/local/lib)

if(NOT NET_LIB)
    message(FATAL_ERROR "libnet.so not found in ${CMAKE_SOURCE_DIR}/../bin")
endif()

if(NOT PUBLIC_LIB)
    message(FATAL_ERROR "libpublic.so not found in ${CMAKE_SOURCE_DIR}/../bin")
endif()

if(NOT FMT_LIB)
    message(FATAL_ERROR "fmt library not found!")
endif()

if(NOT URING_LIB)
    message(FATAL_ERROR "uring library not found!")
endif()

add_executable(ChatServer ${DIR_SRC})

target_link_libraries(ChatServer ${FMT_LIB})
target_link_libraries(ChatServer ${NET_LIB})
target_link_libraries(ChatServer ${PUBLIC_LIB})
target_link_libraries(ChatServer ${OPENSSL_LIBRARIES})

if(URING_LIB)
    target_link_libraries(ChatServer ${URING_LIB})
endif()

set(CMAKE_BUILD_TYPE "Debug")
