import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    height: 72

    property bool isSelected: false
    signal clicked

    Rectangle {
        anchors.fill: parent
        // anchors.margins: 4
        radius: 4
        color: isSelected ? "#e3f2fd" : (mouseArea.containsMouse ? "#f0f0f0" : "#00000000")

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            // 用户头像
            Rectangle {
                id: avatar
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36
                radius: width / 2
                color: "#4a6da7"

                Text {
                    anchors.centerIn: parent
                    text: model.name.substring(0, 1).toUpperCase()
                    color: "white"
                    font.bold: true
                    font.pixelSize: 18
                }

                // 在线状态指示器
                Rectangle {
                    width: 12
                    height: 12
                    radius: width / 2
                    color: model.isonline ? "#4CAF50" : "##cdcdcd"
                    border.color: "white"
                    border.width: 2
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    visible: true
                }
                // 未读消息指示器
                Rectangle {
                    width: 12
                    height: 12
                    radius: width / 2
                    color: "red"
                    anchors.right: parent.right
                    anchors.top: parent.top
                    visible: model.hasunread
                    // Text {
                    //     anchors.centerIn: parent
                    //     text: "2"
                    //     color: "white"
                    //     font.pixelSize: 12
                    // }
                }
            }

            // 用户信息
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 3
                clip: true

                Text {
                    text: model.name
                    font.bold: true
                    font.pixelSize: 16
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    clip: true
                    maximumLineCount: 1  // 最大行数限制
                }

                Text {
                    text: model.address
                    color: "#666666"
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    clip: true
                    maximumLineCount: 1  // 最大行数限制
                }
                Text {
                    function getLastMsgText(token, username, msg) {
                        if (msg == "" || msg == undefined)
                            return ""
                        if (chatitemlistmodel.isPublicChatItem(model.token)) {
                            return username + ":" + msg
                        } else {
                            return msg
                        }
                    }
                    text: getLastMsgText(model.token, model.lastmsgusername,
                                         model.lastmsg)
                    color: "#666666"
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    clip: true
                    Layout.maximumWidth: parent.implicitWidth - avatar.implicitWidth
                                         - lastmsgtime.implicitWidth - RowLayout.spacing * 2
                    maximumLineCount: 1  // 最大行数限制
                }
            }

            Text {
                id: lastmsgtime
                Layout.preferredWidth: 40
                Layout.preferredHeight: 20
                Layout.alignment: Qt.RightEdge | Qt.AlignVCenter
                text: Qt.formatDateTime(model.lastmsgtime, "hh:mm")
                color: "#666666"
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }

    // 鼠标交互区域
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
