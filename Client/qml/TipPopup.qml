import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects

Rectangle {
    id: tipPopup
    width: Math.min(parent.width * 0.8, 300)
    height: contentColumn.height + 40
    radius: 8
    color: "#AA333333"  // 半透明黑色背景
    opacity: 0
    visible: opacity > 0

    property int showtime:2000

    // 位置 - 默认居中
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    anchors.bottomMargin: 50

    // 阴影效果
    layer.enabled: true
    layer.effect: DropShadow {
        horizontalOffset: 0
        verticalOffset: 2
        radius: 8
        samples: 17
        color: "#80000000"
    }

    Column {
        id: contentColumn
        width: parent.width - 40
        anchors.centerIn: parent
        spacing: 8

        Text {
            id: tipText
            width: parent.width
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            color: "white"
            font.pixelSize: 14
            font.family: "Microsoft YaHei"
        }
    }

    // 定时器控制显示时间
    Timer {
        id: hideTimer
        interval: showtime
        onTriggered: tipPopup.hide()
    }

    // 显示动画
    ParallelAnimation {
        id: showAnimation
        NumberAnimation {
            target: tipPopup
            property: "opacity"
            from: 0
            to: 1
            duration: 300
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: tipPopup
            property: "anchors.bottomMargin"
            from: 20
            to: 50
            duration: 300
            easing.type: Easing.OutCubic
        }
        onStarted: {
            tipPopup.z = 9999
        }
    }

    // 隐藏动画
    ParallelAnimation {
        id: hideAnimation
        NumberAnimation {
            target: tipPopup
            property: "opacity"
            from: 1
            to: 0
            duration: 300
            easing.type: Easing.InCubic
        }
        NumberAnimation {
            target: tipPopup
            property: "anchors.bottomMargin"
            from: 50
            to: 20
            duration: 300
            easing.type: Easing.InCubic
        }
        onStopped: {
            tipPopup.z = 0
            tipText.text = ""
        }
    }

    // 公共方法
    function show(message) {
        if(tipText.text == message)
            return
        tipText.text = message
        showAnimation.start()
        hideTimer.restart()
    }

    function hide() {
        hideAnimation.start()
    }
}
