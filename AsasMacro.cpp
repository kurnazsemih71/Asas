// ==== AsasMacro_Pro_v15_Precision.cpp ====
// Hassas Milisaniye Kontrollü, Piksel Kusursuzluğunda Mimari (F1-F8 Sayfa Korumalı + Ping Toleranslı)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <random> // Rastgele sayı (Ping Toleransı) kütüphanesi eklendi

// --- Global Değişkenler ---
bool g_exit = false;
bool g_isMasterActive = false;
bool g_isCapsOn = false;
bool g_isMiniMode = false;
bool g_isTopMost = false;

HFONT g_hFont, g_hBoldFont, g_hMiniListFont;
HBRUSH g_hbrMainBg, g_hbrCardBg, g_hbrBlack, g_hbrTopBar, g_hbrMiniBg, g_hbrMiniListBg;

std::thread g_comboThread, g_hpThread, g_mpThread, g_minorThread;
std::mutex g_inputMutex;
std::chrono::system_clock::time_point g_sessionStartTime;

// UI Kontrol ID'leri
#define IDC_BTN_TOP     101
#define IDC_BTN_MAST    102
#define IDC_BTN_TOMINI  103
#define IDC_BTN_MIN     104
#define IDC_BTN_CLOSE   105
#define IDC_BTN_MINI_ON 401
#define IDC_BTN_MINI_OFF 402
#define IDC_BTN_MINI_EXP 403
#define IDC_EDT_S1      201
#define IDC_EDT_S2      202
#define IDC_EDT_S3      203
#define IDC_EDT_S4      204
#define IDC_EDT_S5      205
#define IDC_EDT_MS      206
#define IDC_EDT_HP      207
#define IDC_EDT_MP      208
#define IDC_EDT_MIN     209
#define IDC_EDT_SK_P    210
#define IDC_EDT_SK_W    211
#define IDC_EDT_R_P     212
#define IDC_EDT_R_W     213

std::vector<HWND> g_normalControls;
std::vector<HWND> g_miniControls;

HWND g_hTopTitle, g_hTopBtn, g_hMasterBtn, g_hStatusLabel;
HWND g_hEdits[5], g_hMsEdit, g_hHpEdit, g_hMpEdit, g_hMinorEdit;
HWND g_hSkillPressEdit, g_hSkillWaitEdit, g_hRPressEdit, g_hRWaitEdit;
HWND g_hMiniTitle, g_hMiniList, g_hMiniTimer;

std::vector<WORD> g_skills;
int g_delayMs = 350;
WORD g_hpKey = 0, g_mpKey = 0, g_minorKey = 0;

// Hassas Ayar Değişkenleri
int g_skillPressMs = 25;
int g_skillWaitMs = 5;
int g_rPressMs = 20;
int g_rWaitMs = 20;

// =========================================================
// RASTGELE GECİKME ÜRETİCİ (PING TOLERANSI)
// =========================================================
int GetRandomDelay(int minDelay, int maxDelay) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(minDelay, maxDelay);
    return distr(gen);
}

// =========================================================
// DONANIM SEVİYESİ TUŞ BASMA & DÖNGÜLER
// =========================================================
void SendKey(WORD keyCode, int pressDelay = 15) {
    if (keyCode == 0) return;
    std::lock_guard<std::mutex> lock(g_inputMutex);
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = MapVirtualKeyW(keyCode, MAPVK_VK_TO_VSC);
    input.ki.wVk = 0;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &input, sizeof(INPUT));

    // Tuşa basılı tutma süresi de rastgelelik içerir
    std::this_thread::sleep_for(std::chrono::milliseconds(pressDelay));

    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void ReadSettings() {
    wchar_t buf[10];
    g_skills.clear();
    for (int i = 0; i < 5; ++i) {
        GetWindowTextW(g_hEdits[i], buf, 10);
        if (wcslen(buf) > 0) g_skills.push_back(VkKeyScanW(buf[0]));
    }
    GetWindowTextW(g_hMsEdit, buf, 10); g_delayMs = _wtoi(buf); if (g_delayMs < 50) g_delayMs = 50;
    GetWindowTextW(g_hHpEdit, buf, 10); g_hpKey = wcslen(buf) > 0 ? VkKeyScanW(buf[0]) : 0;
    GetWindowTextW(g_hMpEdit, buf, 10); g_mpKey = wcslen(buf) > 0 ? VkKeyScanW(buf[0]) : 0;
    GetWindowTextW(g_hMinorEdit, buf, 10); g_minorKey = wcslen(buf) > 0 ? VkKeyScanW(buf[0]) : 0;

    GetWindowTextW(g_hSkillPressEdit, buf, 10); g_skillPressMs = _wtoi(buf); if (g_skillPressMs < 1) g_skillPressMs = 1;
    GetWindowTextW(g_hSkillWaitEdit, buf, 10); g_skillWaitMs = _wtoi(buf); if (g_skillWaitMs < 1) g_skillWaitMs = 1;
    GetWindowTextW(g_hRPressEdit, buf, 10); g_rPressMs = _wtoi(buf); if (g_rPressMs < 1) g_rPressMs = 1;
    GetWindowTextW(g_hRWaitEdit, buf, 10); g_rWaitMs = _wtoi(buf); if (g_rWaitMs < 1) g_rWaitMs = 1;
}

// GÜNCELLENDİ: F1'den F8'e kadar tüm tuşları kapsar
bool IsUserChangingPage() {
    for (int i = VK_F1; i <= VK_F8; ++i) {
        if (GetAsyncKeyState(i) & 0x8000) return true;
    }
    return false;
}

// GÜNCELLENDİ: F1-F8 taraması
bool SmartSleep(int totalMs) {
    int waited = 0;
    while (waited < totalMs) {
        if (!g_isMasterActive || !g_isCapsOn || g_exit) return true;
        if (IsUserChangingPage()) return true; // Uyku sırasında F'e basılırsa uykuyu böl
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        waited += 10;
    }
    return false;
}

// GÜNCELLENDİ: Otomatik "Durdur-Başlat" ve "Rastgele Gecikme" Mantığı Entegre Edildi
void ComboThread() {
    while (!g_exit) {
        if (g_isMasterActive && g_isCapsOn) {

            // 1. Dış Kontrol: Yeni bir döngüye girmeden önce F1-F8'e basıldı mı?
            if (IsUserChangingPage()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(150, 200)));
                continue;
            }

            bool interrupted = false;

            // 2. İç Kontrol: Skilleri basarken aniden F1-F8'e basıldı mı?
            for (WORD skill : g_skills) {
                if (!g_isMasterActive || !g_isCapsOn || g_exit) break;

                // Skill vurulurken yakalanırsa anında kır (Sanal Durdurma)
                if (IsUserChangingPage()) {
                    interrupted = true;
                    break;
                }

                // GÜNCELLEME: İnsansı skill basma süresi
                SendKey(skill, GetRandomDelay(g_skillPressMs, g_skillPressMs + 12));

                // GÜNCELLEME: Skill sonrası insansı bekleme (Sunucu Ping Toleransı)
                std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(g_skillWaitMs, g_skillWaitMs + 15)));
            }

            if (interrupted || !g_isMasterActive || !g_isCapsOn || g_exit) {
                if (interrupted) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(150, 220)));
                }
                continue;
            }

            // Kesinti olmadıysa komboya (R) devam et (Rastgele gecikmeler eklendi)
            SendKey('R', GetRandomDelay(g_rPressMs, g_rPressMs + 10));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(g_rWaitMs, g_rWaitMs + 20)));
            SendKey('R', GetRandomDelay(g_rPressMs, g_rPressMs + 10));

            // Ana döngü beklemesi de rastgeleleştirildi
            SmartSleep(GetRandomDelay(g_delayMs, g_delayMs + 35));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void HpThread() {
    while (!g_exit) {
        if (g_isMasterActive && (GetAsyncKeyState('F') & 0x8000)) {
            SendKey(g_hpKey, GetRandomDelay(12, 25));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(140, 180)));
        }
        else { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
    }
}

void MpThread() {
    while (!g_exit) {
        if (g_isMasterActive && (GetAsyncKeyState(VK_XBUTTON1) & 0x8000)) {
            SendKey(g_mpKey, GetRandomDelay(12, 25));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(140, 180)));
        }
        else { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
    }
}

void MinorThread() {
    while (!g_exit) {
        if (g_isMasterActive && (GetAsyncKeyState(VK_XBUTTON2) & 0x8000)) {
            SendKey(g_minorKey, GetRandomDelay(4, 10));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(4, 10)));
            SendKey(g_minorKey, GetRandomDelay(4, 10));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(12, 25)));
        }
        else { std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    }
}

// =========================================================
// ARAYÜZ MİMARİSİ
// =========================================================
void ToggleMiniMode(HWND hwnd) {
    g_isMiniMode = !g_isMiniMode;
    for (HWND h : g_normalControls) ShowWindow(h, g_isMiniMode ? SW_HIDE : SW_SHOW);
    for (HWND h : g_miniControls) ShowWindow(h, g_isMiniMode ? SW_SHOW : SW_HIDE);

    if (g_isMiniMode) { SetWindowPos(hwnd, NULL, 0, 0, 260, 132, SWP_NOMOVE | SWP_NOZORDER); }
    else { SetWindowPos(hwnd, NULL, 0, 0, 280, 340, SWP_NOMOVE | SWP_NOZORDER); }
    InvalidateRect(hwnd, NULL, TRUE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hbrMainBg = CreateSolidBrush(RGB(11, 15, 25));
        g_hbrTopBar = CreateSolidBrush(RGB(15, 20, 30));
        g_hbrCardBg = CreateSolidBrush(RGB(22, 30, 46));
        g_hbrBlack = CreateSolidBrush(RGB(0, 0, 0));
        g_hbrMiniBg = CreateSolidBrush(RGB(15, 20, 30));
        g_hbrMiniListBg = CreateSolidBrush(RGB(22, 30, 46));

        g_hFont = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hBoldFont = CreateFontW(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hMiniListFont = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        auto AddN = [&](HWND h) { g_normalControls.push_back(h); SendMessage(h, WM_SETFONT, (WPARAM)g_hFont, TRUE); return h; };

        // --- NORMAL UI ---
        g_hTopTitle = AddN(CreateWindowW(L"STATIC", L"Pro Asas", WS_VISIBLE | WS_CHILD, 10, 8, 70, 20, hwnd, NULL, NULL, NULL));
        g_hTopBtn = AddN(CreateWindowW(L"BUTTON", L"USTTE: OFF", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 90, 5, 85, 24, hwnd, (HMENU)IDC_BTN_TOP, NULL, NULL));
        AddN(CreateWindowW(L"BUTTON", L"M", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 180, 5, 28, 24, hwnd, (HMENU)IDC_BTN_TOMINI, NULL, NULL));
        AddN(CreateWindowW(L"BUTTON", L"\x2014", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 212, 5, 28, 24, hwnd, (HMENU)IDC_BTN_MIN, NULL, NULL));
        AddN(CreateWindowW(L"BUTTON", L"\x2715", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 244, 5, 28, 24, hwnd, (HMENU)IDC_BTN_CLOSE, NULL, NULL));

        int lX = 15, lEX = 65, stY = 45, stp = 26;
        AddN(CreateWindowW(L"STATIC", L"Skill 1:", WS_VISIBLE | WS_CHILD, lX, stY, 50, 22, hwnd, NULL, NULL, NULL));
        AddN(CreateWindowW(L"STATIC", L"Skill 2:", WS_VISIBLE | WS_CHILD, lX, stY + stp, 50, 22, hwnd, NULL, NULL, NULL));
        AddN(CreateWindowW(L"STATIC", L"Skill 3:", WS_VISIBLE | WS_CHILD, lX, stY + stp * 2, 50, 22, hwnd, NULL, NULL, NULL));
        AddN(CreateWindowW(L"STATIC", L"Skill 4:", WS_VISIBLE | WS_CHILD, lX, stY + stp * 3, 50, 22, hwnd, NULL, NULL, NULL));
        AddN(CreateWindowW(L"STATIC", L"Skill 5:", WS_VISIBLE | WS_CHILD, lX, stY + stp * 4, 50, 22, hwnd, NULL, NULL, NULL));

        g_hEdits[0] = AddN(CreateWindowW(L"EDIT", L"3", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, lEX, stY - 2, 40, 22, hwnd, (HMENU)IDC_EDT_S1, NULL, NULL));
        g_hEdits[1] = AddN(CreateWindowW(L"EDIT", L"4", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, lEX, stY + stp - 2, 40, 22, hwnd, (HMENU)IDC_EDT_S2, NULL, NULL));
        g_hEdits[2] = AddN(CreateWindowW(L"EDIT", L"5", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, lEX, stY + stp * 2 - 2, 40, 22, hwnd, (HMENU)IDC_EDT_S3, NULL, NULL));
        g_hEdits[3] = AddN(CreateWindowW(L"EDIT", L"6", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, lEX, stY + stp * 3 - 2, 40, 22, hwnd, (HMENU)IDC_EDT_S4, NULL, NULL));
        g_hEdits[4] = AddN(CreateWindowW(L"EDIT", L"7", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, lEX, stY + stp * 4 - 2, 40, 22, hwnd, (HMENU)IDC_EDT_S5, NULL, NULL));

        int rX = 125, rEX = 200;
        AddN(CreateWindowW(L"STATIC", L"HP (F):", WS_VISIBLE | WS_CHILD, rX, stY, 70, 22, hwnd, NULL, NULL, NULL));
        AddN(CreateWindowW(L"STATIC", L"MP (MSX1):", WS_VISIBLE | WS_CHILD, rX, stY + stp, 70, 22, hwnd, NULL, NULL, NULL));
        AddN(CreateWindowW(L"STATIC", L"Minor (MSX2):", WS_VISIBLE | WS_CHILD, rX, stY + stp * 2, 70, 22, hwnd, NULL, NULL, NULL));
        AddN(CreateWindowW(L"STATIC", L"Hiz (ms):", WS_VISIBLE | WS_CHILD, rX, stY + stp * 3, 70, 22, hwnd, NULL, NULL, NULL));

        g_hHpEdit = AddN(CreateWindowW(L"EDIT", L"8", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, rEX, stY - 2, 40, 22, hwnd, (HMENU)IDC_EDT_HP, NULL, NULL));
        g_hMpEdit = AddN(CreateWindowW(L"EDIT", L"9", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, rEX, stY + stp - 2, 40, 22, hwnd, (HMENU)IDC_EDT_MP, NULL, NULL));
        g_hMinorEdit = AddN(CreateWindowW(L"EDIT", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, rEX, stY + stp * 2 - 2, 40, 22, hwnd, (HMENU)IDC_EDT_MIN, NULL, NULL));
        g_hMsEdit = AddN(CreateWindowW(L"EDIT", L"350", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, rEX, stY + stp * 3 - 2, 40, 22, hwnd, (HMENU)IDC_EDT_MS, NULL, NULL));

        int advY = 180;
        AddN(CreateWindowW(L"STATIC", L"Skill Bas:", WS_VISIBLE | WS_CHILD, 15, advY, 65, 20, hwnd, NULL, NULL, NULL));
        g_hSkillPressEdit = AddN(CreateWindowW(L"EDIT", L"25", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, 80, advY - 2, 35, 22, hwnd, (HMENU)IDC_EDT_SK_P, NULL, NULL));

        AddN(CreateWindowW(L"STATIC", L"Skill Bek:", WS_VISIBLE | WS_CHILD, 125, advY, 65, 20, hwnd, NULL, NULL, NULL));
        g_hSkillWaitEdit = AddN(CreateWindowW(L"EDIT", L"5", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, 195, advY - 2, 35, 22, hwnd, (HMENU)IDC_EDT_SK_W, NULL, NULL));

        AddN(CreateWindowW(L"STATIC", L"R Bas:", WS_VISIBLE | WS_CHILD, 15, advY + stp, 65, 20, hwnd, NULL, NULL, NULL));
        g_hRPressEdit = AddN(CreateWindowW(L"EDIT", L"20", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, 80, advY + stp - 2, 35, 22, hwnd, (HMENU)IDC_EDT_R_P, NULL, NULL));

        AddN(CreateWindowW(L"STATIC", L"R Bek:", WS_VISIBLE | WS_CHILD, 125, advY + stp, 65, 20, hwnd, NULL, NULL, NULL));
        g_hRWaitEdit = AddN(CreateWindowW(L"EDIT", L"20", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_CENTER, 195, advY + stp - 2, 35, 22, hwnd, (HMENU)IDC_EDT_R_W, NULL, NULL));

        g_hMasterBtn = AddN(CreateWindowW(L"BUTTON", L"SISTEM: PASIF (KAPALI)", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 15, 245, 250, 40, hwnd, (HMENU)IDC_BTN_MAST, NULL, NULL));
        g_hStatusLabel = AddN(CreateWindowW(L"STATIC", L"CAPSLOCK: KAPALI", WS_VISIBLE | WS_CHILD | SS_CENTER, 15, 295, 250, 20, hwnd, NULL, NULL, NULL));

        SendMessage(g_hTopTitle, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
        SendMessage(g_hMasterBtn, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
        SendMessage(g_hStatusLabel, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

        // --- MİNİ UI ---
        auto AddM = [&](HWND h) { g_miniControls.push_back(h); SendMessage(h, WM_SETFONT, (WPARAM)g_hFont, TRUE); return h; };

        g_hMiniTitle = AddM(CreateWindowW(L"STATIC", L"Assassin", WS_CHILD, 12, 11, 80, 20, hwnd, NULL, NULL, NULL));
        AddM(CreateWindowW(L"BUTTON", L"\x25B6 ON", WS_CHILD | BS_OWNERDRAW, 116, 8, 50, 24, hwnd, (HMENU)IDC_BTN_MINI_ON, NULL, NULL));
        AddM(CreateWindowW(L"BUTTON", L"\x25A0 OFF", WS_CHILD | BS_OWNERDRAW, 170, 8, 50, 24, hwnd, (HMENU)IDC_BTN_MINI_OFF, NULL, NULL));
        AddM(CreateWindowW(L"BUTTON", L"\x25BC", WS_CHILD | BS_OWNERDRAW, 224, 8, 24, 24, hwnd, (HMENU)IDC_BTN_MINI_EXP, NULL, NULL));

        g_hMiniList = AddM(CreateWindowW(L"STATIC", L"\n  \x2022 Atak Kombosu\n  \x2022 Oto Minor Spam\n  \x2022 Akilli HP/MP", WS_CHILD, 12, 38, 236, 62, hwnd, NULL, NULL, NULL));
        g_hMiniTimer = AddM(CreateWindowW(L"STATIC", L"Beklemede | Saat: 00:00:00", WS_CHILD | SS_RIGHT, 12, 108, 236, 18, hwnd, NULL, NULL, NULL));

        SendMessage(g_hMiniTitle, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
        SendMessage(g_hMiniList, WM_SETFONT, (WPARAM)g_hMiniListFont, TRUE);

        SetTimer(hwnd, 1, 100, NULL);
        ReadSettings();
        break;
    }
    case WM_TIMER: {
        bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        if (isCapsOn != g_isCapsOn) {
            g_isCapsOn = isCapsOn;
            SetWindowTextW(g_hStatusLabel, isCapsOn ? L"CAPSLOCK: AKTIF" : L"CAPSLOCK: KAPALI");
            if (!g_isMiniMode) InvalidateRect(g_hStatusLabel, NULL, TRUE);
        }
        if (g_isMasterActive && g_isCapsOn) ReadSettings();

        if (g_isMiniMode) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            struct tm parts; localtime_s(&parts, &now_c);
            std::wstringstream ss;
            if (g_isMasterActive) {
                long long elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - g_sessionStartTime).count();
                ss << L"Aktif: " << std::setfill(L'0') << std::setw(2) << (elapsed / 3600) << L":" << std::setw(2) << ((elapsed % 3600) / 60) << L":" << std::setw(2) << (elapsed % 60);
            }
            else { ss << L"Beklemede "; }
            ss << L" | Saat: " << std::setfill(L'0') << std::setw(2) << parts.tm_hour << L":" << std::setw(2) << parts.tm_min << L":" << std::setw(2) << parts.tm_sec << L"  ";
            SetWindowTextW(g_hMiniTimer, ss.str().c_str());
        }
        break;
    }
    case WM_LBUTTONDOWN: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        if (pt.y < 35) {
            ReleaseCapture(); SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        if (!g_isMiniMode) {
            RECT rcTop = { 0, 0, rcClient.right, 34 };
            FillRect(hdc, &rcTop, g_hbrTopBar);
        }
        else {
            RECT rcTimerBg = { 2, 106, rcClient.right - 2, rcClient.bottom - 2 };
            FillRect(hdc, &rcTimerBg, g_hbrBlack);
        }

        HBRUSH hbrBorder = CreateSolidBrush(RGB(70, 80, 100));
        FrameRect(hdc, &rcClient, hbrBorder);
        InflateRect(&rcClient, -1, -1);
        FrameRect(hdc, &rcClient, hbrBorder);
        DeleteObject(hbrBorder);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam; RECT rect; GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, g_isMiniMode ? g_hbrMiniBg : g_hbrMainBg);
        return 1;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        if (pdis->CtlType == ODT_BUTTON) {
            HDC hdc = pdis->hDC;
            RECT rect = pdis->rcItem;
            int id = pdis->CtlID;

            COLORREF bgCol = RGB(31, 41, 55);
            COLORREF textCol = RGB(255, 255, 255);

            if (id == IDC_BTN_MINI_ON) { bgCol = RGB(16, 185, 129); }
            else if (id == IDC_BTN_MINI_OFF) { bgCol = RGB(239, 68, 68); }
            else if (id == IDC_BTN_TOMINI || id == IDC_BTN_MINI_EXP) { bgCol = RGB(245, 158, 11); textCol = RGB(11, 15, 25); }
            else if (id == IDC_BTN_MAST) { bgCol = g_isMasterActive ? RGB(16, 185, 129) : RGB(239, 68, 68); }
            else if (id == IDC_BTN_TOP) { bgCol = g_isTopMost ? RGB(245, 158, 11) : RGB(31, 41, 55); textCol = g_isTopMost ? RGB(11, 15, 25) : RGB(255, 255, 255); }
            else if (id == IDC_BTN_CLOSE) { bgCol = RGB(239, 68, 68); }
            else if (id == IDC_BTN_MIN) { bgCol = RGB(31, 41, 55); }

            HBRUSH hbr = CreateSolidBrush(bgCol); FillRect(hdc, &rect, hbr); DeleteObject(hbr);
            HBRUSH hbrBorder = CreateSolidBrush(RGB(60, 70, 90)); FrameRect(hdc, &rect, hbrBorder); DeleteObject(hbrBorder);

            wchar_t text[64]; GetWindowTextW(pdis->hwndItem, text, 64);
            SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, textCol);
            DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(22, 30, 46));
        return (LRESULT)g_hbrCardBg;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam; HWND hCtrl = (HWND)lParam;
        SetBkMode(hdc, OPAQUE);

        if (g_isMiniMode) {
            if (hCtrl == g_hMiniTitle) { SetTextColor(hdc, RGB(245, 158, 11)); SetBkColor(hdc, RGB(15, 20, 30)); return (LRESULT)g_hbrMiniBg; }
            else if (hCtrl == g_hMiniList) { SetTextColor(hdc, RGB(180, 185, 195)); SetBkColor(hdc, RGB(22, 30, 46)); return (LRESULT)g_hbrMiniListBg; }
            else if (hCtrl == g_hMiniTimer) { SetTextColor(hdc, g_isMasterActive ? RGB(16, 185, 129) : RGB(255, 255, 255)); SetBkColor(hdc, RGB(0, 0, 0)); return (LRESULT)g_hbrBlack; }
            SetBkColor(hdc, RGB(15, 20, 30)); return (LRESULT)g_hbrMiniBg;
        }
        else {
            if (hCtrl == g_hTopTitle) { SetTextColor(hdc, RGB(200, 200, 205)); SetBkColor(hdc, RGB(15, 20, 30)); return (LRESULT)g_hbrTopBar; }
            else if (hCtrl == g_hStatusLabel) { SetBkColor(hdc, RGB(11, 15, 25)); SetTextColor(hdc, (g_isMasterActive && g_isCapsOn) ? RGB(16, 185, 129) : RGB(239, 68, 68)); return (LRESULT)g_hbrMainBg; }
            else { SetBkColor(hdc, RGB(11, 15, 25)); SetTextColor(hdc, RGB(255, 255, 255)); return (LRESULT)g_hbrMainBg; }
        }
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_MAST) { g_isMasterActive = !g_isMasterActive; SetWindowTextW(g_hMasterBtn, g_isMasterActive ? L"SISTEM: AKTIF (ACIK)" : L"SISTEM: PASIF (KAPALI)"); InvalidateRect(hwnd, NULL, FALSE); if (g_isMasterActive) g_sessionStartTime = std::chrono::system_clock::now(); }
        else if (id == IDC_BTN_TOMINI || id == IDC_BTN_MINI_EXP) { ToggleMiniMode(hwnd); }
        else if (id == IDC_BTN_MINI_ON) { g_isMasterActive = true; InvalidateRect(hwnd, NULL, FALSE); g_sessionStartTime = std::chrono::system_clock::now(); }
        else if (id == IDC_BTN_MINI_OFF) { g_isMasterActive = false; InvalidateRect(hwnd, NULL, FALSE); }
        else if (id == IDC_BTN_CLOSE) { DestroyWindow(hwnd); }
        else if (id == IDC_BTN_MIN) { ShowWindow(hwnd, SW_MINIMIZE); }
        else if (id == IDC_BTN_TOP) { g_isTopMost = !g_isTopMost; SetWindowTextW(g_hTopBtn, g_isTopMost ? L"USTTE: ON" : L"USTTE: OFF"); SetWindowPos(hwnd, g_isTopMost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); InvalidateRect(g_hTopBtn, NULL, FALSE); }
        if (HIWORD(wParam) == EN_CHANGE) { ReadSettings(); }
        break;
    }
    case WM_DESTROY: {
        g_exit = true;
        DeleteObject(g_hFont); DeleteObject(g_hBoldFont); DeleteObject(g_hMiniListFont);
        DeleteObject(g_hbrMainBg); DeleteObject(g_hbrCardBg); DeleteObject(g_hbrBlack); DeleteObject(g_hbrTopBar); DeleteObject(g_hbrMiniBg); DeleteObject(g_hbrMiniListBg);
        if (g_comboThread.joinable()) g_comboThread.join(); if (g_hpThread.joinable()) g_hpThread.join();
        if (g_mpThread.joinable()) g_mpThread.join(); if (g_minorThread.joinable()) g_minorThread.join();
        PostQuitMessage(0); break;
    }
    default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    WNDCLASSW wc = { 0 };
    wc.lpszClassName = L"AsasMacroV15";
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WndProc;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    RegisterClassW(&wc);

    CreateWindowW(wc.lpszClassName, L"Pro Asas Macro", WS_POPUP | WS_VISIBLE, 150, 150, 280, 340, 0, 0, hInstance, 0);

    g_comboThread = std::thread(ComboThread); g_hpThread = std::thread(HpThread);
    g_mpThread = std::thread(MpThread); g_minorThread = std::thread(MinorThread);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}