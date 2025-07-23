cmake_minimum_required(VERSION 3.16)

project(chat_qml_client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

set(CMAKE_AUTORCC ON)

qt_add_executable(appchat_qml_client
    source/main.cpp
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
        qml/ChatPage.qml
    RESOURCES
        resource.qrc
        SOURCES source/ConnectManager.h
        SOURCES source/ConnectManager.cpp
        SOURCES source/QCustomTcpSession.h source/QCustomTcpSession.cpp
        SOURCES source/Buffer.h source/Buffer.cpp
        SOURCES source/SafeStl.h
        SOURCES source/SpinLock.h source/SpinLock.cpp
        SOURCES source/CriticalSectionLock.h source/CriticalSectionLock.cpp
        SOURCES source/QBaseNetWorkSession.h source/QBaseNetWorkSession.cpp
        SOURCES source/QTCPClient.h source/QTCPClient.cpp
        SOURCES source/RequestManager.h source/RequestManager.cpp
        SOURCES source/MsgManager.h source/MsgManager.cpp
        SOURCES source/ChatItemListModel.h source/ChatItemListModel.cpp
        SOURCES source/ModelManager.h
        SOURCES source/ModelManager.cpp
        SOURCES source/ChatSessionModel.h source/ChatSessionModel.cpp
        SOURCES source/UserInfoModel.h source/UserInfoModel.cpp
        SOURCES source/LoginModel.h source/LoginModel.cpp
)

include_directories(source/)

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

include(GNUInstallDirs)
install(TARGETS appchat_qml_client
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
