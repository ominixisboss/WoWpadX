import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import QtQuick.Controls.Material
import WoWpadX 1.0

// Pixel Bridge settings: one page, grouped into sections.
Item {
    id: settingsMemoryReading
    width: 313
    height: 500

    component SectionHeader: ColumnLayout {
        property alias text: title.text
        spacing: 4
        Layout.fillWidth: true
        Layout.topMargin: 8
        Label { id: title; font.pixelSize: 16 }
        Rectangle { height: 1; Layout.fillWidth: true; color: "#ccc" }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: settingsMemoryReading.width
            spacing: 8

            ColumnLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: settingsMemoryReading.width - 20
                Layout.margins: 10
                spacing: 8

                // --- Enable ---
                CheckBox {
                    text: "Enable Pixel Bridge"
                    font.pixelSize: 15
                    checked: AppSettings.enableMemoryReading
                    onToggled: AppSettings.enableMemoryReading = checked
                }

                Label {
                    text: "Reads a tiny marker drawn by ConsolePortLK to know your health, mouselook and AoE state. Turn on Pixel Bridge in the addon too, and play in windowed or windowed fullscreen."
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }

                Label {
                    text: "Learn more"
                    font.pixelSize: 12
                    color: "cornflowerblue"
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: MemoryReadingController.textMoreInfoClicked()
                    }
                }

                ColumnLayout {
                    enabled: AppSettings.enableMemoryReading
                    opacity: enabled ? 1 : 0.5
                    spacing: 6
                    Layout.fillWidth: true

                    // --- Mouse & Camera ---
                    SectionHeader { text: "Mouse & Camera" }

                    CheckBox {
                        text: "Re-center cursor after mouselook"
                        checked: AppSettings.memoryAutoCenter
                        onToggled: AppSettings.memoryAutoCenter = checked

                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: "Moves the cursor to the center of the screen once mouselook has been active for the time below."
                    }

                    RowLayout {
                        enabled: AppSettings.memoryAutoCenter
                        Layout.fillWidth: true
                        Layout.leftMargin: 32
                        spacing: 8
                        Label { text: "After" }
                        Slider {
                            from: 0
                            to: 10000
                            stepSize: 100
                            value: AppSettings.memoryAutoCenterDelay
                            onMoved: AppSettings.memoryAutoCenterDelay = value
                            Layout.fillWidth: true
                        }
                        Label {
                            text: (AppSettings.memoryAutoCenterDelay / 1000).toFixed(1) + " s"
                            Layout.preferredWidth: 40
                        }
                    }

                    CheckBox {
                        text: "Cancel mouselook when alt-tabbed"
                        checked: AppSettings.memoryAutoCancel
                        onToggled: AppSettings.memoryAutoCancel = checked
                    }

                    CheckBox {
                        text: "Invert left/right turn in mouselook"
                        checked: AppSettings.memoryInvertTurn
                        onToggled: AppSettings.memoryInvertTurn = checked
                    }

                    CheckBox {
                        text: "Disable touchpad during mouselook"
                        checked: AppSettings.memoryTouchpadCursorOnly
                        onToggled: AppSettings.memoryTouchpadCursorOnly = checked
                    }

                    CheckBox {
                        text: "Show cursor position on overlay"
                        visible: AppSettings.enableOverlay
                        checked: AppSettings.enableOverlayCrosshair
                        onToggled: AppSettings.enableOverlayCrosshair = checked

                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: "Shows a crosshair on the overlay at the cursor position while mouselooking."
                    }

                    // --- Movement ---
                    SectionHeader { text: "Movement" }

                    CheckBox {
                        text: "Auto switch walk / run"
                        checked: AppSettings.memoryAutoWalk
                        onToggled: AppSettings.memoryAutoWalk = checked

                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: "Walks when the stick is pushed less than the threshold below, runs above it."
                    }

                    RowLayout {
                        enabled: AppSettings.memoryAutoWalk
                        Layout.fillWidth: true
                        Layout.leftMargin: 32
                        spacing: 8
                        Label { text: "Run at" }
                        Slider {
                            from: 35
                            to: 120
                            stepSize: 1
                            value: AppSettings.walkThreshold
                            onMoved: AppSettings.walkThreshold = value
                            Layout.fillWidth: true
                        }
                        Label {
                            text: AppSettings.walkThreshold + " %"
                            Layout.preferredWidth: 40
                        }
                    }

                    // --- AoE Spells ---
                    SectionHeader { text: "AoE Spells" }

                    CheckBox {
                        text: "Place AoE spells with controller buttons"
                        checked: AppSettings.memoryOverrideAoeCast
                        onToggled: AppSettings.memoryOverrideAoeCast = checked

                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: "While placing a targeted AoE spell, these buttons confirm or cancel it instead of mouse clicks."
                    }

                    GridLayout {
                        enabled: AppSettings.memoryOverrideAoeCast
                        columns: 2
                        columnSpacing: 8
                        rowSpacing: 4
                        Layout.fillWidth: true
                        Layout.leftMargin: 32

                        Label { text: "Cast" }
                ComboBox {
                    id: aoeConfirm
                    Layout.fillWidth: true
                    model: MemoryReadingController.aoeConfirmModel
                    currentIndex: MemoryReadingController.aoeConfirmIndex
                    textRole: "label"
                    contentItem: RowLayout {
                        spacing: 5
                        Image {
                            source: MemoryReadingController.aoeConfirmModel.get(aoeConfirm.currentIndex).buttonImg
                            width: 18; height: 18
                        }
                        Label {
                            text: MemoryReadingController.aoeConfirmModel.get(aoeConfirm.currentIndex).label
                        }
                    }
                    delegate: ItemDelegate {
                        width: parent.width
                        RowLayout {
                            spacing: 5
                            Image {
                                source: buttonImg
                                width: 18; height: 18
                            }
                            Label { text: label }
                        }
                    }
                    onCurrentIndexChanged: { 
                        MemoryReadingController.aoeOverrideChanged(currentIndex, aoeCancel.currentIndex)
                    }
                }

                        Label { text: "Cancel" }
                ComboBox {
                    id: aoeCancel
                    Layout.fillWidth: true
                    model: MemoryReadingController.aoeCancelModel
                    currentIndex: MemoryReadingController.aoeCancelIndex
                    textRole: "label"
                    contentItem: RowLayout {
                        spacing: 5
                        Image {
                            source: MemoryReadingController.aoeCancelModel.get(aoeCancel.currentIndex).buttonImg
                            width: 18; height: 18
                        }
                        Label {
                            text: MemoryReadingController.aoeCancelModel.get(aoeCancel.currentIndex).label
                        }
                    }
                    delegate: ItemDelegate {
                        width: parent.width
                        RowLayout {
                            spacing: 5
                            Image {
                                source: buttonImg
                                width: 18; height: 18
                            }
                            Label { text: label }
                        }
                    }
                    onCurrentIndexChanged: MemoryReadingController.aoeOverrideChanged(aoeConfirm.currentIndex, currentIndex)
                }
                    }

                    // --- Rumble ---
                    SectionHeader { text: "Rumble" }

                    CheckBox {
                        text: "Vibrate when taking damage"
                        checked: AppSettings.memoryVibrationDamage
                        onToggled: AppSettings.memoryVibrationDamage = checked

                        ToolTip.visible: hovered
                        ToolTip.delay: 500
                        ToolTip.text: "Vibrates the controller based on how much damage your character takes."
                    }
                }
            }
        }
    }
}
