import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Item {
    id: root
    // width: 120
    // height: 40

    // 发送图片信号
    signal imageSelected(string filePath)

    // 文件选择对话框
    FileDialog {
        id: fileDialog
        title: "选择图片"
        nameFilters: ["图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"]
        onAccepted: {
            console.log("已选择图片:", removePrefixFromUrl(fileDialog.selectedFile))
            root.imageSelected(removePrefixFromUrl(fileDialog.selectedFile))
        }
    }

    // 样式化按钮
    Button {
        id: pickButton
        anchors.fill: parent
        text: "选择图片"

        // 按钮样式
        background: Rectangle {
            radius: 4
            color: pickButton.down ? "#e0e0e0" : "#f0f0f0"
            border.color: "#cccccc"
        }

        onClicked: fileDialog.open()
    }

    // 状态提示
    // Text {
    //     anchors.top: pickButton.bottom
    //     anchors.topMargin: 5
    //     anchors.horizontalCenter: parent.horizontalCenter
    //     text: fileDialog.selectedFile ? "已选择: " + fileNameFromUrl(fileDialog.selectedFile) : ""
    //     font.pixelSize: 10
    //     color: "#666666"
    // }

    // 从URL提取文件名
    function fileNameFromUrl(url) {
        return url.toString().split('/').pop()
    }
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
}
