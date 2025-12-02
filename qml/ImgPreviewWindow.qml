import QtQuick

// 大图预览弹窗
Window {
    id: imagePreviewWindow

    required property url imageSource

    // 窗口设置
    width: Screen.width  // 占屏幕80%宽度
    height: Screen.height // 占屏幕80%高度
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "#80000000"  // 半透明黑色背景 (80是透明度)

    Image {
        width: Math.min(Screen.width * 0.9, sourceImage.sourceSize.width)
        height: Math.min(Screen.width * 0.9, sourceImage.sourceSize.width)
        id: sourceImage
        // anchors.fill: parent
        anchors.centerIn: parent
        fillMode: Image.PreserveAspectFit
        source: imagePreviewWindow.imageSource // 绑定小图的源
    }

    // 点击区域（仅灰色背景响应关闭）
    MouseArea {
        anchors.fill: parent
        // 关键：排除图片区域的点击
        acceptedButtons: Qt.LeftButton
        onClicked: {
            // 计算点击位置是否在图片外
            const clickX = mouseX
            const clickY = mouseY
            const imgLeft = sourceImage.x
            const imgRight = sourceImage.x + sourceImage.width
            const imgTop = sourceImage.y
            const imgBottom = sourceImage.y + sourceImage.height

            if (clickX < imgLeft || clickX > imgRight ||
                clickY < imgTop || clickY > imgBottom) {
                imagePreviewWindow.close()
                imagePreviewWindow.destroy()
            }
        }
    }
}
