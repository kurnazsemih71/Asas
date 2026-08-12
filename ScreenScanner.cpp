#include "ScreenScanner.h"

ScreenScanner::ScreenScanner() {
    // Windows API ile masaüstü çizim (GDI) araçlarını hazırlıyoruz
    m_hDesktop = GetDesktopWindow();
    m_hWindowDC = GetWindowDC(m_hDesktop);
    m_hMemoryDC = CreateCompatibleDC(m_hWindowDC);
    m_hBitmap = NULL;
}

ScreenScanner::~ScreenScanner() {
    // Hafıza sızıntısı (Memory Leak) olmaması için çöpleri temizliyoruz
    if (m_hBitmap) DeleteObject(m_hBitmap);
    DeleteDC(m_hMemoryDC);
    ReleaseDC(m_hDesktop, m_hWindowDC);
}

cv::Mat ScreenScanner::CaptureScreen(int x, int y, int width, int height) {
    if (m_hBitmap) DeleteObject(m_hBitmap);
    m_hBitmap = CreateCompatibleBitmap(m_hWindowDC, width, height);
    SelectObject(m_hMemoryDC, m_hBitmap);

    // Ekranın ilgili koordinatını çok yüksek hızla kopyala (BitBlt)
    BitBlt(m_hMemoryDC, 0, 0, width, height, m_hWindowDC, x, y, SRCCOPY);

    // Kopyalanan görüntüyü OpenCV'ye aktarmak için formatlıyoruz
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // Windows resimleri ters tutar, eksi koyarak düzeltiyoruz
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // OpenCV resim nesnesini (Mat) oluştur
    cv::Mat mat(height, width, CV_8UC4); // 8-bit, 4 Kanal (B,G,R,Alpha)
    GetDIBits(m_hWindowDC, m_hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    return mat;
}

bool ScreenScanner::IsBarLow(int x, int y, int colorType, int threshold) {
    // Sadece 1x1 piksellik (nokta) bir alanın resmini çekiyoruz (Çok hızlı!)
    cv::Mat pixelMat = CaptureScreen(x, y, 1, 1);

    if (pixelMat.empty()) return false;

    // OpenCV pikselleri B-G-R (Mavi-Yeşil-Kırmızı) sırasında tutar
    cv::Vec4b pixel = pixelMat.at<cv::Vec4b>(0, 0);
    int blue = pixel[0];
    int green = pixel[1];
    int red = pixel[2];

    if (colorType == 0) {
        // HP Kontrolü (Kırmızı renk yoğunluğuna bakıyoruz)
        return (red < threshold); // Eğer kırmızılık belirlediğimiz sınırın altındaysa "Can Azaldı" (True) döner
    }
    else {
        // MP Kontrolü (Mavi renk yoğunluğuna bakıyoruz)
        return (blue < threshold); // Eğer mavilik sınırın altındaysa "Mana Azaldı" (True) döner
    }
}