import QtQuick
import QtQuick.Layouts
import QtQuick.Controls 2.15

Item {
    id: root

    property var model

    Rectangle {
        anchors.fill: parent
        color: "#00000000"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 10

            // 用户头像
            Rectangle {
                id: avatar
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48
                radius: width / 2
                color: "#4a6da7"

                Text {
                    anchors.centerIn: parent
                    text: model.username.substring(0, 1).toUpperCase()
                    color: "white"
                    font.bold: true
                    font.pixelSize: 24
                }
            }

            // 用户信息
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 3
                clip: true

                TextArea {
                    text: model.username
                    font.bold: true
                    font.pixelSize: 18
                    Layout.fillWidth: true
                    clip: true
                    background: Rectangle {
                        color: "transparent"
                    }
                    wrapMode: Text.NoWrap
                    readOnly: true
                    selectByMouse: true
                }

                TextArea {
                    text: model.useraddress
                    color: "#666666"
                    font.pixelSize: 14
                    Layout.fillWidth: true
                    clip: true
                    background: Rectangle {
                        color: "transparent"
                    }
                    wrapMode: Text.NoWrap
                    readOnly: true
                    selectByMouse: true
                }
            }
        }
    }
}
