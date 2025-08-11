import QtQuick
import QtQuick.Controls

Item {
    id:root

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
                //定义左边黑色工具栏
                Rectangle {
                    id: toolarea
                    width: 80
                    height: parent.height
                    color: '#2e2e2e'

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

                    //定义工具栏下边的设置类按钮
                    Rectangle {
                        color: "#00000000"
                        y: parent.height - height - 20
                        anchors.horizontalCenter: parent.horizontalCenter
                        // anchors.bottom: parent.bottom
                        width: parent.width
                        height: 200

                        // Button {
                        //     anchors.horizontalCenter: parent.horizontalCenter
                        //     anchors.bottom: parent.bottom
                        //     width: parent.width - 18
                        //     height: 50
                        // }
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
                }
            }
        }
    }
}
