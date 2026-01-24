import QtQuick 2.15
import QtQuick.Window 2.15
import OpenGLRendering 1.0

Window {
    id: modelViewerWindow

    required property url filepath

    width: 1024
    height: 768
    visible: true
    title: filepath + " --预览"

    // C++ OpenGL直接渲染的内容
    OpenGLRenderItem {
        id: glRenderer
        anchors.fill: parent

        // 添加边框以便确认位置
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "red"
            border.width: 2
        }

        Component.onCompleted:{
        }

        Timer {
            id: initTimer
            interval: 100  // 延迟100ms确保渲染器已创建
            running: true
            onTriggered: {
                console.log("初始化OpenGL渲染器...")
                // 设置渲染参数
                if (typeof glRenderer.setFilePath === 'function') {
                    glRenderer.setFilePath(modelViewerWindow.filepath)
                    // glRenderer.setFilePath("D:\\Desktop\\code\\Qt\\chat_sc\\chat_qml_client\\test1.obj")
                }
                // glRenderer.initialized = true
            }
        }
    }

    // // QML控件叠加层
    // Rectangle {
    //     anchors.top: parent.top
    //     anchors.left: parent.left
    //     anchors.margins: 20
    //     width: 300
    //     height: 120
    //     color: "#80000000"
    //     radius: 10

    //     Column {
    //         anchors.fill: parent
    //         anchors.margins: 10
    //         spacing: 5

    //         Text {
    //             text: "渲染架构"
    //             color: "white"
    //             font.bold: true
    //             font.pixelSize: 16
    //         }

    //         Text {
    //             text: "• C++: 直接OpenGL渲染"
    //             color: "lightgreen"
    //         }

    //         Text {
    //             text: "• QML: 仅提供显示窗口"
    //             color: "lightblue"
    //         }

    //         Text {
    //             text: "• 无Qt3D依赖"
    //             color: "yellow"
    //         }
    //     }
    // }

    // Rectangle {
    //     anchors.bottom: parent.bottom
    //     anchors.horizontalCenter: parent.horizontalCenter
    //     anchors.margins: 20
    //     width: 400
    //     height: 40
    //     color: "#cc000000"
    //     radius: 5

    //     Text {
    //         anchors.centerIn: parent
    //         color: "white"
    //         text: "C++原生OpenGL渲染 → QML窗口显示"
    //     }
    // }
}
