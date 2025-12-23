import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {

    property var model
    property int state:0    //0空闲 1推理中 2输出中 3用户停止
    property var lastaimessagebubble:null
    property bool thinkingEnabled: false

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

        // 位置计算：从AI按钮左侧弹出
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

                    // ===== 垃圾桶按钮（清除历史对话） =====
                    Rectangle {
                        id: trashButton
                        width: 30
                        height: 30
                        radius: 15
                        color: "transparent"
                        opacity: 0.8

                        // 垃圾桶图标
                        Canvas {
                            anchors.fill: parent
                            anchors.margins: 5

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                ctx.strokeStyle = "white"
                                ctx.lineWidth = 1
                                ctx.fillStyle = "white"
                                ctx.lineCap = "square"

                                // 桶盖顶部横线
                                ctx.beginPath()
                                ctx.moveTo(3, 6)
                                ctx.lineTo(17, 6)
                                ctx.stroke()

                                // 桶盖左侧斜线
                                ctx.beginPath()
                                ctx.moveTo(3, 6)
                                ctx.lineTo(5, 8)
                                ctx.stroke()

                                // 桶盖右侧斜线
                                ctx.beginPath()
                                ctx.moveTo(17, 6)
                                ctx.lineTo(15, 8)
                                ctx.stroke()

                                // 桶身边框左右竖线
                                ctx.beginPath()
                                ctx.moveTo(5, 8)
                                ctx.lineTo(5, 20)
                                ctx.moveTo(15, 8)
                                ctx.lineTo(15, 20)
                                ctx.stroke()

                                // 桶底横线
                                ctx.beginPath()
                                ctx.moveTo(5, 20)
                                ctx.lineTo(15, 20)
                                ctx.stroke()

                                // 桶身竖线（镂空效果）
                                var lineCount = 4 // 竖线数量
                                var startX = 6
                                var endX = 14
                                var spacing = (endX - startX) / (lineCount - 1)

                                for (var i = 0; i < lineCount; i++) {
                                    var xPos = startX + i * spacing
                                    ctx.beginPath()
                                    ctx.moveTo(xPos, 9) // 从桶盖下方开始
                                    ctx.lineTo(xPos, 19) // 到桶底上方结束
                                    ctx.stroke()
                                }

                                // 桶把手
                                ctx.beginPath()
                                ctx.moveTo(8, 4)
                                ctx.lineTo(12, 4)
                                ctx.stroke()

                                // 桶把手竖线
                                ctx.beginPath()
                                ctx.moveTo(10, 4)
                                ctx.lineTo(10, 6)
                                ctx.stroke()
                            }
                        }

                        // 鼠标悬停效果
                        states: [
                            State {
                                name: "hovered"
                                when: trashMouseArea.containsMouse
                                PropertyChanges {
                                    target: trashButton
                                    opacity: 1.0
                                    scale: 1.1
                                }
                            },
                            State {
                                name: "pressed"
                                when: trashMouseArea.pressed
                                PropertyChanges {
                                    target: trashButton
                                    opacity: 0.6
                                    scale: 0.95
                                }
                            }
                        ]

                        transitions: [
                            Transition {
                                to: "*"
                                NumberAnimation { properties: "opacity,scale"; duration: 150 }
                            }
                        ]

                        // 提示文本
                        ToolTip {
                            id: trashToolTip
                            visible: trashMouseArea.containsMouse
                            text: "清除历史对话"
                            delay: 500
                            timeout: 2000
                        }

                        MouseArea {
                            id: trashMouseArea
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true

                            onClicked: {
                                // 弹出确认对话框
                                clearHistory();
                            }
                        }
                    }


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
                                if (chatscrollbar.pressed) {
                                    return 1.0
                                } else if (chatscrollbar.hovered) {
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
                height: 80
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
                        wrapMode: TextEdit.Wrap
                        background: Rectangle {
                            color: "transparent"
                        }

                        Keys.onReturnPressed: {
                            if ((event.modifiers & Qt.ShiftModifier)
                                    || (event.modifiers & Qt.ControlModifier)) {
                                // Shift+Enter 换行
                                event.accepted = false
                            } else {
                                // Enter 发送
                                sendMessage()
                                event.accepted = true
                            }
                        }
                    }

                    // 右侧按钮列
                    Column {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: 40
                        spacing: 8

                        // 滑动开关
                        Switch {
                            id: thinkingSwitch
                            width: 40
                            height: 20
                            checked: false

                            // 自定义样式
                            indicator: Rectangle {
                                implicitWidth: 40
                                implicitHeight: 20
                                radius: 10
                                color: thinkingSwitch.checked ? "#1abc9c" : "#7f8c8d"
                                border.color: thinkingSwitch.checked ? "#16a085" : "#95a5a6"
                                border.width: 2

                                Rectangle {
                                    x: thinkingSwitch.checked ? parent.width - width - 2 : 2
                                    y: 2
                                    width: 16
                                    height: 16
                                    radius: 8
                                    color: "white"
                                    border.color: "#ddd"
                                    border.width: 1

                                    Behavior on x {
                                        NumberAnimation { duration: 150 }
                                    }
                                }
                            }
                            ToolTip {
                                visible: thinkingSwitch.hovered
                                text: thinkingSwitch.checked ? "思考模式已开启" : "思考模式已关闭"
                                delay: 500
                            }
                            onCheckedChanged :
                            {
                                thinkingEnabled = thinkingSwitch.checked;
                            }
                        }

                        // 发送按钮
                        Button {
                            id: sendButton
                            width: 40
                            height: 40
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
            // height: bubble.height
            height: bubble.height + (isAI ? controlRow.height + 10 : 0)


            property bool isAI: true
            property string message: ""

            Column {
                width: parent.width
                spacing: 10
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
                        Column {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10
                            TextEdit {
                                id: messageText
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
                // AI消息底部的控制按钮（只有AI消息才显示）
                Row {
                    id: controlRow
                    visible: isAI
                    width: parent.width
                    leftPadding: 7 + profilePicture.width + 8  // 对齐头像位置
                    rightPadding: 7
                    spacing: 10
                    layoutDirection: Qt.LeftToRight

                    // 控制按钮
                    Rectangle {
                            id: controlpauseButton
                            width: controlpausetips.implicitWidth + 20
                            height: 30
                            radius: 15
                            border.color: "white"
                            border.width: 2
                            color: "transparent"
                            visible: bubbleitem == mainWindow.lastaimessagebubble &&
                                     (mainWindow.state == 1 || mainWindow.state == 2 || mainWindow.state == 3)

                            Text {
                                id: controlpausetips
                                anchors.centerIn: parent
                                text: {
                                    if (mainWindow.state == 3) return qsTr("继续生成")
                                    if (mainWindow.state == 1 || mainWindow.state == 2) return qsTr("停止生成")
                                    return ""
                                }
                                color: "white"
                                font.pixelSize: 12
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (mainWindow.state == 3) {
                                        mainWindow.state = 1
                                        model.continueGenerate()
                                    } else if (mainWindow.state == 1 || mainWindow.state == 2) {
                                        model.pauseGenerate()
                                    }
                                }
                            }
                        }
                    Rectangle {
                        id: controlregenButton
                        width: controlregentips.implicitWidth + 20
                        height: 30
                        radius: 15
                        border.color: "white"
                        border.width: 2
                        color: "transparent"
                        visible: bubbleitem == mainWindow.lastaimessagebubble &&
                                 (mainWindow.state == 0 || mainWindow.state == 3)

                        Text {
                            id: controlregentips
                            anchors.centerIn: parent
                            text: {
                                if (visible) return "重新生成"
                                return ""
                            }
                            color: "white"
                            font.pixelSize: 12
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (visible && lastaimessagebubble)
                                {
                                    lastaimessagebubble.message = "..."
                                    mainWindow.state = 1;
                                    model.reGenerate(thinkingEnabled);
                                }
                            }
                        }
                    }
                    Item { Layout.fillWidth: true } // 占位，让按钮靠右
                }
            }
        }
    }

    // ==================== 函数 ====================
    function sendMessage() {
        var text = messageInput.text.trim()
        if (text.length === 0) return
        if (state != 0 && state != 3) return

        state = 1
        // 添加用户消息
        var userMsg = messageBubbleComponent.createObject(chatColumn, {
            isAI: false,
            message: text
        })

        lastaimessagebubble = messageBubbleComponent.createObject(chatColumn, {
                                                                        isAI: true,
                                                                        message: "..."
                                                                    })

        messageInput.text = ""

        doAIRequest(text)
    }

    function doAIRequest(userMessage) {
        model.inputText(userMessage,thinkingEnabled);
    }

    function setTailPostion(tailx,taily){
        x = tailx - mainWindow.width;
        y = taily - mainWindow.height + (mainWindow.height - tailCanvas.y)
    }

    function clearHistory() {
        if(state != 0 && state != 3)
            return

        model.clearHistory()

        for (var i = chatColumn.children.length - 1; i >= 0; i--) {
            var child = chatColumn.children[i]
            child.destroy()
        }

        // 重置状态
        lastaimessagebubble = null
        state = 0
    }

    Connections {
        target: model
        function onoutputText(text,operation) { //0结束 1输出内容 2暂停
            if(operation == 1)
            {
                if(state == 3)
                    state = 1

                if(state == 1 && lastaimessagebubble.message == "...")
                    lastaimessagebubble.message = "";

                state = 2
                if(lastaimessagebubble){
                    lastaimessagebubble.message = lastaimessagebubble.message + text;
                }
                else {
                    var aiMsg = messageBubbleComponent.createObject(chatColumn, {
                        isAI: true,
                        message: text
                    })
                    lastaimessagebubble = aiMsg;
                }
            }
            else if (operation == 0)
            {
               state = 0
            }
            else if (operation == 2)
            {
               state = 3
            }
        }
    }
}
