import QtQuick
import QtMultimedia
import QtQuick.Controls

// 大图预览弹窗
Window {
    id: imagePreviewWindow

    required property url imageSource
    required property string fileid
    required property var ocrmodel
    property var ocrResults:[]
    property bool ocrEnabled: false
    property bool canclose:true

    property point dragStart: Qt.point(0, 0)
    property bool dragging: false

    property int imgwidth: 0
    property int imgheight: 0

    // 窗口设置
    width: Math.max(controlBar.width ,imageDisplayLoader.width)
    height: controlBar.height + imageDisplayLoader.height
    x : (Screen.width - width)/2
    y : (Screen.height - height)/2
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

    function processOCR() {
        if (!ocrmodel || !ocrmodel.isLoaded()) {
            console.error("OCR模型不可用");
            return;
        }
        if(ocrmodel.getState(fileid) == -2)
            ocrmodel.addTask(fileid);

        if(ocrmodel.getState(fileid) == 1)
        {
            ocrResults = ocrmodel.getResult(fileid);
            // console.log("processOCR ocrResults", ocrResults)
        }
    }

    function toggleOCR() {
        ocrEnabled = !ocrEnabled;
    }

    onImgwidthChanged: {
        // console.log("onImgwidthChanged",imgwidth);
        ocrOverlay.width = imgwidth;
    }

    onImgheightChanged: {
        // console.log("onImgheightChanged",imgheight);
        ocrOverlay.height = imgheight;
    }

    Connections
    {
        target: ocrmodel
        function onocrTaskDone(donefileid) {
            if(donefileid != fileid)
                return;

            if(ocrmodel.getState(fileid) == 1)
            {
                ocrResults = ocrmodel.getResult(fileid);
                // console.log("onocrTaskDone ocrResults", ocrResults)
            }
        }
    }

    // 顶部控制栏
    Column
    {

        Rectangle {
            id: controlBar
            width: Math.max(200, parent.width)
            height: 40
            color: "white"
            radius: 8

            Item {
                anchors.left: parent.left
                width: parent.width - (rightButtonRow.width + rightButtonRow.anchors.margins)
                height: parent.height

                MouseArea
                {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton

                    propagateComposedEvents: true

                    onPressed: function(mouse) {
                        dragStart = Qt.point(mouse.x, mouse.y);
                    }
                    onPositionChanged: function(mouse) {
                        dragging = true;
                        if (dragging) {
                            imagePreviewWindow.x += mouse.x - dragStart.x;
                            imagePreviewWindow.y += mouse.y - dragStart.y;
                        }
                    }
                    onReleased: {
                        dragging = false;
                    }
                }
            }

            // 右上角按钮区域
            Row {
                id: rightButtonRow
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 4
                spacing: 20
                z: 100  // 确保在最上层

                // OCR开关按钮
                Rectangle {
                    id: ocrToggleButton
                    width: height
                    height: controlBar.height - parent.anchors.margins * 2
                    radius: height/2
                    color: ocrEnabled ? "#757575" : "#4CAF50"
                    opacity: 0.8

                    visible: !isGifFile(imageSource)

                    Text {
                        anchors.centerIn: parent
                        text: ocrEnabled ? "关" : "OCR"
                        color: "white"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: toggleOCR()
                        cursorShape: Qt.PointingHandCursor
                    }
                }

                // 关闭按钮
                Rectangle {
                    id: closeButton
                    width: height
                    height: controlBar.height - parent.anchors.margins * 2
                    radius: height/2
                    color: "#F44336"
                    opacity: 0.8

                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        color: "white"
                        font.pixelSize: 20
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            imagePreviewWindow.close()
                            imagePreviewWindow.destroy()
                        }
                        cursorShape: Qt.PointingHandCursor
                    }
                }
            }

            // 控制栏分隔线
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 3
                color: "#666666"
            }
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

            Item
            {
                id:ocrOverlay
                anchors.centerIn : parent
                z:100

                Repeater {
                    model: ocrResults

                    delegate: Rectangle {
                        // 透明背景，只用于定位
                        color: "transparent"

                        // 根据OCR结果定位
                        x: modelData.x
                        y: modelData.y

                        visible: ocrEnabled

                        // 文字区域标注
                        Rectangle {
                            id: textBg
                            width: modelData.width
                            height: modelData.height

                            color: Qt.rgba(1, 0, 0, 0.2)  // 半透明红色背景
                            radius: 3
                        }

                        // 文字标签
                        TextArea {
                            id: textLabel
                            text: modelData.text

                            font.bold: true
                            font.pixelSize: 14

                            clip: false
                            background: Rectangle {
                                color: "transparent"
                            }

                            color: "#FFFF00"
                            wrapMode: Text.NoWrap

                            readOnly: true
                            selectByMouse: true

                            height: implicitHeight
                        }


                        // 置信度指示器
                        // Rectangle {
                        //     anchors.top: textBg.bottom
                        //     anchors.topMargin: 2
                        //     width: textBg.width
                        //     height: 5
                        //     color: "lightgray"

                        //     Rectangle {
                        //         width: parent.width * (modelData.conf / 100)
                        //         height: parent.height
                        //         color: modelData.conf > 90 ? "green" :
                        //                modelData.conf > 80 ? "orange" : "red"
                        //     }

                        //     Text {
                        //         anchors.centerIn: parent
                        //         text: modelData.conf + "%"
                        //         color: "white"
                        //         font.pixelSize: 10
                        //     }
                        // }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton

                    propagateComposedEvents: true

                    onClicked: function(mouse){
                        mouse.accepted = false;
                    }
                    onDoubleClicked: function(mouse){
                        if(!canclose || dragging) {
                            mouse.accepted = false;
                        }
                        else
                        {
                            imagePreviewWindow.close()
                            imagePreviewWindow.destroy()
                        }
                    }
                    onPressed: function(mouse) {
                        dragStart = Qt.point(mouse.x, mouse.y);
                    }
                    onPositionChanged: function(mouse) {
                        dragging = true;
                        if (dragging) {
                            imagePreviewWindow.x += mouse.x - dragStart.x;
                            imagePreviewWindow.y += mouse.y - dragStart.y;
                        }
                    }
                    onReleased: {
                        dragging = false;
                    }
                }
            }
        }
    }

    Component {
        id: imageComponent

        Item
        {
            width: Math.max(innerImage.width,200)
            height: Math.max(innerImage.height,200)

            Image {

                function getFitWidth()
                {
                    let maxvideoheight = Screen.height * 0.9 - controlBar.height
                    let maxvideowidth = Screen.width * 0.9

                    let scaleByWidth = maxvideowidth / sourceSize.width
                    let scaleByHeight = maxvideoheight / sourceSize.height
                    let scale = Math.min(scaleByWidth, scaleByHeight)

                    if(scale < 1.0)
                    {
                        let newWidth = sourceSize.width * scale
                        return newWidth;
                    }
                    else
                        return sourceSize.width
                }
                function getFitHeight()
                {
                    let maxvideoheight = Screen.height * 0.9 - controlBar.height
                    let maxvideowidth = Screen.width * 0.9

                    let scaleByWidth = maxvideowidth / sourceSize.width
                    let scaleByHeight = maxvideoheight / sourceSize.height
                    let scale = Math.min(scaleByWidth, scaleByHeight)


                    if(scale < 1.0)
                    {
                        let newHeight = sourceSize.height * scale
                        return newHeight;
                    }
                    else
                        return sourceSize.height
                }

                id: innerImage
                width: getFitWidth()
                height: getFitHeight()
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
                source: processImagePath(imageSource)

                onStatusChanged: {
                    if (status === Image.Ready) {
                        // console.log("图片加载完成，开始OCR处理");
                        processOCR();
                    } else if (status === Image.Error) {
                        console.error("图片加载失败:", source);
                    }
                }
                onWidthChanged: {
                    imgwidth = width;
                }
                onHeightChanged: {
                    imgheight = height;
                }
            }
        }
    }

    Component {
        id: gifComponent
        Item
        {
            width: Math.max(innerGifImage.width,200)
            height: Math.max(innerGifImage.height,200)

            AnimatedImage {

                function getFitWidth()
                {
                    let maxvideoheight = Screen.height * 0.9 - controlBar.height
                    let maxvideowidth = Screen.width * 0.9

                    let scaleByWidth = maxvideowidth / sourceSize.width
                    let scaleByHeight = maxvideoheight / sourceSize.height
                    let scale = Math.min(scaleByWidth, scaleByHeight)

                    if(scale < 1.0)
                    {
                        let newWidth = sourceSize.width * scale
                        return newWidth;
                    }
                    else
                        return sourceSize.width
                }
                function getFitHeight()
                {
                    let maxvideoheight = Screen.height * 0.9 - controlBar.height
                    let maxvideowidth = Screen.width * 0.9

                    let scaleByWidth = maxvideowidth / sourceSize.width
                    let scaleByHeight = maxvideoheight / sourceSize.height
                    let scale = Math.min(scaleByWidth, scaleByHeight)


                    if(scale < 1.0)
                    {
                        let newHeight = sourceSize.height * scale
                        return newHeight;
                    }
                    else
                        return sourceSize.height
                }


                id: innerGifImage
                playing: true  // 默认自动播放
                speed: 1.0     // 播放速度（1.0=正常）
                width: getFitWidth()
                height: getFitHeight()
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
                source: processImagePath(imageSource)

                onWidthChanged: {
                    imgwidth = width;
                }
                onHeightChanged: {
                    imgheight = height;
                }
            }
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
