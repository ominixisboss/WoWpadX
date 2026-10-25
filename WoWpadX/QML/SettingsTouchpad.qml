import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import WoWpadX 1.0

// Touchpad, stick clicks and DSX: everything specific to PlayStation-style pads.
Item {
    id: settingsTouchpad
    width: 313
    height: 421

    ScrollView {
        anchors.fill: parent
        contentWidth: parent.width
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Touchpad
        Label { text: "Touchpad"; font.pixelSize: 16 }
        Rectangle { height: 1; width: parent.width - 12; color: "#ccc" }

        CheckBox {
            text: "Enable touchpad"
            enabled: true
            checked: AppSettings.enableTouchpad
            onToggled: AppSettings.enableTouchpad = checked

            ToolTip.visible: hovered
            ToolTip.delay: 500 
            ToolTip.text: "Enables or disables your controller touchpad (if present)"
        }

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Layout.rightMargin: 12
            Label { text: "Clicking the touchpad"; opacity: 0.8 }

            ComboBox {
                enabled: AppSettings.enableTouchpad
                model: ["Left/Right Click", "Share/Options", "Buttons: left LG, right RG"]
                currentIndex: AppSettings.touchpadMode
                onCurrentIndexChanged: AppSettings.touchpadMode = currentIndex
                Layout.fillWidth: true

                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: "Separate buttons: clicking the left half of the touchpad is LG, the right half is RG"
            }
        }

        // Stick clicks
        Label { text: "Stick Buttons (L3 / R3)"; font.pixelSize: 16 }
        Rectangle { height: 1; width: parent.width - 12; color: "#ccc" }

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Layout.rightMargin: 12
            Label { text: "Pressing the sticks"; opacity: 0.8 }

            ComboBox {
                model: ["Left/Right Click", "Buttons: L3 LG2, R3 RG2"]
                currentIndex: AppSettings.stickClickMode
                onCurrentIndexChanged: AppSettings.stickClickMode = currentIndex
                Layout.fillWidth: true

                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: "Separate buttons: L3 and R3 become their own bindable buttons (second grip slots in ConsolePortLK)"
            }
        }

        // DSX
        Label { text: "DSX (DualSenseX)"; font.pixelSize: 16 }
        Rectangle { height: 1; width: parent.width - 12; color: "#ccc" }

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Layout.rightMargin: 12
            Label { text: "DSX mode (applies after restart)"; opacity: 0.8 }

            ComboBox {
                model: ["Auto-detect", "Always on", "Off"]
                currentIndex: AppSettings.dsxMode
                onCurrentIndexChanged: AppSettings.dsxMode = currentIndex
                Layout.fillWidth: true

                ToolTip.visible: hovered
                ToolTip.delay: 500
                ToolTip.text: "When DSX is running, use its virtual controller and leave the DualSense to DSX. Takes effect after restarting AzerothPad HD."
            }
        }

    } }
}
