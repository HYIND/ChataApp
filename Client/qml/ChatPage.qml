import QtQuick
import QtQuick.Controls

Item {
    id:root

    property point windowPos

    TipPopup {
        id: tipPopup
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.bottomMargin: 50
        showtime:1000
    }

    // MouseArea{
    //     anchors.fill: parent
    //     propagateComposedEvents: true
    //     onClicked: (mouse)=>{
    //         console.log("item clicked")
    //         mouse.accepted = false
    //     }
    //     onPositionChanged: (mouse)=>{
    //         mouse.accepted = false
    //     }
    // }

    // 监听传入的位置变化
    onWindowPosChanged: {
        var point = aichatmousearea.mapToItem(null, 0, 0)
        var mappoint = Qt.point(windowPos.x, point.y+windowPos.y)
        aichatwindow.setTailPostion(mappoint.x,mappoint.y)
    }

    Row {
        id:mainContent
        visible: true
        width: parent.width
        height: parent.height
        Column {
            id: leftArea
            width: toolarea.width + userarea.width
            height: parent.height
            //定义左上角的用户信息栏
            Rectangle {
                id: userinfo
                width: parent.width
                height: 80
                color: "#f2f2f2"
                border.color: "#dcdad9"
                UserInfoDelegate {
                    anchors.fill: parent
                    anchors.leftMargin:10
                    model: userinfomodel
                }
            }

            Row {
                width: parent.width
                height: parent.height - userinfo.height
                //定义左边工具栏
                Rectangle {
                    id: toolarea
                    width: 80
                    height: parent.height
                    color: '#f2f2f2'

                    //定义工具栏上边的社交类按钮
                    Rectangle {
                        color: "#00000000"
                        y: 50
                        anchors.horizontalCenter: parent.horizontalCenter
                        // anchors.top: parent.top
                        width: parent.width
                        height: 200

                        Column {
                            anchors.fill: parent
                            spacing: 8
                            // Button {
                            //     anchors.horizontalCenter: parent.horizontalCenter
                            //     // anchors.top: parent.top
                            //     width: parent.width - 18
                            //     height: 50
                            // }
                            // Button {
                            //     anchors.horizontalCenter: parent.horizontalCenter
                            //     // anchors.top: parent.top
                            //     width: parent.width - 18
                            //     height: 50
                            // }
                        }
                    }

                    //定义工具栏下边的功能类按钮
                    Rectangle {
                        color: "#00000000"
                        y: parent.height - height - 20
                        anchors.horizontalCenter: parent.horizontalCenter
                        // anchors.bottom: parent.bottom
                        width: parent.width
                        height: 200

                        Column {
                            anchors.fill: parent
                            spacing: 8

                            Rectangle {
                                id: aiToolButton
                                width: parent.width - 18
                                height: 50
                                radius: 5
                                color: aichatwindow.visible ? "#1abc9c" : toolarea.color
                                anchors.horizontalCenter: parent.horizontalCenter

                                Image {
                                    anchors.centerIn: parent
                                    source: "qrc:/icon/assets/images/AIChatLogo.png"
                                    width: 40
                                    height: 40
                                    sourceSize: Qt.size(40, 40)
                                }

                                // 提示文本
                                // Text {
                                //     anchors {
                                //         top: parent.bottom
                                //         horizontalCenter: parent.horizontalCenter
                                //         topMargin: 5
                                //     }
                                //     text: "AI助手"
                                //     color: "white"
                                //     font.pixelSize: 10
                                // }

                                MouseArea {
                                    id: aichatmousearea
                                    LlamaChatWindow {
                                        id: aichatwindow
                                        onVisibleChanged:{
                                            if(!visible){
                                                aichatmousearea.onExited()
                                            }
                                        }
                                        model:aiassistantmodel
                                    }

                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if(aiassistantmodel.isLoaded()) {
                                            root.onWindowPosChanged()
                                            aichatwindow.visible = !aichatwindow.visible
                                        }
                                        else {
                                            tipPopup.show("AI功能暂不可用哦！")
                                        }
                                    }

                                    onEntered: {
                                        if (!aichatwindow.visible) {
                                            aiToolButton.color = "#1abc9c"
                                        }
                                    }
                                    onExited: {
                                        if (!aichatwindow.visible) {
                                            aiToolButton.color = toolarea.color
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                //定义中间的用户列表
                Rectangle {
                    id: userarea

                    width: 260
                    height: parent.height
                    anchors.left: toolarea.right
                    border.color: "#dcdad9"

                    //搜索栏目
                    // Rectangle {
                    //     id: seracharea

                    //     width: parent.width
                    //     height: 80
                    //     anchors.horizontalCenter: parent.horizontalCenter
                    //     anchors.top: parent.top
                    //     border.color: "#dcdad9"
                    // }

                    //用户列表
                    Rectangle {
                        color: "#e5e5e5"
                        id: chatlistarea

                        width: parent.width
                        height: parent.height - userarea.height
                        anchors.fill: parent
                        border.color: "#dcdad9"

                        ListView {
                            // anchors.margins:10
                            anchors.fill: parent
                            id: chatlist
                            clip: true
                            model: chatitemlistmodel
                            boundsBehavior: Flickable.StopAtBounds
                            currentIndex: -1

                            Connections {
                                target: chatitemlistmodel
                                function onsetCurrentIndex(index) {
                                    if (chatlist.currentIndex != index) {
                                        chatlist.currentIndex = index
                                        chatlist.positionViewAtIndex(
                                                    index, ListView.Contain)
                                    }
                                }
                            }

                            signal itemClicked(string token)

                            delegate: ChatItemDelegate {
                                width: chatlist.width
                                isSelected: chatlist.currentIndex === index

                                onClicked: {
                                    chatlist.currentIndex = index
                                    chatlist.itemClicked(model.token)
                                }
                            }

                            ScrollBar.vertical: ScrollBar {
                                policy: ScrollBar.AsNeeded
                            }
                            onItemClicked: token => {
                                               chatitemlistmodel.handleItemClicked(
                                                   token)
                                           }

                            // // 当前选中项高亮
                            // highlight: Rectangle {
                            //     color: "#c6c5c4"
                            // }
                            // highlightMoveDuration: 200
                        }
                    }
                }
            }
        }
        //定义右边的聊天区域
        Rectangle {
            id: chatarea

            color: "#f5f5f5"
            width: parent.width - leftArea.width
            height: parent.height
            border.color: "#dcdad9"

            Column {
                width: parent.width
                height: parent.height
                spacing: 0
                //中间的消息区域
                ChatView {
                    id: messageDisplay
                    width: parent.width
                    height: parent.height
                    sessionModel: sessionmodel
                    aisummaryModel: aisummarymodel
                }
            }
        }
    }
    Connections {
        target: chatitemlistmodel
        function onmsgError(index) {
            tipPopup.show("对方不在线哦！")
        }
    }
}
