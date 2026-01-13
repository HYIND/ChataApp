import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    property var sessionModel: null
    property var aisummaryModel: null
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
    Rectangle{
        anchors.fill: parent
        visible: hasActiveSession
        color: "#00000000"
        
        DropArea {

            // 从URL中去除前面的file://
            function removePrefixFromUrl(url) {
                var stringurl = url.toString()
                 if (stringurl.startsWith("file:///")) {
                     return stringurl.substring(8)
                 } else if (stringurl.startsWith("file://")) {
                     return stringurl.substring(7)
                 }
                 return stringurl
            }
            function isImageFile(filePath) {
                var imageExtensions = ['.png', '.jpg', '.jpeg', '.bmp', '.gif', '.webp', '.svg']
                var lowerPath = String(filePath).toLowerCase()
                return imageExtensions.some(ext => lowerPath.endsWith(ext))
            }

            anchors.fill: parent

            onEntered: {
                console.log("有内容拖入区域")
            }
            onExited: {
                console.log("内容拖出区域")
            }

            function processFilesAsync(urls){
                for (var i = 0; i < urls.length; i++) {
                    var filePath = removePrefixFromUrl(urls[i].toString())                    
                    if (isImageFile(filePath)) {
                        sessionModel.sendPicture(sessionModel.sessionToken, filePath)
                    } else {
                        sessionModel.sendFile(sessionModel.sessionToken, filePath)
                    }
                }
            }

            // 当在区域内释放时触发
            onDropped: drop => {
                console.log("文件被释放")
                // 处理拖放的文件
                 if (drop.hasUrls) {
                    processFilesAsync(drop.urls)
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
                // AI总结按钮
                Button {
                    id: summaryButton
                    visible: aisummaryModel.isLoaded()
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 20
                    width: 70
                    height: 30

                    background: Rectangle {
                        color: "#1abc9c"
                        radius: 5
                    }

                    contentItem: Text {
                        text: "AI总结"
                        color: "black"
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        anchors.fill: parent
                    }

                    // 点击事件
                    onClicked: {
                        var state = aisummaryModel.getSummaryState(sessionModel.sessionToken);
                        if(state != 0)
                        {
                            summaryButton.contentItem.text = "处理中..."
                            aisummaryModel.addSummaryTask(sessionModel.sessionToken);
                            ToolTip.text = "AI总结中...";
                        }
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: "点击查看聊天内容总结"
                    ToolTip.delay: 300

                    Connections {
                        target: sessionModel
                        function onsessionTokenChanged() {
                            var state = aisummaryModel.getSummaryState(sessionModel.sessionToken);
                            if(state == -1)
                            {
                                summaryButton.contentItem.text = "AI总结"
                                summaryButton.ToolTip.text = "点击总结聊天内容总结"
                            }
                            else if (state == 0)
                            {
                                summaryButton.contentItem.text = "处理中..."
                                summaryButton.ToolTip.text = "AI总结中..."
                            }
                            else if (state == 1)
                            {
                                summaryButton.contentItem.text = "处理完成"
                                summaryButton.ToolTip.text = aisummaryModel.getSummaryText(sessionModel.sessionToken);
                            }
                            else if (state == 2)
                            {
                                summaryButton.contentItem.text = "处理失败"
                                summaryButton.ToolTip.text = "AI总结失败！"
                            }
                        }
                    }
                    Connections {
                        target: aisummaryModel
                        function onsummaryDone(sessiontoken,summarydata) {
                            if(sessiontoken == sessionModel.sessionToken)
                            {
                                summaryButton.contentItem.text = "查看总结"
                                summaryButton.ToolTip.text = summarydata;
                            }
                        }
                    }
                }
            }

            Rectangle {
                id:chatmsglistarea
                color:"transparent"
                anchors {
                    top: chatinfoarea.bottom
                    left: parent.left
                    right: parent.right
                    bottom: editarea.top
                    margins: 8
                }
                //中间的消息显示
                ListView {
                    id: chatmsglist
                    anchors.fill: parent
                    // width: parent.width
                    // height: parent.height
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
                    ScrollBar.vertical: ScrollBar {
                        id: verticalScrollBar
                        width: 6
                        policy: ScrollBar.AsNeeded

                        background: Rectangle {
                            color: "transparent"
                        }

                        contentItem: Rectangle {
                            implicitWidth: 6
                            radius: 3
                            z : 10
                            color: "#1abc9c"

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
                }
            }

            //下方的编辑区域
            Rectangle {
                id: editarea
                color: "#00000000"
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
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
}
