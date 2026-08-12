#include "MacroEngine.h"

struct AsasSkillSlot {
    WORD pageKey;
    WORD skillKey;
    int cooldownMs;
    int priority;
    std::chrono::steady_clock::time_point lastCastTime;
};

// Asas skill veritabanı (Senin F1 ve F2 tuş dizilimin sırasına göre)
static std::vector<AsasSkillSlot> g_asasSkillDatabase = {
    // [F1] ANA HASAR (3-4-5-6-7)
    { VK_F1, '3', 12000, 10, std::chrono::steady_clock::now() - std::chrono::seconds(20) }, // Spike
    { VK_F1, '4', 11000, 9,  std::chrono::steady_clock::now() - std::chrono::seconds(20) }, // Thrust
    { VK_F1, '5', 5000,  8,  std::chrono::steady_clock::now() - std::chrono::seconds(20) }, // Bloody Beast
    { VK_F1, '6', 5000,  7,  std::chrono::steady_clock::now() - std::chrono::seconds(20) }, // Cut
    { VK_F1, '7', 10000, 6,  std::chrono::steady_clock::now() - std::chrono::seconds(20) }, // Pierce

    // [F2] SERİ HASAR (3-4-5-6-7)
    { VK_F2, '3', 5000,  5,  std::chrono::steady_clock::now() - std::chrono::seconds(20) }, // Shock
    { VK_F2, '4', 5000,  4,  std::chrono::steady_clock::now() - std::chrono::seconds(20) }, // Jab
    { VK_F2, '5', 5000,  3,  std::chrono::steady_clock::now() - std::chrono::seconds(20) }, // Stab2
    { VK_F2, '6', 5000,  2,  std::chrono::steady_clock::now() - std::chrono::seconds(20) }, // Stab
    { VK_F2, '7', 5000,  1,  std::chrono::steady_clock::now() - std::chrono::seconds(20) }  // Cut
};

static WORD g_currentPage = VK_F1;

MacroEngine::MacroEngine() {
    g_exit = false;
    g_isMasterActive = false;
    g_isCapsOn = false;
    m_classMode = CLASS_WARRIOR_BP;
    g_delayMs = 350;
    g_hpKey = 0; g_mpKey = 0; g_minorKey = 0;
    g_skillPressMs = 25; g_skillWaitMs = 5; g_rPressMs = 20; g_rWaitMs = 20;
}

MacroEngine::~MacroEngine() {
    Stop();
}

void MacroEngine::SetClassMode(MacroClassMode mode) {
    m_classMode = mode;
}

void MacroEngine::Start() {
    g_exit = false;
    g_currentPage = VK_F1;
    g_comboThread = std::thread(&MacroEngine::ComboLoop, this);
    g_hpThread = std::thread(&MacroEngine::HpLoop, this);
    g_mpThread = std::thread(&MacroEngine::MpLoop, this);
    g_minorThread = std::thread(&MacroEngine::MinorLoop, this);
}

void MacroEngine::Stop() {
    g_exit = true;
    if (g_comboThread.joinable()) g_comboThread.join();
    if (g_hpThread.joinable()) g_hpThread.join();
    if (g_mpThread.joinable()) g_mpThread.join();
    if (g_minorThread.joinable()) g_minorThread.join();
}

// 🌟 KRİTİK ÇÖZÜM BURADA: Sistemin Hafızasını Sıfırlama
void MacroEngine::SetMasterActive(bool active) {
    g_isMasterActive = active;

    // Sen arayüzden sistemi Pasif/Aktif yaptığında bütün cooldown süreleri sıfırlanır.
    // Ancak CapsLock ile dur-kalk yaptığında burası tetiklenmez, süre hafızası korunur!
    for (auto& skill : g_asasSkillDatabase) {
        skill.lastCastTime = std::chrono::steady_clock::now() - std::chrono::seconds(20);
    }
}

bool MacroEngine::IsMasterActive() const {
    return g_isMasterActive;
}

void MacroEngine::UpdateSettings(const std::vector<WORD>& skills, int delay, WORD hp, WORD mp, WORD minor, int sPress, int sWait, int rPress, int rWait) {
    g_skills = skills;
    g_delayMs = delay;
    g_hpKey = hp;
    g_mpKey = mp;
    g_minorKey = minor;
    g_skillPressMs = sPress;
    g_skillWaitMs = sWait;
    g_rPressMs = rPress;
    g_rWaitMs = rWait;
}

int MacroEngine::GetRandomDelay(int minDelay, int maxDelay) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(minDelay, maxDelay);
    return distr(gen);
}

void MacroEngine::SendKey(WORD keyCode, int pressDelay) {
    if (keyCode == 0) return;
    std::lock_guard<std::mutex> lock(g_inputMutex);
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;

    bool isFunctionKey = (keyCode >= VK_F1 && keyCode <= VK_F8);

    if (isFunctionKey) {
        input.ki.wVk = keyCode;
        input.ki.dwFlags = 0;
    }
    else {
        input.ki.wScan = MapVirtualKeyW(keyCode, MAPVK_VK_TO_VSC);
        input.ki.wVk = 0;
        input.ki.dwFlags = KEYEVENTF_SCANCODE;
    }
    SendInput(1, &input, sizeof(INPUT));

    std::this_thread::sleep_for(std::chrono::milliseconds(pressDelay));

    if (isFunctionKey) {
        input.ki.dwFlags = KEYEVENTF_KEYUP;
    }
    else {
        input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    }
    SendInput(1, &input, sizeof(INPUT));
}

bool MacroEngine::IsUserChangingPage() {
    for (int i = VK_F1; i <= VK_F8; ++i) {
        if (GetAsyncKeyState(i) & 0x8000) return true;
    }
    return false;
}

bool MacroEngine::SmartSleep(int totalMs) {
    int waited = 0;
    while (waited < totalMs) {
        bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        if (!g_isMasterActive || !isCapsOn || g_exit) return true;

        for (int i = VK_F1; i <= VK_F8; ++i) {
            if (GetAsyncKeyState(i) & 0x8000) {
                g_currentPage = i;
                if (m_classMode == CLASS_WARRIOR_BP) {
                    return true;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        waited += 10;
    }
    return false;
}

void MacroEngine::ComboLoop() {
    while (!g_exit) {
        bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

        if (g_isMasterActive && isCapsOn) {

            // ==========================================
            // MOD 1: KLASİK WARRIOR / BP SİSTEMİ
            // ==========================================
            if (m_classMode == CLASS_WARRIOR_BP) {
                if (IsUserChangingPage()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(150, 200)));
                    continue;
                }

                bool interrupted = false;
                for (WORD skill : g_skills) {
                    isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                    if (!g_isMasterActive || !isCapsOn || g_exit) break;
                    if (IsUserChangingPage()) { interrupted = true; break; }

                    SendKey(skill, GetRandomDelay(g_skillPressMs, g_skillPressMs + 12));
                    std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(g_skillWaitMs, g_skillWaitMs + 15)));
                }

                isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                if (interrupted || !g_isMasterActive || !isCapsOn || g_exit) {
                    if (interrupted) std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(150, 220)));
                    continue;
                }

                SendKey('R', GetRandomDelay(g_rPressMs, g_rPressMs + 10));
                std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(g_rWaitMs, g_rWaitMs + 20)));
                SendKey('R', GetRandomDelay(g_rPressMs, g_rPressMs + 10));

                SmartSleep(GetRandomDelay(g_delayMs, g_delayMs + 35));
            }
            // ==========================================
            // MOD 2: AKILLI YARI-OTOMATİK ASAS SİSTEMİ
            // ==========================================
            else if (m_classMode == CLASS_ASAS) {

                auto now = std::chrono::steady_clock::now();
                bool skillFired = false;

                // Priority sırasına göre tarama (Artık düz 3-4-5-6-7)
                for (auto& skill : g_asasSkillDatabase) {
                    isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                    if (!g_isMasterActive || !isCapsOn || g_exit) break;

                    if (skill.pageKey != g_currentPage) {
                        continue;
                    }

                    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - skill.lastCastTime).count();

                    if (elapsedMs >= skill.cooldownMs) {
                        SendKey(skill.skillKey, GetRandomDelay(g_skillPressMs, g_skillPressMs + 5));

                        skill.lastCastTime = std::chrono::steady_clock::now();
                        skillFired = true;

                        SmartSleep(GetRandomDelay(g_skillWaitMs, g_skillWaitMs + 5));
                        break;
                    }
                }

                SendKey('R', GetRandomDelay(g_rPressMs, g_rPressMs + 5));
                SmartSleep(GetRandomDelay(g_rWaitMs, g_rWaitMs + 10));
                SendKey('R', GetRandomDelay(g_rPressMs, g_rPressMs + 5));

                SmartSleep(GetRandomDelay(g_delayMs, g_delayMs + 35));
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void MacroEngine::HpLoop() {
    while (!g_exit) {
        bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        if (g_isMasterActive && isCapsOn && (GetAsyncKeyState('F') & 0x8000)) {
            SendKey(g_hpKey, GetRandomDelay(12, 25));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(140, 180)));
        }
        else { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
    }
}

void MacroEngine::MpLoop() {
    while (!g_exit) {
        bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        if (g_isMasterActive && isCapsOn && (GetAsyncKeyState(VK_XBUTTON1) & 0x8000)) {
            SendKey(g_mpKey, GetRandomDelay(12, 25));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(140, 180)));
        }
        else { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
    }
}

void MacroEngine::MinorLoop() {
    while (!g_exit) {
        bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        if (g_isMasterActive && isCapsOn && (GetAsyncKeyState(VK_XBUTTON2) & 0x8000)) {
            SendKey(g_minorKey, GetRandomDelay(4, 10));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(4, 10)));
            SendKey(g_minorKey, GetRandomDelay(4, 10));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(12, 25)));
        }
        else { std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    }
}