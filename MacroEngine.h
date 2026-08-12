#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>

enum MacroClassMode {
    CLASS_WARRIOR_BP = 0,
    CLASS_ASAS = 1
};

class MacroEngine {
private:
    bool g_exit;
    bool g_isMasterActive;
    bool g_isCapsOn;

    std::thread g_comboThread;
    std::thread g_hpThread;
    std::thread g_mpThread;
    std::thread g_minorThread;
    std::mutex g_inputMutex;

    std::vector<WORD> g_skills;
    int g_delayMs;
    WORD g_hpKey, g_mpKey, g_minorKey;
    int g_skillPressMs, g_skillWaitMs, g_rPressMs, g_rWaitMs;

    MacroClassMode m_classMode;

    int GetRandomDelay(int minDelay, int maxDelay);
    void SendKey(WORD keyCode, int pressDelay);
    bool IsUserChangingPage();
    bool SmartSleep(int totalMs);

    // İki motoru tek bir akıllı motorda birleştirdik
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

    void SetClassMode(MacroClassMode mode);
    void UpdateSettings(const std::vector<WORD>& skills, int delay, WORD hp, WORD mp, WORD minor, int sPress, int sWait, int rPress, int rWait);
};