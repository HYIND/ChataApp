import QtQuick
import QtQuick.Controls 2.5

Item {
    id: root
    height: profilephoto.height + loadingLoader.height

    property bool isCurrentUser: sessionModel.isMyToken(model.srctoken)

    MouseArea {
        propagateComposedEvents: true
        onClicked: {
            mouse.accepted = false
        }
        onPositionChanged: {
            mouse.accepted = false
        }
    }

    //消息区域
    Row {
        padding: 30
        width: parent.width
        spacing: 8
        layoutDirection: isCurrentUser ? Qt.RightToLeft : Qt.LeftToRight

        //头像区域
        Rectangle {
            id: profilephoto
            width: 40
            height: 40
            radius: 20
            color: "#7cdcfe"

            Text {
                text: model.name.substring(0, 1).toUpperCase()
                color: "white"
                font.bold: true
                font.pixelSize: 16
                anchors.centerIn: parent
            }
        }

        //消息区域
        Column {
            spacing: 2

            //发送者时间和名称
            Row {
                spacing: 8
                Text {
                    text: model.name
                    font.bold: true
                    font.pixelSize: 12
                }
                Text {
                    text: Qt.formatDateTime(model.time, "hh:mm:ss")
                    font.bold: true
                    font.pixelSize: 12
                }
            }

            Item {
                width: parent.width
                height: loadingLoader.height

                Component {
                    id: bubble_picture
                    //消息气泡(图片)
                    Rectangle {
                        id: messagebubble_picture
                        visible: model.type == 2
                        width: imgPreview.width + 20
                        height: imgPreview.height + 16
                        radius: 8
                        color: "#7cdcfe"

                        anchors.right: isCurrentUser ? parent.right : undefined
                        anchors.left: isCurrentUser ? undefined : parent.left

                        Image {
                            id: imgPreview
                            width: Math.min(imgPreview.sourceSize.width + 20,
                                            root.width * 0.7, 380)
                            height: width * (imgPreview.sourceSize.height
                                             / imgPreview.sourceSize.width)
                            anchors.centerIn: parent
                            fillMode: Image.PreserveAspectFit
                            source: loadBase64(model.msg)
                            asynchronous: true
                            // 通过函数更新图片
                            function loadBase64(base64Data) {
                                return "data:image/png;base64," + base64Data
                            }
                        }

                        // 加载状态指示
                        BusyIndicator {
                            anchors.centerIn: parent
                            running: parent.status === Image.Loading
                        }
                    }
                }
                Component {
                    id: bubble_text
                    //消息气泡(文本)
                    Rectangle {
                        id: messagebubble_text
                        visible: model.type != 2
                        width: messageText.width + 20
                        height: messageText.height + 16
                        radius: 8
                        color: "#7cdcfe"

                        anchors.right: isCurrentUser ? parent.right : undefined
                        anchors.left: isCurrentUser ? undefined : parent.left

                        Text {
                            id: messageText
                            width: Math.min(messageText.implicitWidth,
                                            root.width * 0.7, 380)
                            height: messageText.implicitHeight
                            anchors.centerIn: parent
                            text: model.msg
                            wrapMode: Text.Wrap
                        }
                    }
                }

                // 加载器控制
                Loader {
                    id: loadingLoader
                    anchors.right: isCurrentUser ? parent.right : undefined
                    anchors.left: isCurrentUser ? undefined : parent.left
                    sourceComponent: model.type == 2 ? bubble_picture : bubble_text
                    active: true
                }
            }
        }
    }
}
