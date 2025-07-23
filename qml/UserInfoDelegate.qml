import QtQuick
import QtQuick.Layouts

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

                Text {
                    text: model.username
                    font.bold: true
                    font.pixelSize: 18
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    clip: true
                    maximumLineCount: 1  // 最大行数限制
                }

                Text {
                    text: model.useraddress
                    color: "#666666"
                    font.pixelSize: 14
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    clip: true
                    maximumLineCount: 1  // 最大行数限制
                }
            }
        }
    }
}
