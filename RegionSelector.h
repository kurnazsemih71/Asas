#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class RegionSelector {
public:
    // Bu fonksiyon çağrıldığında ekran donar/saydamlaşır, kullanıcı seçim yapar ve RECT (koordinatlar) döner.
    static RECT SelectRegion();
};