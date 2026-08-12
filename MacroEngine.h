#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include "ScreenScanner.h"

class MacroEngine {
private:
    ScreenScanner m_scanner; // OpenCV Göz objesi

    // Yüzdelik okuma için tüm barın alanını ve limitini tutan değişkenler
    RECT g_hpRect = { 0 };
    RECT g_mpRect = { 0 };
    int g_hpLimit = 50;
    int g_mpLimit = 50;

    // YENİ: Seçilen alanın orijinal renklerini hafızada tutacağız
    COLORREF g_originalHpColor = RGB(0, 0, 0);
    COLORREF g_originalMpColor = RGB(0, 0, 0);

    bool g_exit;
    bool g_isMasterActive;
    bool g_isCapsOn;

    std::thread g_comboThread;
    std::thread g_hpThread;
    std::thread g_mpThread;
    std::thread g_minorThread;
    std::mutex g_inputMutex;

    // Ayarlar
    std::vector<WORD> g_skills;
    int g_delayMs;
    WORD g_hpKey, g_mpKey, g_minorKey;
    int g_skillPressMs, g_skillWaitMs, g_rPressMs, g_rWaitMs;

    int GetRandomDelay(int minDelay, int maxDelay);
    void SendKey(WORD keyCode, int pressDelay);
    bool IsUserChangingPage();
    bool SmartSleep(int totalMs);

    void ComboLoop();
    void HpLoop();
    void MpLoop();
    void MinorLoop();

public:
    MacroEngine();
    ~MacroEngine();

    void Start();
    void Stop();
    void SetMasterActive(bool active);
    bool IsMasterActive() const;

    // Ayarları dışarıdan güncellemek için
    void UpdateSettings(const std::vector<WORD>& skills, int delay, WORD hp, WORD mp, WORD minor, int sPress, int sWait, int rPress, int rWait);

    // OpenCV Koordinat ve Yüzdelik limit atama fonksiyonları
    void SetHpRect(RECT r);
    void SetMpRect(RECT r);
    void SetLimits(int hpLimit, int mpLimit);

    void SetHpRect(RECT r, COLORREF c);
    void SetMpRect(RECT r, COLORREF c);
};