import QtQuick

// 大图预览弹窗
Window {
    id: imagePreviewWindow

    required property url imageSource
    property bool canclose:true

    // 窗口设置
    width: imageDisplayLoader.width
    height: imageDisplayLoader.height
    x : (Screen.width - imageDisplayLoader.width)/2
    y : (Screen.height - imageDisplayLoader.height)/2
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

    function isGifFile(path) {
        if (!path) return false;
        var lowerPath = path.toString().toLowerCase();
        return lowerPath.endsWith(".gif") ||
               lowerPath.includes(".gif?") ||
               lowerPath.includes("data:image/gif") ||
               lowerPath.includes("image/gif;base64");
    }

    function processImagePath(path)
    {
        var qurl = Qt.url(path)
        var resolveurl = Qt.resolvedUrl(qurl);
        return resolveurl;
    }

    Loader {
        id: imageDisplayLoader
        // 根据图片类型动态设置sourceComponent
        sourceComponent: {
            // var processedSource = processImagePath(imageSource);
            // console.log("加载图片:", processedSource);
            // console.log("isGif:", isGifFile(imageSource));

            if (isGifFile(imageSource)) {
                return gifComponent;
            } else {
                return imageComponent;
            }
        }

        onLoaded: {
            // console.log("组件加载完成，item:", item);
            if (item) {
                // 设置Loader的尺寸为内容的尺寸
                width = item.width
                height = item.height
                // console.log("Loader尺寸:", width, height);
            }
        }

        property string currentSource: processImagePath(imageSource)
    }

    Component {
        id: imageComponent
        Image {
            id: innerImage
            width: Math.min(Screen.width * 0.9, innerImage.sourceSize.width)
            height: Math.min(Screen.height * 0.9, innerImage.sourceSize.height)
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            source: processImagePath(imageSource)
        }
    }

    Component {
        id: gifComponent
        AnimatedImage {
            id: innerGifImage
            playing: true  // 默认自动播放
            speed: 1.0     // 播放速度（1.0=正常）
            width: Math.min(Screen.width * 0.9, innerGifImage.sourceSize.width)
            height: Math.min(Screen.height * 0.9, innerGifImage.sourceSize.height)
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            source: processImagePath(imageSource)
        }
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
            const imgLeft = imageDisplayLoader.x
            const imgRight = imageDisplayLoader.x + imageDisplayLoader.width
            const imgTop = imageDisplayLoader.y
            const imgBottom = imageDisplayLoader.y + imageDisplayLoader.height

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
