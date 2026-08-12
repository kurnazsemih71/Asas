#include "RegionSelector.h"

// Arka planda kullanılacak geçici global değişkenler (Sadece bu dosyaya özel)
namespace {
    bool g_isDragging = false;
    POINT g_ptStart = { 0 };
    POINT g_ptCurrent = { 0 };
    RECT g_selectedRect = { 0 };

    LRESULT CALLBACK SelectorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_LBUTTONDOWN:
            g_isDragging = true;
            g_ptStart.x = LOWORD(lParam);
            g_ptStart.y = HIWORD(lParam);
            g_ptCurrent = g_ptStart;
            SetCapture(hwnd);
            return 0;

        case WM_MOUSEMOVE:
            if (g_isDragging) {
                g_ptCurrent.x = LOWORD(lParam);
                g_ptCurrent.y = HIWORD(lParam);
                InvalidateRect(hwnd, NULL, TRUE); // Ekranı yenile (Kırmızı çizgiyi çizmek için)
            }
            return 0;

        case WM_LBUTTONUP:
            if (g_isDragging) {
                g_isDragging = false;
                ReleaseCapture();

                // Seçilen dikdörtgenin sınırlarını düzgün bir şekilde belirle (Ters çizimlere karşı)
                g_selectedRect.left = min(g_ptStart.x, g_ptCurrent.x);
                g_selectedRect.right = max(g_ptStart.x, g_ptCurrent.x);
                g_selectedRect.top = min(g_ptStart.y, g_ptCurrent.y);
                g_selectedRect.bottom = max(g_ptStart.y, g_ptCurrent.y);

                // İşlem bitti, bu geçici pencereyi kapat
                PostQuitMessage(0);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // Eğer fareyle sürükleme yapılıyorsa kırmızı seçim çerçevesini çiz
            if (g_isDragging) {
                RECT drawRect;
                drawRect.left = min(g_ptStart.x, g_ptCurrent.x);
                drawRect.right = max(g_ptStart.x, g_ptCurrent.x);
                drawRect.top = min(g_ptStart.y, g_ptCurrent.y);
                drawRect.bottom = max(g_ptStart.y, g_ptCurrent.y);

                // Kırmızı ve kalın bir fırça oluştur
                HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));
                FrameRect(hdc, &drawRect, hBrush);
                DeleteObject(hBrush);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                // ESC'ye basılırsa seçimi iptal et ve çık
                g_selectedRect = { 0, 0, 0, 0 };
                PostQuitMessage(0);
            }
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
}

RECT RegionSelector::SelectRegion() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = SelectorWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS); // İmleci "Artı" (Nişangah) işaretine çevirir
    wc.lpszClassName = L"RegionSelectorClass";
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0)); // Siyah arka plan

    RegisterClassW(&wc);

    // Tüm ekranları kaplayacak boyutları al (Çift monitör varsa orayı da kapsar)
    int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // En üstte duran, yarı saydam bir pencere oluşturuyoruz
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"Selector",
        WS_POPUP | WS_VISIBLE,
        screenX, screenY, screenW, screenH,
        NULL, NULL, hInstance, NULL
    );

    // Siyah pencereyi %40 saydam (Alpha: 100) yapıyoruz ki arkadaki oyunu görelim
    SetLayeredWindowAttributes(hwnd, 0, 100, LWA_ALPHA);

    // Seçim yapılana kadar burada programı beklet (Modal Loop)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hInstance);

    return g_selectedRect;
}