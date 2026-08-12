#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <opencv2/opencv.hpp> // OpenCV Kütüphanesi

class ScreenScanner {
private:
    HWND m_hDesktop;
    HDC m_hWindowDC;
    HDC m_hMemoryDC;
    HBITMAP m_hBitmap;

public:
    ScreenScanner();
    ~ScreenScanner();

    // Ekranın belirli bir bölgesinin resmini çekip OpenCV formatına (cv::Mat) çevirir
    cv::Mat CaptureScreen(int x, int y, int width, int height);

    // HP (Kırmızı) veya MP (Mavi) barlarının azalıp azalmadığını kontrol eder
    // colorType: 0 = Kırmızı(HP), 1 = Mavi(MP)
    bool IsBarLow(int x, int y, int colorType, int threshold = 50);
};