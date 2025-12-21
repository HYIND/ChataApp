import QtQuick

// 大图预览弹窗
Window {
    id: imagePreviewWindow

    required property url imageSource
    property bool canclose:true

    // 窗口设置
    width: sourceImage.width
    height: sourceImage.height
    x : (Screen.width - sourceImage.width)/2
    y : (Screen.height - sourceImage.height)/2
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "#50f5f5f5"


    onVisibleChanged:
    {
        if(visible)
        {
            canclose = false;
            canclosetimer.start()
        }
    }

    Image {
        width: Math.min(Screen.width * 0.9, sourceImage.sourceSize.width)
        height: Math.min(Screen.height * 0.9, sourceImage.sourceSize.height)
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
            if(!canclose)return;

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
        onDoubleClicked: {
            if(!canclose)return;
            imagePreviewWindow.close()
            imagePreviewWindow.destroy()
        }
    }

    Timer
    {
        id:canclosetimer
        interval: 200
        onTriggered: {
            canclose = true;
        }
    }
}
