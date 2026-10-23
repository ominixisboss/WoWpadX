cmake_minimum_required(VERSION 3.21)
project(AzerothPadHD LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network)
find_package(SDL3 REQUIRED CONFIG)

qt_add_executable(AzerothPadHD WIN32
    AppSettings.cpp BindManager.cpp ControllerManager.cpp DevicesController.cpp
    InputMapper.cpp Log.cpp main.cpp MainPageController.cpp MemoryReadingController.cpp
    OverlayClient.cpp OverlayLoader.cpp ProcessManager.cpp TrayManager.cpp WoWReader.cpp
    AppSettings.h BindManager.h ControllerManager.h DevicesController.h InputMapper.h
    KeybindDefaults.h Log.h MainPageController.h MemoryReadingController.h OverlayClient.h
    OverlayLoader.h ProcessManager.h TrayManager.h WoWReader.h Gamepad/gamepad.h
    qml.qrc resources.rc
)

target_include_directories(AzerothPadHD PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(AzerothPadHD PRIVATE UNICODE _UNICODE NOMINMAX)
target_compile_options(AzerothPadHD PRIVATE /utf-8 /permissive-)
target_link_libraries(AzerothPadHD PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Network
    SDL3::SDL3 user32 gdi32 shell32 shcore)
