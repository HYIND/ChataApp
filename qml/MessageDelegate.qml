import QtQuick
import QtQuick.Controls 2.5
import Qt5Compat.GraphicalEffects

Item {
    id: root
    height: profilephoto.height + loadingLoader.height

    property bool isCurrentUser: sessionModel.isMyToken(model.srctoken)

    // MouseArea {
    //     anchors.fill: parent
    //     propagateComposedEvents: true
    //     onClicked: (mouse)=>{
    //         console.log("item clicked")
    //         mouse.accepted = false
    //     }
    //     onPositionChanged: {
    //         mouse.accepted = false
    //     }
    // }
    //消息区域
    Row {
        padding: 30
        width: parent.width
        spacing: 8
        layoutDirection: isCurrentUser ? Qt.RightToLeft : Qt.LeftToRight

        //头像区域
        Rectangle {
            id: profilephoto
            width: 40
            height: 40
            radius: 20
            color: "#7cdcfe"

            Text {
                text: model.name.substring(0, 1).toUpperCase()
                color: "white"
                font.bold: true
                font.pixelSize: 16
                anchors.centerIn: parent
            }
        }

        //消息区域
        Column {
            spacing: 2

            //发送者时间和名称
            Row {
                height: msgsender_name.implicitHeight
                spacing: 3
                TextArea {
                    id: msgsender_name
                    text: model.name
                    font.bold: true
                    font.pixelSize: 12
                    background: Rectangle {
                        color: "transparent"
                    }
                }
                TextArea {
                    id: msg_time
                    text: Qt.formatDateTime(model.time, "hh:mm:ss")
                    font.bold: true
                    font.pixelSize: 12
                    background: Rectangle {
                        color: "transparent"
                    }
                }

                // MouseArea{
                //     anchors.fill: parent
                //     propagateComposedEvents: true
                //     onClicked: {
                //         console.log("item clicked")
                //         mouse.accepted = false
                //     }
                // }
            }

            Item {
                width: parent.width
                height: loadingLoader.height

                Component {
                    id: bubble_picture
                    //消息气泡(图片)
                    Rectangle {
                        id: messagebubble_picture
                        visible: model.type == 2
                        width: imgPreview.width + 20
                        height: imgPreview.height + 16
                        radius: 8
                        color: "#7cdcfe"

                        anchors.right: isCurrentUser ? parent.right : undefined
                        anchors.left: isCurrentUser ? undefined : parent.left

                        Image {
                            id: imgPreview
                            width: Math.min(imgPreview.sourceSize.width + 20,
                                            root.width * 0.7, 380)
                            height: width * (imgPreview.sourceSize.height
                                             / imgPreview.sourceSize.width)
                            anchors.centerIn: parent
                            fillMode: Image.PreserveAspectFit
                            source: loadBase64(model.msg)
                            asynchronous: true
                            // 通过函数更新图片
                            function loadBase64(base64Data) {
                                return "data:image/png;base64," + base64Data
                            }
                            property var previewWindow: null
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (!imgPreview.previewWindow) {
                                        var component = Qt.createComponent(
                                                    "ImgPreviewWindow.qml")
                                        if (component.status === Component.Ready) {
                                            const validSource = String(
                                                                  imgPreview.source)
                                            var win = component.createObject(
                                                        root, {
                                                            "imageSource": imgPreview.source
                                                        })
                                            imgPreview.previewWindow = win
                                            imgPreview.previewWindow.onClosing.connect(
                                                        function () {
                                                            imgPreview.previewWindow = null
                                                        })
                                            imgPreview.previewWindow.show()
                                        }
                                    } else {
                                        // 如果窗口已存在，则激活它
                                        imgPreview.previewWindow.requestActivate()
                                    }
                                }
                            }
                        }

                        // 加载状态指示
                        BusyIndicator {
                            anchors.centerIn: parent
                            running: parent.status === Image.Loading
                        }
                    }
                }
                Component {
                    id: bubble_file
                    //消息气泡(文件)
                    Rectangle {
                        id: messagebubble_file
                        visible: model.type == 3
                        width: 220
                        height: 100
                        radius: 8
                        color: "#ffffff"

                        anchors.right: isCurrentUser ? parent.right : undefined
                        anchors.left: isCurrentUser ? undefined : parent.left

                        Row {
                            anchors.fill: parent
                            anchors.margins: 12
                            Column {
                                height: parent.height
                                width: parent.width
                                spacing: 7
                                Rectangle {
                                    height: filenameText.height
                                    width: parent.width
                                    Text {
                                        id: filenameText
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width
                                        height: filenameText.implicitHeight
                                        text: model.filename
                                        font.pixelSize: 15
                                        clip: true
                                        maximumLineCount: 1 // 最大行数限制
                                        color: "#000000"
                                        elide: Text.ElideRight
                                    }
                                }
                                Rectangle {
                                    height: filesizeText.height
                                    width: parent.width
                                    Text {
                                        id: filesizeText
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width
                                        height: filesizeText.implicitHeight
                                        text: model.filesizestr
                                        font.pixelSize: 12
                                        clip: true
                                        maximumLineCount: 1 // 最大行数限制
                                        color: "#d5d5d5"
                                        elide: Text.ElideRight
                                    }
                                }
                                Row {
                                    id: fileprogress
                                    width: parent.width
                                    height: parent.height - filenameText.height
                                            - filesizeText.height
                                    spacing: 10
                                    Rectangle {
                                        id: filebar
                                        height: parent.height
                                        width: parent.width - 60
                                        color: "#00000000"
                                        Rectangle {
                                            anchors.verticalCenter: parent.verticalCenter
                                            id: filebar_background
                                            height: 6
                                            radius: 1
                                            width: parent.width
                                            color: "#d5d5d5"
                                            Rectangle {
                                                id: filebar_progress
                                                height: parent.height
                                                radius: parent.radius
                                                width: parent.width * (model.fileprogress / 100)
                                                color: "#28bf74"
                                                clip: true // 关键点：裁剪超出部分

                                                Rectangle {
                                                    visible: model.fileprogress > 0
                                                             && model.fileprogress < 100
                                                             && model.filestatus == 1
                                                    x: 0
                                                    id: filebar_Grad
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    height: parent.height - 3
                                                    width: 50
                                                    // 使用 LinearGradient 实现高光效果
                                                    layer.enabled: true
                                                    layer.effect: LinearGradient {
                                                        start: Qt.point(0, 0)
                                                        end: Qt.point(50,
                                                                      0) // 水平渐变
                                                        gradient: Gradient {
                                                            GradientStop {
                                                                position: 0.0
                                                                color: "#28bf74"
                                                            } // 主绿色
                                                            GradientStop {
                                                                position: 0.4
                                                                color: "#FFFFFF"
                                                            } // 高亮部分
                                                            GradientStop {
                                                                position: 0.6
                                                                color: "#FFFFFF"
                                                            } // 高亮部分
                                                            GradientStop {
                                                                position: 1.0
                                                                color: "#28bf74"
                                                            } // 主绿色
                                                        }
                                                    }
                                                    // 动画：让高光从左向右流动
                                                    PropertyAnimation on x {
                                                        from: -filebar_Grad.width
                                                        to: filebar_background.width
                                                            + filebar_Grad.width
                                                        duration: 1500
                                                        loops: Animation.Infinite
                                                        running: model.fileprogress > 0
                                                                 && model.fileprogress < 100
                                                                 && model.filestatus == 1
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    Rectangle {
                                        height: parent.height
                                        width: 60
                                        Row {
                                            anchors.fill: parent
                                            spacing: 6

                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                id: fileprogressText
                                                width: fileprogressText.implicitWidth
                                                height: fileprogressText.implicitHeight
                                                text: model.fileprogress + "%"
                                                font.pixelSize: 12
                                                maximumLineCount: 1 // 最大行数限制
                                                color: "#5a5a5a"
                                            }

                                            // 暂停按钮  (status=1) - 传输中，显示暂停按钮
                                            Component {
                                                id: stopbtn
                                                Rectangle {
                                                    height: parent.height
                                                    width: 15
                                                    Row {
                                                        anchors.verticalCenter: parent.verticalCenter
                                                        spacing: 3
                                                        height: parent.height
                                                        width: parent.width
                                                        Rectangle {
                                                            color: "#0a0a0a"
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            height: 10
                                                            width: 3
                                                            radius: 1
                                                        }
                                                        Rectangle {
                                                            color: "#0a0a0a"
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            height: 10
                                                            width: 3
                                                            radius: 1
                                                        }
                                                    }
                                                    MouseArea{
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            sessionmodel.stopTrans(model.fileid);
                                                        }
                                                    }
                                                }
                                            }

                                            // 开始按钮  (status=0) - 停止，显示开始传输按钮
                                            Component {
                                                id: startbtn
                                                Rectangle {
                                                    height: parent.height
                                                    width: 15
                                                    Canvas {
                                                        anchors.centerIn: parent
                                                        width: 10
                                                        height: 10

                                                        onPaint: {
                                                            var ctx = getContext(
                                                                        "2d")
                                                            ctx.fillStyle = "#0a0a0a"
                                                            ctx.beginPath()
                                                            ctx.moveTo(0,
                                                                       0) // 起点
                                                            ctx.lineTo(width,
                                                                       height / 2) // 右下角
                                                            ctx.lineTo(0,
                                                                       height) // 左下角
                                                            ctx.closePath()
                                                            ctx.fill()
                                                        }
                                                    }
                                                    MouseArea{
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            sessionmodel.startTrans(model.fileid);
                                                        }
                                                    }
                                                }
                                            }
                                            // 错误按钮 (status=3) - 红色叉叉
                                            Component {
                                                id: errorbtn
                                                Rectangle {
                                                    height: parent.height
                                                    width: 15
                                                    Canvas {
                                                        anchors.centerIn: parent
                                                        width: 10
                                                        height: 10
                                                        onPaint: {
                                                            var ctx = getContext(
                                                                        "2d")
                                                            ctx.strokeStyle = "#ff0000" // 红色
                                                            ctx.lineWidth = 2

                                                            // 画叉叉的第一条线
                                                            ctx.beginPath()
                                                            ctx.moveTo(1, 1)
                                                            ctx.lineTo(width - 1,
                                                                       height - 1)
                                                            ctx.stroke()

                                                            // 画叉叉的第二条线
                                                            ctx.beginPath()
                                                            ctx.moveTo(width - 1,
                                                                       1)
                                                            ctx.lineTo(1,
                                                                       height - 1)
                                                            ctx.stroke()
                                                        }
                                                    }
                                                    MouseArea{
                                                        anchors.fill: parent
                                                        hoverEnabled: true
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            sessionmodel.startTrans(model.fileid);
                                                        }
                                                    }
                                                }
                                            }
                                            // 加载器控制
                                            Loader {
                                                visible: model.filestatus != 2
                                                id: btnLoader
                                                height: parent.height
                                                width: 15
                                                sourceComponent: model.filestatus === 0 ? startbtn : model.filestatus === 1 ? stopbtn : model.filestatus === 3 ? errorbtn : null
                                                active: true
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                Component {
                    id: bubble_text
                    //消息气泡(文本)
                    Rectangle {
                        id: messagebubble_text
                        visible: model.type == 1
                        width: messageText.width + 20
                        height: messageText.height + 16
                        radius: 8
                        color: "#7cdcfe"

                        anchors.right: isCurrentUser ? parent.right : undefined
                        anchors.left: isCurrentUser ? undefined : parent.left

                        TextMetrics {
                            id: textMetrics
                            font: messageText.font
                            text: messageText.text
                            elide: Text.ElideNone
                        }

                        TextArea {
                            id: messageText
                            leftPadding: 0
                            rightPadding: 0
                            topPadding: 0
                            bottomPadding: 0
                            font.family: "Microsoft YaHei"
                            font.pixelSize: 12
                            width: Math.min(textMetrics.width + 5,
                                            root.width * 0.7, 380)
                            height: messageText.implicitHeight
                            anchors.centerIn: parent
                            text: model.msg
                            wrapMode: Text.Wrap
                            background: Rectangle {
                                color: "transparent"
                            }
                        }
                    }
                }

                // 加载器控制
                Loader {
                    id: loadingLoader
                    anchors.right: isCurrentUser ? parent.right : undefined
                    anchors.left: isCurrentUser ? undefined : parent.left
                    sourceComponent: model.type === 2 ? bubble_picture : model.type
                                                        === 3 ? bubble_file : bubble_text
                    active: true
                }
            }
        }
    }
}
