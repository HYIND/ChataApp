cmake_minimum_required(VERSION 3.16)

project(chat_qml_client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

set(CMAKE_AUTORCC ON)

qt_add_executable(appchat_qml_client
    main.cpp
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
        SOURCES ConnectManager.h
        SOURCES ConnectManager.cpp
        SOURCES QCustomTcpSession.h QCustomTcpSession.cpp
        SOURCES Buffer.h Buffer.cpp
        SOURCES SafeStl.h
        SOURCES SpinLock.h SpinLock.cpp
        SOURCES CriticalSectionLock.h CriticalSectionLock.cpp
        SOURCES QBaseNetWorkSession.h QBaseNetWorkSession.cpp
        SOURCES QTCPClient.h QTCPClient.cpp
        SOURCES RequestManager.h RequestManager.cpp
        SOURCES MsgManager.h MsgManager.cpp
        SOURCES ChatItemListModel.h ChatItemListModel.cpp
        SOURCES ModelManager.h
        SOURCES ModelManager.cpp
        SOURCES ChatSessionModel.h ChatSessionModel.cpp
        SOURCES UserInfoModel.h UserInfoModel.cpp
        SOURCES LoginModel.h LoginModel.cpp
)

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
