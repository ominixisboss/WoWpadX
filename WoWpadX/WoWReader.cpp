#include "WoWReader.h"
#include "Log.h"
#include <algorithm>
#include <QString>

HWND WoWReader::windowHandle = nullptr;
std::atomic<uint32_t> WoWReader::atomicColor(0);
std::atomic<bool> WoWReader::synced(false);
std::atomic<bool> WoWReader::focused(false);
POINT WoWReader::beaconOffset = { 0, 0 };
POINT WoWReader::screenCenter = { 0, 0 };
POINT WoWReader::beaconCheckPoint = { 0, 0 };
std::atomic<bool> WoWReader::useWindowDC(false);
std::thread WoWReader::workerThread;
std::atomic<bool> WoWReader::threadRunning(false);
std::chrono::steady_clock::time_point WoWReader::lastSeenBeacon = std::chrono::steady_clock::now();
std::chrono::steady_clock::time_point WoWReader::lastHandleCheck = std::chrono::steady_clock::now();

bool WoWReader::init(HWND wowHwnd) {
    if (!wowHwnd || !IsWindow(wowHwnd)) return false;

    // If the thread is already running, just update the handle
    if (threadRunning) {
        windowHandle = wowHwnd;
        synced = false;
        return true;
    }

    windowHandle = wowHwnd;
    start();
    return true;
}

void WoWReader::close() {
    stop();
    windowHandle = nullptr;
    synced = false;
    focused = false;
}

void WoWReader::start() {
    if (threadRunning) return;
    threadRunning = true;
    workerThread = std::thread(&WoWReader::threadLoop);
}

void WoWReader::stop() {
    threadRunning = false;
    if (workerThread.joinable()) workerThread.join();
}

void WoWReader::updateWindowMetrics() {
    auto now = std::chrono::steady_clock::now();

    // 1. Validate the handle every 2 seconds or if it becomes invalid
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastHandleCheck).count() >= 2) {
        lastHandleCheck = now;
        if (!windowHandle || !IsWindow(windowHandle)) {
            synced = false;
            focused = false;
            // Handle is dead; the thread remains running but waits for ProcessManager
            // to call WoWReader::init again with the new HWND.
            return;
        }
    }

    // Cache focus state
    bool currentlyFocused = (GetForegroundWindow() == windowHandle);
    focused.store(currentlyFocused);

    // Cache center coordinates only if focused to save resources
    if (currentlyFocused || screenCenter.x == 0){
        RECT rect;
        if (GetClientRect(windowHandle, &rect)) {
            POINT pt = { rect.right / 2, rect.bottom / 2 };

            if (IsHungAppWindow(windowHandle)) {
                synced = false; // Immediately drop sync if game hangs
                return;
            }

            ClientToScreen(windowHandle, &pt);
            screenCenter = pt;
        }
    }
}

void WoWReader::threadLoop() {
    auto lastCalibrationTry = std::chrono::steady_clock::now();

    while (threadRunning) {
        if (!windowHandle || !IsWindow(windowHandle)) {
            synced = false;
            focused = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        updateWindowMetrics();
        auto now = std::chrono::steady_clock::now();

        // 1. If focused, try to sync or sample
        if (focused.load()) {
            if (!synced.load()) {
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCalibrationTry).count() >= 250) {
                    lastCalibrationTry = now;
                    if (findBeacon()) {
                        synced = true;
                        Log::writeLine("Pixel Bridge: Synced with client.");
                        lastSeenBeacon = now;
                    }
                    else {
                        // Explain why sync fails, at most every 5 seconds
                        static auto lastDiag = std::chrono::steady_clock::time_point{};
                        if (now - lastDiag >= std::chrono::seconds(5)) {
                            lastDiag = now;
                            logDiagnostics();
                        }
                    }
                }
            }
            else {
                sampleData();
                lastSeenBeacon = now;
            }
        }

        // 2. GLOBAL TIMEOUT (This runs even when Alt-Tabbed!)
        // If we haven't successfully sampled data in 3 seconds, drop sync.
        if (synced.load()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastSeenBeacon).count();
            if (elapsed >= 3) {
                synced = false;
                atomicColor.store(0);
                Log::writeLine("Pixel Bridge: Sync lost (timeout).");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
}

// Pixels are stored as 0x00RRGGBB (DIB order), not COLORREF
static inline uint8_t pxR(uint32_t c) { return (c >> 16) & 0xFF; }
static inline uint8_t pxG(uint32_t c) { return (c >> 8) & 0xFF; }
static inline uint8_t pxB(uint32_t c) { return c & 0xFF; }

static inline bool isMagentaPx(uint32_t c) { return pxR(c) > 200 && pxG(c) < 60 && pxB(c) > 200; }
static inline bool isBlackPx(uint32_t c) { return pxR(c) < 30 && pxG(c) < 30 && pxB(c) < 30; }

// Captures a region of the game's client area into 'out' (0x00RRGGBB, top-down).
// By default it reads the composited desktop (GetDC(NULL)), which works with DXVK,
// flip-model presentation and DPI-virtualized windows used by most 3.3.5a HD setups.
// The window DC path is kept as a fallback for older setups.
bool WoWReader::captureClient(int x, int y, int w, int h, bool fromWindowDC, std::vector<uint32_t>& out) {
    if (w <= 0 || h <= 0) return false;

    int srcX = x, srcY = y;
    HDC hSrc = nullptr;
    if (fromWindowDC) {
        hSrc = GetDC(windowHandle);
    }
    else {
        POINT origin = { x, y };
        if (!ClientToScreen(windowHandle, &origin)) return false;
        srcX = origin.x;
        srcY = origin.y;
        hSrc = GetDC(nullptr);
    }
    if (!hSrc) return false;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC hDC = CreateCompatibleDC(hSrc);
    HBITMAP hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    bool ok = false;

    if (hDC && hBitmap && bits) {
        HGDIOBJ oldObj = SelectObject(hDC, hBitmap);
        if (BitBlt(hDC, 0, 0, w, h, hSrc, srcX, srcY, SRCCOPY | CAPTUREBLT)) {
            GdiFlush();
            const uint32_t* px = static_cast<const uint32_t*>(bits);
            out.assign(px, px + (size_t)w * h);
            for (auto& c : out) c &= 0x00FFFFFF;
            ok = true;
        }
        SelectObject(hDC, oldObj);
    }

    if (hBitmap) DeleteObject(hBitmap);
    if (hDC) DeleteDC(hDC);
    ReleaseDC(fromWindowDC ? windowHandle : nullptr, hSrc);
    return ok;
}

bool WoWReader::findBeacon() {
    if (!windowHandle || !IsWindow(windowHandle) || IsHungAppWindow(windowHandle)) return false;

    RECT rc;
    if (!GetClientRect(windowHandle, &rc)) return false;

    // HD resolutions / UI scale make the beacon bigger, so search a larger area
    const int scanW = (std::min)((int)rc.right, 256);
    const int scanH = (std::min)((int)rc.bottom, 256);

    // Try screen capture first, then the legacy window DC capture
    for (int attempt = 0; attempt < 2; attempt++) {
        bool fromWindowDC = (attempt == 1);
        std::vector<uint32_t> img;
        if (!captureClient(0, 0, scanW, scanH, fromWindowDC, img)) continue;

        auto at = [&](int x, int y) { return img[(size_t)y * scanW + x]; };

        for (int y = 0; y < scanH; y++) {
            for (int x = 0; x < scanW; x++) {
                if (!isMagentaPx(at(x, y))) continue;

                // Measure the magenta block so we can sample the center of each block,
                // which is immune to edge blending from scaling/filtering.
                int bw = 0;
                while (x + bw < scanW && isMagentaPx(at(x + bw, y))) bw++;
                int bh = 0;
                while (y + bh < scanH && isMagentaPx(at(x, y + bh))) bh++;

                if (bw < 1 || bh < 1) continue;

                POINT beacon = { x + bw / 2, y + bh / 2 };
                // Data block is the same width, directly right of the beacon
                POINT data = { x + bw + bw / 2, y + bh / 2 };
                if (data.x >= rc.right) continue;

                beaconCheckPoint = beacon;
                beaconOffset = data;
                useWindowDC = fromWindowDC;

                Log::writeLine(QString("Pixel Bridge: Beacon %1x%2 at (%3,%4), capture=%5")
                    .arg(bw).arg(bh).arg(x).arg(y)
                    .arg(fromWindowDC ? "window" : "screen"));
                return true;
            }
        }
    }
    return false;
}

// Logs what the reader actually sees in the top-left corner, for both capture paths.
void WoWReader::logDiagnostics() {
    RECT rc = {};
    GetClientRect(windowHandle, &rc);
    wchar_t cls[64] = {};
    GetClassNameW(windowHandle, cls, 64);
    POINT origin = { 0, 0 };
    ClientToScreen(windowHandle, &origin);

    Log::writeLine(QString("Pixel Bridge diag: window class=%1 client=%2x%3 at (%4,%5) focused=%6")
        .arg(QString::fromWCharArray(cls)).arg(rc.right).arg(rc.bottom)
        .arg(origin.x).arg(origin.y).arg(focused.load()));

    const int w = (std::min)((int)rc.right, 64), h = (std::min)((int)rc.bottom, 32);
    for (int attempt = 0; attempt < 2; attempt++) {
        bool fromWindowDC = (attempt == 1);
        std::vector<uint32_t> img;
        if (!captureClient(0, 0, w, h, fromWindowDC, img)) {
            Log::writeLine(QString("  %1 capture failed").arg(fromWindowDC ? "window" : "screen"));
            continue;
        }

        // Most magenta-looking pixel, to see if the threshold is just missed
        int bestScore = -1000, bx = 0, by = 0;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                uint32_t c = img[(size_t)y * w + x];
                int score = (std::min)(pxR(c), pxB(c)) - pxG(c);
                if (score > bestScore) { bestScore = score; bx = x; by = y; }
            }

        QString row;
        for (int x = 0; x < (std::min)(w, 16); x++)
            row += QString("%1 ").arg(img[(size_t)(std::min)(2, h - 1) * w + x], 6, 16, QChar('0'));

        uint32_t best = img[(size_t)by * w + bx];
        Log::writeLine(QString("  %1: row y=2: %2| most magenta (%3,%4)=%5")
            .arg(fromWindowDC ? "window" : "screen").arg(row)
            .arg(bx).arg(by).arg(best, 6, 16, QChar('0')));
    }
}

void WoWReader::sampleData() {
    if (IsHungAppWindow(windowHandle)) return;

    // Capture one row spanning beacon center -> data center
    const int x0 = beaconCheckPoint.x;
    const int w = beaconOffset.x - beaconCheckPoint.x + 1;
    std::vector<uint32_t> row;
    if (!captureClient(x0, beaconCheckPoint.y, w, 1, useWindowDC.load(), row)) return;

    uint32_t bc = row.front();
    if (isMagentaPx(bc) || isBlackPx(bc)) {
        lastSeenBeacon = std::chrono::steady_clock::now();
        uint32_t c = row.back();
        // Keep the original COLORREF layout used by the interpreters
        atomicColor.store((uint32_t)RGB(pxR(c), pxG(c), pxB(c)));
    }
}

// Green channel state bits.
// New ConsolePortLK encodes bits in the high part of the channel (state * 32 + 16),
// which survives gamma/dithering/post-processing. Old builds sent the raw value (0-7).
uint8_t WoWReader::decodeStateBits(uint32_t color) {
    uint8_t g = GetGValue(color);
    if (g < 12) return g & 0x07;     // legacy encoding
    return (g >> 5) & 0x07;
}

// Lock-free interpreters for the InputWatcher
bool WoWReader::readGameState() { return synced.load() && (atomicColor.load() != 0); }
bool WoWReader::readMouselook() { return (decodeStateBits(atomicColor.load()) & 2) != 0; }
int  WoWReader::readMovementState() { return (decodeStateBits(atomicColor.load()) & 1) ? 1 : 0; }
bool WoWReader::readAoeState() { return GetBValue(atomicColor.load()) > 127; }
std::pair<long, long> WoWReader::readPlayerHealth() {
    uint32_t val = atomicColor.load();
    return { (long)GetRValue(val), 255 };
}
