import QtQuick.Controls.Material
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

// Settings: a home list of sections (name + description). Picking one opens
// that page; the header shows the page name and a back arrow.
Item {
    id: settingsPanel
    width: 313
    height: 500

    // -1 = settings home, otherwise the page index in pageCarousel
    property int currentPage: -1
    property string currentTitle: "Settings"

    ListModel {
        id: settingsMenuModel
        ListElement { name: "General"; description: "Startup, tray and logging"; pageIndex: 0 }
        ListElement { name: "Controller"; description: "Choose which controller to use"; pageIndex: 1 }
        ListElement { name: "Button Bindings"; description: "Modifiers, button icons and key bindings"; pageIndex: 2 }
        ListElement { name: "Sticks & Triggers"; description: "Trigger sensitivity, cursor and camera speed"; pageIndex: 3 }
        ListElement { name: "Touchpad & PS5"; description: "Touchpad, L3/R3 buttons and DSX"; pageIndex: 4 }
        ListElement { name: "Pixel Bridge"; description: "Rumble, auto-walk, cursor centering and AoE casting"; pageIndex: 5 }

        Component.onCompleted: {
            if (MainPageController.overlayFilesPresent) {
                settingsMenuModel.append({ "name": "Overlay", "description": "In-game overlay (experimental)", "pageIndex": 6 });
            }
        }
    }

    function openPage(index, title) {
        pageCarousel.currentIndex = index
        currentPage = index
        currentTitle = title
    }

    function goHome() {
        currentPage = -1
        currentTitle = "Settings"
    }

    Rectangle {
        id: appBar
        height: 56
        width: parent.width
        color: "#FF3700B3" // PrimaryMid
        z: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 16
            spacing: 8

            ToolButton {
                id: backButton
                visible: settingsPanel.currentPage >= 0
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                onClicked: settingsPanel.goHome()

                contentItem: Item {
                    Shape {
                        width: 24
                        height: 24
                        anchors.centerIn: parent
                        ShapePath {
                            fillColor: "white"
                            strokeWidth: 0
                            // Material "arrow back"
                            PathSvg { path: "M20,11V13H8L13.5,18.5L12.08,19.92L4.16,12L12.08,4.08L13.5,5.5L8,11H20Z" }
                        }
                    }
                }
            }

            Item {
                visible: !backButton.visible
                Layout.preferredWidth: 8
            }

            Text {
                text: settingsPanel.currentTitle
                font.pixelSize: 20
                color: "white"
                elide: Text.ElideRight
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    // Settings home
    ListView {
        id: homeList
        visible: settingsPanel.currentPage < 0
        anchors.top: appBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        model: settingsMenuModel

        delegate: ItemDelegate {
            width: homeList.width
            height: 64
            onClicked: settingsPanel.openPage(model.pageIndex, model.name)

            contentItem: RowLayout {
                spacing: 8

                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true

                    Label {
                        text: model.name
                        font.pixelSize: 16
                        Layout.fillWidth: true
                    }
                    Label {
                        text: model.description
                        font.pixelSize: 12
                        opacity: 0.65
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Label {
                    text: "›"
                    font.pixelSize: 22
                    opacity: 0.5
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 16
                height: 1
                color: "#33ffffff"
            }
        }
    }

    SwipeView {
        id: pageCarousel
        visible: settingsPanel.currentPage >= 0
        anchors.top: appBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: 0
        interactive: false
        clip: true

        SettingsWoWmapper {}
        SettingsDevices {}
        SettingsKeybindings {}
        SettingsAnalog {}
        SettingsTouchpad {}
        SettingsMemoryReading {}
        SettingsOverlay {}
    }
}
