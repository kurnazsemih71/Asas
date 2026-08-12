#include "MacroEngine.h"

MacroEngine::MacroEngine() {
    g_exit = false;
    g_isMasterActive = false;
    g_isCapsOn = false;
    g_delayMs = 350;
    g_hpKey = 0; g_mpKey = 0; g_minorKey = 0;
    g_skillPressMs = 25; g_skillWaitMs = 5; g_rPressMs = 20; g_rWaitMs = 20;

    // Yüzdelik bar değerlerinin başlangıç ayarları (Geçmişten kalanlar)
    g_hpLimit = 50;
    g_mpLimit = 50;
    g_hpRect = { 0, 0, 0, 0 };
    g_mpRect = { 0, 0, 0, 0 };
}

MacroEngine::~MacroEngine() {
    Stop();
}

void MacroEngine::Start() {
    g_exit = false;
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

void MacroEngine::SetMasterActive(bool active) {
    g_isMasterActive = active;
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

void MacroEngine::SetHpRect(RECT r, COLORREF c) {
    g_hpRect = r;
    g_originalHpColor = c;
}

void MacroEngine::SetMpRect(RECT r, COLORREF c) {
    g_mpRect = r;
    g_originalMpColor = c;
}

void MacroEngine::SetLimits(int hpLimit, int mpLimit) {
    g_hpLimit = hpLimit;
    g_mpLimit = mpLimit;
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
    input.ki.wScan = MapVirtualKeyW(keyCode, MAPVK_VK_TO_VSC);
    input.ki.wVk = 0;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &input, sizeof(INPUT));

    std::this_thread::sleep_for(std::chrono::milliseconds(pressDelay));

    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
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
        if (IsUserChangingPage()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        waited += 10;
    }
    return false;
}

void MacroEngine::ComboLoop() {
    while (!g_exit) {
        bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        if (g_isMasterActive && isCapsOn) {
            if (IsUserChangingPage()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(150, 200)));
                continue;
            }

            bool interrupted = false;
            for (WORD skill : g_skills) {
                isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                if (!g_isMasterActive || !isCapsOn || g_exit) break;

                if (IsUserChangingPage()) {
                    interrupted = true;
                    break;
                }

                SendKey(skill, GetRandomDelay(g_skillPressMs, g_skillPressMs + 12));
                std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(g_skillWaitMs, g_skillWaitMs + 15)));
            }

            isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            if (interrupted || !g_isMasterActive || !isCapsOn || g_exit) {
                if (interrupted) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(150, 220)));
                }
                continue;
            }

            SendKey('R', GetRandomDelay(g_rPressMs, g_rPressMs + 10));
            std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(g_rWaitMs, g_rWaitMs + 20)));
            SendKey('R', GetRandomDelay(g_rPressMs, g_rPressMs + 10));

            SmartSleep(GetRandomDelay(g_delayMs, g_delayMs + 35));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

// POT MOTORLARI İPTAL EDİLDİ: Artık 3, 4, 5, 6, 7 tuşlarını sırayla tarayıp boştate olanları vuracak (Skill Rotation)
void MacroEngine::HpLoop() {
    // 3, 4, 5, 6, 7 tuş kodları (Virtual Key karakter kodları)
    int skillKeys[] = { '3', '4', '5', '6', '7' };
    int totalSkills = 5;

    // Her skill için cooldown (bekleme süresi) takibi
    auto lastCastTimes = std::vector<std::chrono::steady_clock::time_point>(totalSkills, std::chrono::steady_clock::now() - std::chrono::seconds(5));

    while (!g_exit) {
        bool isCapsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

        if (g_isMasterActive && isCapsOn) {
            // Sağ tık (VK_RBUTTON) basılı tutulduğunda veya aktifken skill döngüsü akmaya başlar
            bool isTriggered = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

            if (isTriggered) {
                auto now = std::chrono::steady_clock::now();

                for (int i = 0; i < totalSkills; i++) {
                    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCastTimes[i]).count();

                    // Eğer skillin bekleme süresi (örn: 1000ms / 1 saniye) bittiyse vur
                    if (elapsedMs > 1000) {
                        SendKey(skillKeys[i], GetRandomDelay(15, 25));
                        lastCastTimes[i] = std::chrono::steady_clock::now(); // Cooldown'ı başlat

                        std::this_thread::sleep_for(std::chrono::milliseconds(GetRandomDelay(60, 100)));
                        break; // Sıradaki skill için döngüyü akıt
                    }
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

// MP Loop da boşta kalmasın, burayı da yedek skill tarayıcı veya serbest bırakabiliriz (Şimdilik boşta bekler)
void MacroEngine::MpLoop() {
    while (!g_exit) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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