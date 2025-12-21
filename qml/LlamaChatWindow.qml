import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {

    property var model
    property int state:0    //0空闲 1推理中 2输出中
    property var pendingaimessagebubble:null

    id: mainWindow
    width: chatWindow.width
    height: chatWindow.height
    visible: false
    title: "AI工作台"
    color: "#00000000"
    flags: Qt.FramelessWindowHint

    onVisibleChanged:{
        chatWindow.visible = visible;
    }

    // ==================== 气泡式聊天窗口 ====================
    Item {
        id: chatWindow
        visible: false
        width: chatBubble.width+tailCanvas.width
        height: chatBubble.height
        opacity: 1

        // 位置计算：从AI按钮右侧弹出
        x: 0
        y: 0

        // ===== 气泡尾巴 =====
        Canvas {
            id: tailCanvas
            width: 0
            height: 30
            x: parent.width - width
            y: parent.height - 100 - height

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = "#1abc9c"

                ctx.beginPath()
                ctx.moveTo(0, 0)
                ctx.lineTo(width, height / 2)
                ctx.lineTo(0, height)
                ctx.closePath()
                ctx.fill()
            }
        }


        // ===== 聊天窗口主体 =====
        Rectangle {
            id: chatBubble
            width: 400
            height: 600
            radius: 0
            color: "#2c3e50"
            border.color: "#1abc9c"
            border.width: 3

            // 标题栏
            Rectangle {
                id: header
                width: parent.width
                height: 50
                radius: 0
                color: "#1abc9c"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15

                    // Image {
                    //     source: "qrc:/icon/assets/images/AIChatLogo.png"
                    //     width: 24
                    //     height: 24
                    // }

                    Text {
                        text: "AI助手"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 16
                        Layout.leftMargin: 10
                    }

                    Item { Layout.fillWidth: true }

                    // 关闭按钮
                    Rectangle {
                        width: 30
                        height: 30
                        radius: 15
                        color: "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "×"
                            color: "white"
                            font.pixelSize: 24
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                mainWindow.close()
                            }
                        }
                    }
                }
            }

            Rectangle {
                color:"transparent"
                anchors {
                    top: header.bottom
                    left: parent.left
                    right: parent.right
                    bottom: inputArea.top
                    margins: 10
                }

                // 聊天区域
                Flickable {
                    id: chatFlickable
                    anchors.fill: parent;
                    clip: true
                    contentWidth: width
                    contentHeight: chatColumn.height

                    Column {
                        id: chatColumn
                        width: parent.width - chatscrollbar.width - 5
                        spacing: 10
                    }

                    // 自定义滚动条
                    ScrollBar.vertical: ScrollBar {
                        id:chatscrollbar
                        width: 6
                        policy: ScrollBar.AsNeeded

                        contentItem: Rectangle {
                            implicitWidth: 6
                            radius: 3
                            color: "#1abc9c"
                            z : 10
                            opacity:
                            {
                                if (verticalScrollBar.pressed) {
                                    return 1.0
                                } else if (verticalScrollBar.hovered) {
                                    return 0.8
                                } else {
                                    return 0.6
                                }
                            }
                            Behavior on opacity {
                                NumberAnimation { duration: 100 }
                            }
                        }
                    }

                    onContentHeightChanged: {
                        if(chatFlickable.contentHeight>chatFlickable.height)
                            chatFlickable.contentY = chatFlickable.contentHeight - chatFlickable.height
                    }
                    Behavior on contentY {
                        NumberAnimation {
                            duration: 300
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }

            // 输入区域
            Rectangle {
                id: inputArea
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    margins: 10
                }
                height: 50
                radius: 8
                color: "#34495e"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 5

                    TextField {
                        id: messageInput
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        placeholderText: "输入消息..."
                        color: "white"
                        font.pixelSize: 14
                        background: Rectangle {
                            color: "transparent"
                        }

                        Keys.onReturnPressed: {
                            if (event.modifiers & Qt.ShiftModifier) {
                                // Shift+Enter 换行
                                event.accepted = false
                            } else {
                                // Enter 发送
                                sendMessage()
                                event.accepted = true
                            }
                        }
                    }

                    Button {
                        id: sendButton
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        enabled: messageInput.text.trim().length > 0

                        background: Rectangle {
                            radius: 6
                            color: sendButton.enabled ? "#1abc9c" : "#7f8c8d"
                        }

                        contentItem: Text {
                            text: "➤"
                            color: "white"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: sendMessage()
                    }
                }
            }
        }

        // ===== 动画效果 =====
        PropertyAnimation {
            id: showAnimation
            target: chatWindow
            property: "scale"
            from: 0.5
            to: 1.0
            duration: 200
            easing.type: Easing.OutBack
        }

        PropertyAnimation {
            id: hideAnimation
            target: chatWindow
            property: "scale"
            from: 1.0
            to: 0.5
            duration: 150
            easing.type: Easing.InBack
        }

        onVisibleChanged: {
            if (visible) {
                showAnimation.start()
                messageInput.forceActiveFocus()
            } else {
                hideAnimation.start()
            }
        }
    }

    // ==================== 消息气泡组件 ====================
    Component {
        id: messageBubbleComponent

        Item {
            id:bubbleitem
            width: chatColumn.width
            height: bubble.height

            property bool isAI: true
            property string message: ""

            Row {
                width:parent.width
                leftPadding:7
                rightPadding:7
                spacing: 8
                layoutDirection: isAI ? Qt.LeftToRight : Qt.RightToLeft

                // 头像
                Rectangle {
                    id:profilePicture
                    width: 30
                    height: 30
                    radius: 15
                    color: isAI ? "#1abc9c" : "#3498db"

                    Text {
                        anchors.centerIn: parent
                        text: isAI ? "AI" : "我"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 12
                    }
                }

                // 消息气泡
                Rectangle {
                    id: bubble
                    width: Math.min(messageText.implicitWidth + messageText.font.pixelSize * 2,
                                    parent.width - (parent.leftPadding + parent.rightPadding + parent.spacing + profilePicture.width))
                    height: messageText.implicitHeight + messageText.font.pixelSize * 1.5
                    radius: 8
                    color: isAI ? "#34495e" : "#1abc9c"
                    TextEdit {
                        id: messageText
                        anchors.centerIn: parent
                        anchors.margins: font.pixelSize
                        text: bubbleitem.message
                        font.bold: false
                        font.pixelSize: 14
                        color: "white"
                        width: Math.min(implicitWidth, bubble.width - messageText.font.pixelSize * 2)

                        wrapMode: TextEdit.Wrap

                        // 文本选择功能
                        selectByMouse: true
                        selectByKeyboard: true
                        readOnly: true // 只读模式

                        // 透明背景
                        textMargin: 0

                        // 如果需要自定义选择颜色
                        selectionColor: "lightblue"
                        selectedTextColor: "black"
                    }
                }
            }
        }
    }

    // ==================== 函数 ====================
    function sendMessage() {
        var text = messageInput.text.trim()
        if (text.length === 0) return
        if (state != 0) return

        state = 1
        // 添加用户消息
        var userMsg = messageBubbleComponent.createObject(chatColumn, {
            isAI: false,
            message: text
        })

        pendingaimessagebubble = messageBubbleComponent.createObject(chatColumn, {
                                                                        isAI: true,
                                                                        message: "..."
                                                                    })

        messageInput.text = ""

        doAIRequest(text)
    }

    function doAIRequest(userMessage) {
        model.inputText(userMessage);
    }

    function setTailPostion(tailx,taily){
        x = tailx - mainWindow.width;
        y = taily - mainWindow.height + (mainWindow.height - tailCanvas.y)
    }

    Connections {
        target: model
        function onoutputText(text,isend) {
            if(!isend)
            {
                if(state == 1)
                    pendingaimessagebubble.message = "";

                state = 2
                if(pendingaimessagebubble){
                    pendingaimessagebubble.message = pendingaimessagebubble.message + text;
                }
                else {
                    var aiMsg = messageBubbleComponent.createObject(chatColumn, {
                        isAI: true,
                        message: text
                    })
                    pendingaimessagebubble = aiMsg;
                }
            }else
            {
                if(pendingaimessagebubble)
                     pendingaimessagebubble = null;
                state = 0
            }
        }
    }
}
