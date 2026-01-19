cmake_minimum_required(VERSION 3.16)

project(chat_qml_client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 REQUIRED COMPONENTS Quick)

# 定义所有源文件
set(APP_SOURCES
    source/src/main.cpp
    source/src/ConnectManager.cpp
    source/src/RequestManager.cpp
    source/src/MsgManager.cpp
    source/src/ChatItemListModel.cpp
    source/src/ModelManager.cpp
    source/src/ChatSessionModel.cpp
    source/src/UserInfoModel.cpp
    source/src/LoginModel.cpp
    source/src/MessagePackage.cpp
    source/src/NetWorkHelper.cpp
    source/src/3DModelViewerModel.cpp
    source/src/LlamaModel.cpp
    source/src/LlamaToolHelper.cpp
    source/src/LlamaClient.cpp
    source/src/AIAssistantModel.cpp
    source/src/AISummaryModel.cpp
    source/src/OCRModel.cpp
    source/src/OCRProcess.cpp


    source/network/QCustomTcpSession.cpp
    source/network/Buffer.cpp
    source/network/SpinLock.cpp
    source/network/CriticalSectionLock.cpp
    source/network/QBaseNetWorkSession.cpp
    source/network/QTCPClient.cpp
    source/network/FileTransManager.cpp
    source/network/FileTransferTask.cpp
    source/network/FileTransferDownLoadTask.cpp
    source/network/FileTransferUpLoadTask.cpp
    source/network/FileIOHandler.cpp
    source/network/CRC32Helper.cpp
    source/network/MD5Helper.cpp
    source/network/AsyncMD5.cpp
    source/network/ThreadPool.cpp
)

set(APP_INCLUDES
    source/include/stdafx.h
    source/include/ConnectManager.h
    source/include/ChatItemListModel.h
    source/include/ModelManager.h
    source/include/ChatSessionModel.h
    source/include/UserInfoModel.h
    source/include/LoginModel.h
    source/include/MessagePackage.h
    source/include/NetWorkHelper.h
    source/include/3DModelViewerModel.h
    source/include/LlamaModel.h
    source/include/LlamaToolHelper.h
    source/include/LlamaClient.h
    source/include/AIAssistantModel.h
    source/include/AISummaryModel.h
    source/include/OCRModel.h
    source/include/OCRProcess.h


    source/network/QCustomTcpSession.h
    source/network/Buffer.h
    source/network/SpinLock.h
    source/network/CriticalSectionLock.h
    source/network/QBaseNetWorkSession.h
    source/network/QTCPClient.h
    source/network/FileTransManager.h
    source/network/FileTransferTask.h
    source/network/FileTransferDownLoadTask.h
    source/network/FileTransferUpLoadTask.h
    source/network/FileIOHandler.h
    source/network/SafeStl.h
    source/network/CRC32Helper.h
    source/network/MD5Helper.h
    source/network/AsyncMD5.h
    source/network/ThreadPool.h
)

qt_add_executable(appchat_qml_client
    ${APP_INCLUDES}
    ${APP_SOURCES}
)

qt_add_qml_module(appchat_qml_client
    URI chat_qml_client
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ChatItemDelegate.qml
        qml/MessageDelegate.qml
        qml/ChatView.qml
        qml/UserInfoDelegate.qml
        qml/LoadPage.qml
        qml/ImagePickerButton.qml
        qml/FilePickerButton.qml
        qml/ChatPage.qml
        qml/ImgPreviewWindow.qml
        qml/3DModelViewer.qml
        qml/LlamaChatWindow.qml
        qml/TipPopup.qml
        qml/videoPreviewWindow.qml
    RESOURCES
        resource.qrc
)

include_directories(source/include source/network ThirdParty/ ThirdParty/llama.cpp)
include_directories(source/include source/network ThirdParty/ ThirdParty/tesseract)

target_include_directories(appchat_qml_client
    PRIVATE
        source/include
        source/network
        ThirdParty/
        ThirdParty/llama.cpp
)

find_package(Qt6 COMPONENTS Core5Compat REQUIRED)
target_link_libraries(appchat_qml_client PUBLIC Qt6::Core5Compat)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appchat_qml_client PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appchat_qml_client
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# find_package(Qt6 REQUIRED COMPONENTS Network)
# target_link_libraries(mytarget PRIVATE Qt6::Network)

target_link_libraries(appchat_qml_client
    PRIVATE Qt6::Quick
)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
target_link_libraries(appchat_qml_client PRIVATE Qt6::Widgets)

find_package(Qt6 REQUIRED COMPONENTS 3DCore)
target_link_libraries(appchat_qml_client PRIVATE Qt6::3DCore)

# find_package(Qt6 REQUIRED COMPONENTS 3drender)
# target_link_libraries(appchat_qml_client PRIVATE Qt6::3drender)

find_package(Qt6 REQUIRED COMPONENTS 3DExtras)
target_link_libraries(appchat_qml_client PRIVATE Qt6::3DExtras)

include(GNUInstallDirs)
install(TARGETS appchat_qml_client
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_link_libraries(appchat_qml_client PRIVATE ${CMAKE_SOURCE_DIR}/lib/llama.lib)
target_link_libraries(appchat_qml_client PRIVATE ${CMAKE_SOURCE_DIR}/lib/tesseract55.lib)
target_link_libraries(appchat_qml_client PRIVATE ${CMAKE_SOURCE_DIR}/lib/leptonica-1.87.0.lib)



