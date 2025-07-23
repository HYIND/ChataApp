import QtQuick
import QtQuick.Controls 2.5

Window {

    id: root

    Component {
        id: chatpage
        ChatPage {
            width: parent.width
            height: parent.height
        }
    }
    Component {
        id: loadpage
        LoadPage {
            width: parent.width
            height: parent.height
        }
    }

    property int currentIndex: 0
    // 加载器控制
    Loader {
        id: loadingLoader
        anchors.fill: parent
        sourceComponent: currentIndex === 0 ? loadpage : chatpage
        active: true
    }

    Connections {
        target: loginmodel
        function onloginSuccess() {
            currentIndex = 1
        }
        function onloginFail() {
            currentIndex = 0
            popup.open()
        }
    }

    Popup {
        id: popup
        anchors.centerIn: parent
        width: 300
        height: 200
        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose

        Item {
            anchors.centerIn: parent
            width: Math.max(popuptips.width, popupbtn.width)
            height: popuptips.height + popupbtn.height + column.spacing
            Column {
                id: column
                spacing: 20
                Item {
                    id: popuptips
                    width: 70
                    height: 30
                    Text {
                        text: "连接服务器失败！"
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: 18
                    }
                }

                Button {
                    id: popupbtn
                    width: 70
                    height: 30
                    Text {
                        text: "确定"
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        font.pixelSize: 18
                    }
                    onClicked: Qt.quit()
                }
            }
        }
    }

    width: 1060
    height: 700
    visible: true
}
