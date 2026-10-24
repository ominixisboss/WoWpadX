// ControllerManager.cpp

#include "AppSettings.h"
#include "OverlayClient.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <chrono>
#include <string>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QDebug>

#include "ControllerManager.h"
#include "Log.h"

ControllerManager* ControllerManager::m_instance = nullptr;

#include <windows.h>
#include <TlHelp32.h>

// DSX (DualSenseX) support.
// DSX reads the DualSense itself and exposes a virtual Xbox 360 or DualShock 4
// controller. When it's active we leave the real DualSense to DSX (so we don't
// fight it over the lightbar, triggers and rumble) and use the virtual pad.
static bool s_dsxActive = false;

static bool IsDsxRunning()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, L"DSX.exe") == 0 ||
                _wcsicmp(entry.szExeFile, L"DualSenseX.exe") == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

// Picks the controller to use when none is selected.
// With DSX active, prefer its virtual controller over the raw DualSense.
static SDL_Gamepad* PickPreferredController(const std::vector<SDL_Gamepad*>& pads)
{
    if (pads.empty()) return nullptr;
    if (s_dsxActive) {
        for (auto* pad : pads) {
            if (SDL_GetGamepadType(pad) != SDL_GAMEPAD_TYPE_PS5)
                return pad;
        }
    }
    return pads.front();
}

bool ControllerManager::IsDsxActive() { return s_dsxActive; }

ControllerManager* ControllerManager::instance() {
    if (!m_instance)
        m_instance = new ControllerManager();
    return m_instance;
}


void ControllerManager::HandleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_GAMEPAD_ADDED) {
            SDL_Gamepad* pad = SDL_OpenGamepad(e.gdevice.which);
            if (pad) {
                controllers.push_back(pad); 

                emit controllersChanged();
            }
        }
        else if (e.type == SDL_EVENT_GAMEPAD_REMOVED) {
            for (auto it = controllers.begin(); it != controllers.end(); ++it) {
                if (SDL_GetGamepadID(*it) == e.gdevice.which) {
                    if (*it == activeController) {
                        SetController(nullptr, -1);
                    }
                    const char* name = SDL_GetGamepadName(*it);
                    std::string controllerName = name ? name : "Unknown Controller";

                    SDL_CloseGamepad(*it);
                    OverlayClient::SendNotification("Controller disconnected", controllerName + " was disconnected.");
                    controllers.erase(it);
                     
                    emit controllersChanged(); 

                    break;
                }
            }
        }

        // Touchpad input is polled directly by InputMapper (see UpdateTouchpad)
    }
}


QString ControllerManager::GetButtonIcon(GamepadBinding button)
{
    QString type;
    QString name;

    if (this->activeController != NULL)
    {
        const char* padName = SDL_GetGamepadName(activeController);
        name = padName ? padName : "";
    }

    if (name.contains("DualShock", Qt::CaseInsensitive) || name.contains("DualSense") || name.contains("DS4") || name.contains("DS3") || name.contains("PS4") || name.contains("Playstation") || name.contains("PS3"))
        type = "DS";
    else if (name.contains("Xbox", Qt::CaseInsensitive))
        type = "Xbox";
    else if (name.contains("Switch", Qt::CaseInsensitive) || name.contains("Joy-Con", Qt::CaseInsensitive))
        type = "NS";
    else if (name.contains("Steam", Qt::CaseInsensitive) || name.contains("Deck", Qt::CaseInsensitive))
        type = "Deck";
    else
        type = "Xbox";

    switch (button)
    {
    case GamepadBinding::DPadUp:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_UP.png").arg(type);
    case GamepadBinding::DPadDown:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_DOWN.png").arg(type);

    case GamepadBinding::DPadLeft:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_LEFT.png").arg(type);
    case GamepadBinding::DPadRight:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_RIGHT.png").arg(type);
    case GamepadBinding::North:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_UP.png").arg(type);
    case GamepadBinding::South:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_DOWN.png").arg(type);

    case GamepadBinding::West:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_LEFT.png").arg(type);
    case GamepadBinding::East:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_RIGHT.png").arg(type);
    case GamepadBinding::Back:
        return QString("qrc:/Controllers/%1/Buttons/CP_X_LEFT.png").arg(type);
    case GamepadBinding::Start:
        return QString("qrc:/Controllers/%1/Buttons/CP_X_RIGHT.png").arg(type);

    case GamepadBinding::Guide:
        return QString("qrc:/Controllers/%1/Buttons/CP_X_CENTER.png").arg(type);
    case GamepadBinding::Misc1:
        return QString("qrc:/Controllers/%1/Buttons/CP_X_MISC1.png").arg(type); 
    case GamepadBinding::LeftStick:
        return QString("qrc:/Controllers/%1/Buttons/CP_T_L3.png").arg(type);
    case GamepadBinding::RightStick:
        return QString("qrc:/Controllers/%1/Buttons/CP_T_R3.png").arg(type);
    case GamepadBinding::LeftShoulder:
        return QString("qrc:/Controllers/%1/Buttons/CP_TL1.png").arg(type);
    case GamepadBinding::LeftTrigger:
        return QString("qrc:/Controllers/%1/Buttons/CP_TL2.png").arg(type);
    case GamepadBinding::RightShoulder:
        return QString("qrc:/Controllers/%1/Buttons/CP_TR1.png").arg(type);
    case GamepadBinding::RightTrigger:
        return QString("qrc:/Controllers/%1/Buttons/CP_TR2.png").arg(type);
    case GamepadBinding::LeftPaddle1:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_GRIP1.png").arg(type);
    case GamepadBinding::LeftPaddle2:
        return QString("qrc:/Controllers/%1/Buttons/CP_L_GRIP2.png").arg(type);
    case GamepadBinding::RightPaddle1:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_GRIP1.png").arg(type);
    case GamepadBinding::RightPaddle2:
        return QString("qrc:/Controllers/%1/Buttons/CP_R_GRIP2.png").arg(type);

    }
}

QString ControllerManager::GetButtonName(GamepadBinding button)
{
    QString type;
    QString name;

    if (this->activeController != NULL)
    {
        const char* padName = SDL_GetGamepadName(activeController);
        name = padName ? padName : "";
    }

    if (name.contains("DualShock", Qt::CaseInsensitive) || name.contains("DualSense") || name.contains("DS4") || name.contains("DS3") || name.contains("PS4") || name.contains("Playstation") || name.contains("PS3"))
        type = "DS";
    else if (name.contains("Xbox", Qt::CaseInsensitive))
        type = "Xbox";
    else if (name.contains("Switch", Qt::CaseInsensitive) || name.contains("Joy-Con", Qt::CaseInsensitive))
        type = "NS";
    else if (name.contains("Steam", Qt::CaseInsensitive) || name.contains("Deck", Qt::CaseInsensitive))
        type = "Deck";
    else
        type = "Xbox";

    switch (button)
    {
    case GamepadBinding::DPadUp:
        return "D-pad Up";
    case GamepadBinding::DPadDown:
        return "D-pad Down";
    case GamepadBinding::DPadLeft:
        return "D-pad Left";
    case GamepadBinding::DPadRight:
        return "D-pad Right";
    case GamepadBinding::North:
        return QMap<QString, QString>{{"Xbox", "Y"}, { "Deck", "Y" }, { "NS", "X" }}.value(type, "Triangle");
    case GamepadBinding::South:
        return QMap<QString, QString>{{"Xbox", "A"}, { "Deck", "A" }, { "NS", "B" }}.value(type, "Cross");
    case GamepadBinding::West:
        return QMap<QString, QString>{{"Xbox", "X"}, { "Deck", "X" }, { "NS", "Y" }}.value(type, "Square");
    case GamepadBinding::East:
        return QMap<QString, QString>{{"Xbox", "B"}, { "Deck", "B" }, { "NS", "A" }}.value(type, "Circle");
    case GamepadBinding::Back:
        return QMap<QString, QString>{{"Xbox", "Back"}, { "Deck", "Back" }, { "NS", "Minus" }}.value(type, "Share");
    case GamepadBinding::Start:
        return QMap<QString, QString>{{"Xbox", "Start"}, { "Deck", "Start" }, { "NS", "Plus" }}.value(type, "Options");
    case GamepadBinding::Guide:
        return QMap<QString, QString>{{"Xbox", "Guide"}, { "Deck", "Guide" }, { "NS", "Home" }}.value(type, "PS");
    case GamepadBinding::Misc1:
        return QMap<QString, QString>{{"Xbox", "Share"}, { "Deck", "Capture" }, { "NS", "Capture" }}.value(type, "Microphone");
    case GamepadBinding::LeftStick:
        return "Left Stick";
    case GamepadBinding::RightStick:
        return "Right Stick";
    case GamepadBinding::LeftShoulder:
        return QMap<QString, QString>{{"Xbox", "Left Bumper"}, { "Deck", "L1" }, { "NS", "L" }}.value(type, "L1");
    case GamepadBinding::LeftTrigger:
        return QMap<QString, QString>{{"Xbox", "Left Trigger"}, { "Deck", "L2" }, { "NS", "ZL" }}.value(type, "L2");
    case GamepadBinding::RightShoulder:
        return QMap<QString, QString>{{"Xbox", "Right Bumper"}, { "Deck", "R1" }, { "NS", "R" }}.value(type, "R1");
    case GamepadBinding::RightTrigger:
        return QMap<QString, QString>{{"Xbox", "Right Trigger"}, { "Deck", "R2" }, { "NS", "ZR" }}.value(type, "R2");

    case GamepadBinding::LeftPaddle1:
        return QMap<QString, QString>{{ "Deck", "L4" }}.value(type, "Left Grip 1"); 
    case GamepadBinding::LeftPaddle2:
        return QMap<QString, QString>{{ "Deck", "L5" }}.value(type, "Left Grip 2");
    case GamepadBinding::RightPaddle1:
        return QMap<QString, QString>{{ "Deck", "R4" }}.value(type, "Right Grip 1");
    case GamepadBinding::RightPaddle2:
        return QMap<QString, QString>{{ "Deck", "R5" }}.value(type, "Right Grip 2");

    }
}

int ControllerManager::GetBatteryLevel(SDL_Gamepad* pad) {
    if (!pad) return 100;

    int percentage = 0;
    SDL_PowerState state = SDL_GetGamepadPowerInfo(pad, &percentage);

    // If the state is unknown, return 100 to avoid false "low battery" alarms
    if (state == SDL_POWERSTATE_UNKNOWN) {
        return 100;
    }
    if (percentage == -1) {
        return 100;
    }

    return percentage;
}

void ControllerManager::SetController(SDL_Gamepad* controller, SDL_JoystickID instanceId) {
    activeController = controller;
    activeInstanceId = instanceId;
    
    emit activeControllerChanged();

    if (activeController != NULL)
    {
        std::string controllerName = SDL_GetGamepadName(activeController);
        Log::writeLine("Selected controller: " + QString(controllerName.c_str()));
        OverlayClient::SendNotification("Controller selected", "Controller " + controllerName + " is now the active controller.");
    }
}

void ControllerManager::startWatcher() {
    if (threadRunning) return;

    SDL_SetHint("SDL_HINT_NO_SIGNAL_HANDLERS", "1");

    int dsxMode = AppSettings::instance()->dsxMode(); // 0 = auto, 1 = always, 2 = never
    s_dsxActive = (dsxMode == 1) || (dsxMode == 0 && IsDsxRunning());
    if (s_dsxActive) {
        // Don't open the DualSense through HIDAPI; DSX owns it
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "0");
        Log::writeLine("DSX detected: using its virtual controller. Set DSX to DualShock 4 emulation to use the touchpad.");
    }
    SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);

    threadRunning = true; 
    watcherThread = std::thread([this]() { ControllerWatcher(); });


}

void ControllerManager::stopWatcher() {
    threadRunning = false;
    if (watcherThread.joinable()) {
        watcherThread.join();
    }
    for (auto& pad : controllers) {
        SDL_CloseGamepad(pad);
    }
    controllers.clear();
    activeController = nullptr;
    activeInstanceId = -1;
    SDL_Quit();
}



void ControllerManager::ControllerWatcher() {
    while (threadRunning) {
        {
            std::lock_guard<std::mutex> lock(controllerMutex);

            HandleEvents();

            if (!activeController && !controllers.empty()) {
                SDL_Gamepad* preferred = PickPreferredController(controllers);
                SetController(preferred, SDL_GetGamepadID(preferred));
            }

            if (AppSettings::instance()->enableOverlay() && AppSettings::instance()->enableOverlayBattery() && activeController) {
                int battery = GetBatteryLevel(activeController);
                if (battery > lastBatteryWarn) {
                    lastBatteryWarn = 100;
                    if (battery > 35) showedBatteryCritical = false;
                    if (battery > 45) showedBatteryLow = false;
                }
                else if (battery <= lastBatteryWarn - 10) {
                    if (battery <= 40 && !showedBatteryLow) {
                        OverlayClient::SendNotification("Battery low", "Your controller battery has reached " + std::to_string(battery) + "%.", 0 /* "LOW_BATTERY" */);
                        showedBatteryLow = true;
                    }
                    else if (battery <= 30 && !showedBatteryCritical) {
                        OverlayClient::SendNotification("Battery critically low", "Your controller battery has reached " + std::to_string(battery) + "%. Plug in now to avoid interruption.", 0 /* "LOW_BATTERY" */);
                        showedBatteryCritical = true;
                    }
                    lastBatteryWarn -= 10;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

SDL_Gamepad* ControllerManager::getActiveController() {
    return activeController;
}

bool ControllerManager::Rumble(uint16_t low_freq, uint16_t high_freq, uint32_t duration_ms) {
    if (!activeController) return false;
    return SDL_RumbleGamepad(activeController, low_freq, high_freq, duration_ms);
}

const std::vector<SDL_Gamepad*>& ControllerManager::getControllers() {
    return controllers;
}
