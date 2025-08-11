import QtQuick
import QtQuick.Controls 2.5

Item {
    id: root
    anchors.fill: parent

    property var sessionModel: null
    readonly property bool hasActiveSession: (root.sessionModel
                                              && root.sessionModel.sessionToken) ? true : false

    // 空白状态提示（当sessionToken为空时显示）
    Rectangle {
        visible: !hasActiveSession
        anchors.fill: parent
        color: "#f5f5f5"

        Column {
            anchors.centerIn: parent
            spacing: 20

            // Image {
            //     anchors.horizontalCenter: parent.horizontalCenter
            //     source: "qrc:/images/empty-chat.png"
            //     width: 120
            //     height: 120
            // }
            Text {
                text: "请选择聊天会话"
                font.pixelSize: 18
                color: "#999999"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0
        visible: hasActiveSession

        //上方的群聊名/用户名区域
        Rectangle {
            color: "#00000000"
            id: chatinfoarea

            width: parent.width
            height: 80
            border.color: "#dcdad9"

            Column {
                anchors.margins: 10
                spacing: 10
                anchors.fill: parent
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    // font.bold: true
                    font.pixelSize: 22
                    text: root.sessionModel.sessionTitle
                }
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    // font.bold: true
                    font.pixelSize: 14
                    text: root.sessionModel.sessionSubtitle
                }
            }
        }

        //中间的消息显示
        ListView {
            id: chatmsglist
            width: parent.width
            height: parent.height - editarea.height - chatinfoarea.height
            model: root.sessionModel
            clip: true
            delegate: MessageDelegate {
                width: chatmsglist.width
            }
            Connections {
                target: root.sessionModel
                function onNewMessageAdded() {
                    scrollDelayTimer.stop()
                    scrollDelayTimer.start()
                }
            }
            spacing: 5
            // onCountChanged: {
            //     scrollDelayTimer.stop()
            //     scrollDelayTimer.start()
            // }
            onHeightChanged: {
                chatmsglist.cacheBuffer = Math.max(chatmsglist.height * 3,
                                                   chatmsglist.cacheBuffer)
            }

            cacheBuffer: height * 3

            Timer {
                id: scrollDelayTimer
                interval: 170 // 0.17秒延迟
                onTriggered: {
                    function docheck() {
                        chatmsglist.positionViewAtEnd()
                        // chatmsglist.contentY = Math.max(
                        //             chatmsglist.contentHeight - chatmsglist.height
                        //             + chatmsglist.bottomMargin, 0)
                        if (chatmsglist.contentY > chatmsglist.bottomMargin)
                            chatmsglist.contentY = chatmsglist.contentY + chatmsglist.bottomMargin
                    }
                    var numberEqual = 0
                    var lastContentY = chatmsglist.contentY
                    while (Qt.callLater(docheck)) {
                        if (lastContentY == chatmsglist.contentY) {
                            if (numberEqual >= 10) {
                                break
                            } else {
                                numberEqual++
                            }
                        } else {
                            numberEqual = 0
                        }
                    }
                }
            }
            bottomMargin: 20
        }

        //下方的编辑区域
        Rectangle {
            id: editarea
            color: "#00000000"
            width: parent.width
            height: 220
            border.color: "#dcdad9"

            Column {
                anchors.fill: parent
                spacing: 0

                //发送图片等功能区域
                Rectangle {
                    color: "#00000000"
                    border.color: "#dcdad9"
                    id: textinput_edit
                    height: 40
                    width: parent.width
                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 30
                        spacing: 30
                        ImagePickerButton {
                            height: parent.height
                            width: 80
                            onImageSelected: url => {
                                                 sessionModel.sendPicture(
                                                     sessionModel.sessionToken,
                                                     url)
                                             }
                        }
                        FilePickerButton {
                            height: parent.height
                            width: 80
                            onImageSelected: url => {
                                                 sessionModel.sendFile(
                                                     sessionModel.sessionToken,
                                                     url)
                                             }
                        }
                    }
                }
                //文本输入区域
                Rectangle {
                    color: "#00000000"
                    border.color: "#dcdad9"
                    id: textinput
                    height: editarea.height - textinput_edit.height - textinput_sendarea.height
                    width: parent.width

                    Rectangle {
                        anchors.fill: parent
                        clip: true
                        TextEdit {
                            id: textinput_write
                            anchors.margins: 10
                            anchors.fill: parent
                            focus: true
                            font.pixelSize: 15
                            wrapMode: TextEdit.Wrap

                            Keys.onPressed: event => {
                                                if (event.key === Qt.Key_Return
                                                    || event.key === Qt.Key_Enter) {
                                                    if (event.modifiers & Qt.ControlModifier) {
                                                        // Ctrl+Enter: 插入换行符
                                                        textinput_write.insert(
                                                            textinput_write.cursorPosition,
                                                            "\n")
                                                    } else {
                                                        // 单独按Enter: 触发发送
                                                        sendMessage()
                                                        event.accepted = true // 阻止默认行为
                                                    }
                                                }
                                            }

                            function sendMessage() {
                                if (textinput_write.text.trim() !== "") {
                                    textinput_sendBtn.clicked()
                                }
                            }
                        }

                        //滚动条
                        // ScrollBar {
                        //     id: vbar
                        //     hoverEnabled: true
                        //     active: hovered || pressed //鼠标在文本编辑框内或者按下鼠标显示滚动条
                        //     orientation: Qt.Vertical //垂直方向
                        //     size: frame.height / textEdit.height
                        //     width: 10
                        //     //滚动条位置-右边
                        //     anchors.top: parent.top
                        //     anchors.right: parent.right
                        //     anchors.bottom: parent.bottom
                        //     //滚动条背景
                        //     background: Rectangle {
                        //         color: "green"
                        //     }
                        // }
                    }
                }
                //发送按钮区域
                Rectangle {
                    color: "#00000000"
                    border.color: "#dcdad9"
                    id: textinput_sendarea
                    height: 40
                    width: parent.width
                    Button {
                        id: textinput_sendBtn
                        width: 100
                        height: parent.height
                        x: parent.width - width - 30
                        text: "发送"
                        // anchors.right: parent.right
                        // anchors.horizontalCenter: parent.horizontalCenter
                        onClicked: {
                            if (textinput_write.text.trim() !== "") {
                                sessionModel.sendMessage(
                                            sessionModel.sessionToken,
                                            textinput_write.text)
                                textinput_write.clear()
                            }
                        }
                    }
                }
            }
        }
    }
}
