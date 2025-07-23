import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    anchors.fill: parent
    color: "#f5f5f5"
    z: 9999  // 确保在最上层

    Column {
        anchors.centerIn: parent
        spacing: 30

        // 转圈动画
        BusyIndicator {
            id: spinner
            anchors.horizontalCenter: parent.horizontalCenter
            width: 60
            height: 60
            running: true
        }

        // 加载文字（带渐变动画）
        Text {
            id: loadingText
            text: "加载中"
            font.pixelSize: 18
            color: "#333"

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                running: true
                NumberAnimation { from: 0.5; to: 1; duration: 800 }
                NumberAnimation { from: 1; to: 0.5; duration: 800 }
            }
        }
    }
}
