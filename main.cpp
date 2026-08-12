#include <windows.h>
#include "MacroEngine.h"
#include "UIManager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 1. Arka plan motorunu başlat
    MacroEngine engine;
    engine.Start();

    // 2. Arayüz yöneticisini oluştur ve çalıştır
    UIManager ui(engine);
    if (!ui.Initialize(hInstance, nCmdShow)) {
        return 1;
    }

    // 3. Ana Windows mesaj döngüsü
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}