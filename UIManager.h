#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <chrono> // ZAMAN HESAPLAMA KÜTÜPHANESİ EKLENDİ
#include "MacroEngine.h"

class UIManager {
private:
    HWND m_hwnd;
    MacroEngine& m_engine;

    bool g_isMiniMode;
    bool g_isTopMost;
    bool g_isMasterActive;
    bool g_isCapsOn;

    HFONT g_hFont, g_hBoldFont, g_hMiniListFont;
    HBRUSH g_hbrMainBg, g_hbrCardBg, g_hbrBlack, g_hbrTopBar, g_hbrMiniBg, g_hbrMiniListBg;

    std::vector<HWND> g_normalControls;
    std::vector<HWND> g_miniControls;

    HWND g_hTopTitle, g_hTopBtn, g_hMasterBtn, g_hStatusLabel;
    HWND g_hEdits[5], g_hMsEdit, g_hHpEdit, g_hMpEdit, g_hMinorEdit;
    HWND g_hSkillPressEdit, g_hSkillWaitEdit, g_hRPressEdit, g_hRWaitEdit;
    HWND g_hMiniTitle, g_hMiniList, g_hMiniTimer;
    std::chrono::system_clock::time_point g_sessionStartTime;

    void ToggleMiniMode();
    void ReadAndApplySettings();

public:
    UIManager(MacroEngine& engine);
    ~UIManager();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};