#include "RegionSelector.h"
#include <cstdio>

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
                InvalidateRect(hwnd, NULL, FALSE); // Titremeyi ve iz bırakmayı önlemek için FALSE yaptık
            }
            return 0;

        case WM_LBUTTONUP:
            if (g_isDragging) {
                g_isDragging = false;
                ReleaseCapture();

                g_selectedRect.left = min(g_ptStart.x, g_ptCurrent.x);
                g_selectedRect.right = max(g_ptStart.x, g_ptCurrent.x);
                g_selectedRect.top = min(g_ptStart.y, g_ptCurrent.y);
                g_selectedRect.bottom = max(g_ptStart.y, g_ptCurrent.y);

                PostQuitMessage(0);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            if (g_isDragging) {
                RECT drawRect;
                drawRect.left = min(g_ptStart.x, g_ptCurrent.x);
                drawRect.right = max(g_ptStart.x, g_ptCurrent.x);
                drawRect.top = min(g_ptStart.y, g_ptCurrent.y);
                drawRect.bottom = max(g_ptStart.y, g_ptCurrent.y);

                // Kırmızı kalın çerçeve
                HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));
                FrameRect(hdc, &drawRect, hBrush);
                DeleteObject(hBrush);

                // Ölçü hesaplama
                int w = drawRect.right - drawRect.left;
                int h = drawRect.bottom - drawRect.top;

                wchar_t infoText[64];
                swprintf_s(infoText, 64, L" Genislik: %d px | Yukseklik: %d px ", w, h);

                // Kalın ve net font
                HFONT hBoldFont = CreateFontW(18, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
                HFONT hOldFont = (HFONT)SelectObject(hdc, hBoldFont);

                RECT textRect = { g_ptCurrent.x + 20, g_ptCurrent.y + 20, g_ptCurrent.x + 300, g_ptCurrent.y + 50 };

                // Metin kutusu arka planı (Koyu şık tema)
                HBRUSH bgBox = CreateSolidBrush(RGB(15, 20, 30));
                FillRect(hdc, &textRect, bgBox);
                DeleteObject(bgBox);

                // Kutunun etrafına ince beyaz çerçeve
                HBRUSH boxBorder = CreateSolidBrush(RGB(255, 255, 255));
                FrameRect(hdc, &textRect, boxBorder);
                DeleteObject(boxBorder);

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(0, 255, 128)); // Canlı Neon Yeşil Yazı

                DrawTextW(hdc, infoText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hdc, hOldFont);
                DeleteObject(hBoldFont);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1; // Titremeyi (Flicker) engeller

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                g_selectedRect = { 0, 0, 0, 0 };
                PostQuitMessage(0);
            }
            return 0;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    }
}

RECT RegionSelector::SelectRegion() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = SelectorWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
    wc.lpszClassName = L"RegionSelectorClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassW(&wc);

    int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // WS_EX_COMPOSITED (Çift Tamponlama) ile pikseller kusursuz çizilir, donma ve tıklama sorunu yaşanmaz
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_COMPOSITED,
        wc.lpszClassName, L"Selector",
        WS_POPUP | WS_VISIBLE,
        screenX, screenY, screenW, screenH,
        NULL, NULL, hInstance, NULL
    );

    // Hafif saydamlık veriyoruz ki arkadaki oyun net görünsün ama tıklamalar engellenmesin
    SetLayeredWindowAttributes(hwnd, 0, 120, LWA_ALPHA);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hInstance);

    return g_selectedRect;
}