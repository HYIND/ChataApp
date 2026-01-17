import QtQuick
import QtMultimedia
import QtQuick.Controls

// 大图预览弹窗
Window {
    id: videoWindow

    required property url videoSource
    required property string fileid
    property bool canclose:true

    property point dragStart: Qt.point(0, 0)
    property bool dragging: false

    // 窗口设置
    width: Math.max(controlBar.width,videoDisplayLoader.width)
    height: controlBar.height + videoDisplayLoader.height
    x : (Screen.width - videoDisplayLoader.width)/2
    y : (Screen.height - videoDisplayLoader.height)/2
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

    function processPath(path)
    {
        var qurl = Qt.url(path)
        var resolveurl = Qt.resolvedUrl(qurl);
        return resolveurl;
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
                            videoWindow.x += mouse.x - dragStart.x;
                            videoWindow.y += mouse.y - dragStart.y;
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
                            videoWindow.close()
                            videoWindow.destroy()
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
            id: videoDisplayLoader
            sourceComponent: {
                return videoComponent;
            }

            // onLoaded: {
            //     if (item) {
            //         // 设置Loader的尺寸为内容的尺寸
            //         width = item.width
            //         height = item.height
            //     }
            // }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton

                propagateComposedEvents: true

                onClicked: function(mouse){
                    mouse.accepted = false;
                }
                onPressed: function(mouse) {
                    dragStart = Qt.point(mouse.x, mouse.y);
                }
                onPositionChanged: function(mouse) {
                    dragging = true;
                    if (dragging) {
                        videoWindow.x += mouse.x - dragStart.x;
                        videoWindow.y += mouse.y - dragStart.y;
                    }
                }
                onReleased: {
                    dragging = false;
                }

            }
        }
    }

    Component {
        id: videoComponent
        Item
        {
            id:cponentitem
            width: Math.min(Screen.width * 0.9,  Math.max(videoColumn.width,200))
            height: Math.min(Screen.height * 0.9,  Math.max(videoColumn.height,200))

            Column
            {
                id:videoColumn
                width: Math.max(innerVideoplayer.width, videoControls.width)
                height : innerVideoplayer.height + videoControls.height + videoTime.height + spacing*2

                property bool forceLayout: false
                onForceLayoutChanged: {
                    if (forceLayout) {
                        // 重新设置子项位置
                        for (var i = 0; i < children.length; i++) {
                            var child = children[i]
                            if (i === 0) {
                                child.y = 0
                            } else {
                                child.y = children[i-1].y + children[i-1].height + spacing
                            }
                        }
                        forceLayout = false
                    }
                }

                function updateLayout() {
                    videoColumn.width = Math.max(innerVideoplayer.width, videoControls.width)
                    videoColumn.height = innerVideoplayer.height + videoControls.height + videoTime.height + spacing * 2

                    cponentitem.width = Math.max(videoColumn.width, 200)
                    cponentitem.height = Math.max(videoColumn.height, 200)

                    videoDisplayLoader.width = cponentitem.width
                    videoDisplayLoader.height = cponentitem.height

                    videoWindow.width = Math.max(controlBar.width,videoDisplayLoader.width)
                    videoWindow.height = controlBar.height + videoDisplayLoader.height

                    controlBar.width = videoWindow.width

                    videoWindow.x = (Screen.width - videoWindow.width) / 2
                    videoWindow.y = (Screen.height - videoWindow.height) / 2

                    videoColumn.forceLayout = true
                }

                spacing: 10

                Rectangle
                {
                    id:innerVideoplayer
                    width : 960
                    height : 540
                    color: "black"

                    MediaPlayer {
                        id: mediaPlayer
                        source: processPath(videoSource)
                        audioOutput: AudioOutput {}
                        videoOutput: videoOutput
                        onMediaStatusChanged: {
                            if (mediaStatus === MediaPlayer.LoadedMedia) {
                                play()
                            }
                        }
                    }

                    VideoOutput {
                        id: videoOutput
                        anchors.fill: parent
                        fillMode: VideoOutput.PreserveAspectFit
                        endOfStreamPolicy:VideoOutput.KeepLastFrame

                        onSourceRectChanged: {
                            console.log("sourceRect 变化:", sourceRect)
                            if (sourceRect.width > 0 && sourceRect.height > 0) {
                                let videoAspectRatio = sourceRect.width / sourceRect.height
                                console.log("获取到视频尺寸:",sourceRect.width,sourceRect.height,
                                            "宽高比:", videoAspectRatio)

                                console.log(videoTime.width,videoTime.height)

                                let maxvideoheight = Screen.height * 0.9 - (controlBar.height) - (videoControls.height + videoTime.height + videoColumn.spacing * 2)
                                let maxvideowidth = Screen.width * 0.9

                                let scaleByWidth = maxvideowidth / sourceRect.width
                                let scaleByHeight = maxvideoheight / sourceRect.height
                                let scale = Math.min(scaleByWidth, scaleByHeight)

                                if(scale < 1.0)
                                {
                                    let newWidth = sourceRect.width * scale
                                    let newHeight = sourceRect.height * scale
                                    innerVideoplayer.width =  newWidth
                                    innerVideoplayer.height = newHeight
                                }
                                else
                                {
                                    innerVideoplayer.width =  sourceRect.width
                                    innerVideoplayer.height = sourceRect.height
                                }
                            }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            mediaPlayer.playbackState == MediaPlayer.PlayingState ?
                                        mediaPlayer.pause() : mediaPlayer.play()
                        }
                    }

                    onWidthChanged: videoColumn.updateLayout()
                    onHeightChanged: videoColumn.updateLayout()
                }

                Item {
                    id: videoControls
                    width: Math.max(20, innerVideoplayer.width)
                    height: videoSlider.height

                    Slider {
                        id: videoSlider

                        anchors {
                            left: parent.left
                            right: parent.right
                            verticalCenter: parent.verticalCenter
                            leftMargin: 15    // 左边距
                            rightMargin: 15   // 右边距
                        }

                        from: 0
                        to: mediaPlayer.duration
                        value: mediaPlayer.position

                        focusPolicy: Qt.NoFocus

                        handle: Rectangle {
                            x: videoSlider.leftPadding + videoSlider.visualPosition *
                               (videoSlider.availableWidth - width)
                            y: videoSlider.topPadding + (videoSlider.availableHeight - height) / 2
                            implicitWidth: 20
                            implicitHeight: 20
                            radius: 10
                            color: videoSlider.pressed ? "#f0f0f0" : "#f6f6f6"
                            border.color: "#bdbebf"
                            border.width: 2
                        }

                        onPressedChanged: {
                            if (!pressed) {  // 松开时
                                mediaPlayer.position = value
                                mediaPlayer.play()
                            }
                            else
                            {
                                mediaPlayer.position = value
                                mediaPlayer.pause()
                            }
                        }
                    }
                }
                Item {
                    id: videoTime
                    width: videoSlider.width
                    height: 30
                    Text {
                        function msToTime(duration) {
                            if (!duration || duration <= 0) return "00:00"

                            var seconds = Math.floor(duration / 1000)
                            var minutes = Math.floor(seconds / 60)
                            seconds = seconds % 60

                            // 补零
                            var minutesStr = minutes < 10 ? "0" + minutes : minutes
                            var secondsStr = seconds < 10 ? "0" + seconds : seconds

                            return minutesStr + ":" + secondsStr
                        }

                        function getTimeStr()
                        {
                            return msToTime(mediaPlayer.position) + " / "+ msToTime(mediaPlayer.duration)
                        }

                        id: videoTimeText
                        text: getTimeStr()
                        color: "black"
                        font.pixelSize: 14

                        leftPadding: 30
                    }
                }
            }

            // width: Math.min(Screen.width * 0.9,  Math.max(innerVideo.sourceSize.width,200))
            // height: Math.min(Screen.height * 0.9,  Math.max(innerVideo.sourceSize.height,200))

            // width: Math.min(Screen.width * 0.9,  Math.max(0,200))
            // height: Math.min(Screen.height * 0.9,  Math.max(0,200))
            // Video {
            //     id: innerVideo
            //     width: Math.min(parent.width,  100)
            //     height: Math.min(parent.height, 100)
            //     anchors.centerIn: parent
            //     // fillMode: VideoOutput.PreserveAspectFit
            //     source: processPath(videoSource)
            // }
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
