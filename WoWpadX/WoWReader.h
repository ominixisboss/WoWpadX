#pragma once

#include <windows.h>
#include <chrono>
#include <utility>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>

class WoWReader {
public:
    static bool init(HWND wowHwnd);
    static void close();
    static void start();
    static void stop();

    // Interpreters (Lock-free for InputWatcher)
    static bool readGameState();
    static bool readMouselook();
    static int  readMovementState();
    static bool readAoeState();
    static std::pair<long, long> readPlayerHealth();

    static bool isAttached() { return windowHandle != nullptr; }

    // Thread-safe state getters
    static bool isSynced() { return synced.load(); }
    static bool isFocused() { return focused.load(); }
    static HWND getWindowHandle() { return windowHandle; }
    static POINT getScreenCenter() { return screenCenter; }

private:
    static HWND windowHandle;
    static std::atomic<uint32_t> atomicColor;
    static std::atomic<bool> synced;
    static std::atomic<bool> focused;
    static POINT beaconOffset;      // Data block center (client coords)
    static POINT beaconCheckPoint;  // Beacon block center (client coords)
    static std::atomic<bool> useWindowDC; // Fallback capture path for clients where screen capture fails
    static POINT screenCenter;

    static std::thread workerThread;
    static std::atomic<bool> threadRunning;
    static std::chrono::steady_clock::time_point lastSeenBeacon; 
    static std::chrono::steady_clock::time_point lastHandleCheck;

    static void threadLoop();
    static bool findBeacon();
    static void sampleData();
    static void updateWindowMetrics();
    static void logDiagnostics();
    static bool captureClient(int x, int y, int w, int h, bool fromWindowDC, std::vector<uint32_t>& out);
    static uint8_t decodeStateBits(uint32_t color);
};