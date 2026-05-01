#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "globals.h"

// Pengganti String biar enteng dan cepat!
#define BTN_NONE  0
#define BTN_UP    1
#define BTN_DOWN  2
#define BTN_LEFT  3
#define BTN_RIGHT 4
#define BTN_OK    5

// Tarik fungsi dari display buat set brightness
extern void setOledBrightness(uint8_t level);

// Pengganti millis()
uint32_t input_millis() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// Deklarasi fungsi di bawah
void handleNavigasiScanner(int btn);
void handleNavigasiDeauth(int btn);
void handleNavigasiSpam(int btn);

void handleJoystick() {
    static uint32_t lastPress = 0;
    if (input_millis() - lastPress < 250) return; 

    // --- 1. TENTUIN DULU BTN NYA ---
    int btn = BTN_NONE;
    if (gpio_get_level(PIN_UP) == 0)         btn = BTN_UP;
    else if (gpio_get_level(PIN_DOWN) == 0)  btn = BTN_DOWN;
    else if (gpio_get_level(PIN_LEFT) == 0)  btn = BTN_LEFT;
    else if (gpio_get_level(PIN_RIGHT) == 0) btn = BTN_RIGHT;
    else if (gpio_get_level(PIN_OK) == 0)    btn = BTN_OK;

    if (btn == BTN_NONE) return; 

    // --- 2. PANGGIL APPMODE ---
    if (appMode == 1) { handleNavigasiScanner(btn); lastPress = input_millis(); return; }
    if (appMode == 2) { handleNavigasiDeauth(btn);  lastPress = input_millis(); return; }
    if (appMode == 4) { handleNavigasiSpam(btn);    lastPress = input_millis(); return; } 
     if (appMode == 5) { handleNavigasiScanSta(btn); lastPress = input_millis(); return; }
    
    // Mode 3 (Brightness)
    if (appMode == 3) {
        if (btn == BTN_UP) {
            if (brightnessValue < 245) brightnessValue += 10;
            setOledBrightness(brightnessValue);
        }
        else if (btn == BTN_DOWN) {
            if (brightnessValue > 10) brightnessValue -= 10;
            setOledBrightness(brightnessValue);
        }
        else if (btn == BTN_LEFT) appMode = 0;
        
        lastPress = input_millis();
        return;
    }

    // --- BATAS TAMBAHAN MENU UTAMA ---
    if (!inSubMenu) {
        if (btn == BTN_RIGHT) {
            currentMenu = (currentMenu + 1) % 4; 
            lastPress = input_millis();
        }
        else if (btn == BTN_LEFT) {
            currentMenu = (currentMenu - 1 + 4) % 4; 
            lastPress = input_millis();
        }
        else if (btn == BTN_OK) {
            inSubMenu = true; 
            currentSub = 0;   
            topMenu = 0;
            lastPress = input_millis();
        }
    } 
    else {
        if (btn == BTN_DOWN) {
            int limitMenu = 0; 
            if(currentMenu == 0)      limitMenu = 4; 
            else if(currentMenu == 1) limitMenu = 3;
            else if(currentMenu == 2) limitMenu = 5;
            else                      limitMenu = 4;

            if (currentSub < (limitMenu - 1)) { 
                currentSub++;
                if (currentSub >= topMenu + 5) topMenu++;
            }
            lastPress = input_millis();
        }
        else if (btn == BTN_UP) {
            if (currentSub > 0) {
                currentSub--;
                if (currentSub < topMenu) topMenu--;
            }
            lastPress = input_millis();
        }
        else if (btn == BTN_LEFT) {
            inSubMenu = false; 
            lastPress = input_millis();
        }
        else if (btn == BTN_OK) {
            if (currentMenu == 0 && currentSub == 0) {
                appMode = 1;      
                scannerState = 0; 
            } else if (currentMenu == 0 && currentSub == 2) {
                appMode = 1;
                scannerState = 2;     
                cursorInScanner = 0;  
                scrollPosScanner = 0; 
            } else if (currentMenu == 3 && currentSub == 0) { 
                appMode = 3; 
            } else if (currentMenu == 0 && currentSub == 1) {
                aktifModeSpam = 1; 
                appMode = 4;       
                spamState = 0;
            } else if (currentMenu == 0 && currentSub == 3) {
                aktifModeSpam = 2; 
                appMode = 4;
                spamState = 0;
            }
            lastPress = input_millis();
        }
    }
}

// ==========================================
// LOGIKA NAVIGASI KHUSUS 
// ==========================================
void handleNavigasiScanner(int btn) {
    if (scannerState == 0) {
        if (btn == BTN_LEFT) appMode = 0; 
        else if (btn == BTN_RIGHT || btn == BTN_OK) { 
            scannerState = 1;     
            triggerScan = true;   
            scanDone = false;     
            cursorInScanner = 0;  
            scrollPosScanner = 0; 
        }
    }
    else if (scannerState == 1) {
        if (btn == BTN_LEFT) scannerState = 0; 
    }
    else if (scannerState == 2) {
        if (btn == BTN_UP) {
            if (cursorInScanner > 0) cursorInScanner--;
            else if (scrollPosScanner > 0) scrollPosScanner--;
        } 
        else if (btn == BTN_DOWN) {
            if (cursorInScanner < 2 && (scrollPosScanner + cursorInScanner) < (totalWiFi - 1)) cursorInScanner++;
            else if ((scrollPosScanner + 3) < totalWiFi) scrollPosScanner++;
        }
        else if (btn == BTN_OK) {
            if (totalWiFi > 0) {
                targetLockedIdx = scrollPosScanner + cursorInScanner; 
                targetTerkunci = listWiFi[targetLockedIdx];
                adaTarget = true; 
                scannerState = 4;   
                contextCursor = 0;  
            }
        }
        else if (btn == BTN_LEFT) {
            scannerState = 0; 
            appMode = 0;      
        }
    }
    else if (scannerState == 3) {
        if (btn == BTN_LEFT) scannerState = 4; 
    }
      else if (scannerState == 4) {
        // Navigasi Naik/Turun
        if (btn == BTN_UP) {
            contextCursor = (contextCursor > 0) ? contextCursor - 1 : 2; // Batas atas ke 2
        } 
        else if (btn == BTN_DOWN) {
            contextCursor = (contextCursor < 2) ? contextCursor + 1 : 0; // Batas bawah ke 2
        }
        
        else if (btn == BTN_OK) {
        if (contextCursor == 0) { appMode = 2; deauthState = 0; } 
        else if (contextCursor == 1) { appMode = 5; scannerStateSta = 0; contextCursor = 0; }
        else if (contextCursor == 2) { // TRACK
            appMode = 6; 
            triggerTrack = true; // Nyalain update RSSI
        }
        else if (contextCursor == 3) { scannerState = 3; }
    }
        else if (btn == BTN_LEFT) scannerState = 2; 
    }

}

void handleNavigasiScanSta(int btn) {
    if (scannerStateSta == 0) {
        if (btn == BTN_LEFT) appMode = 1; // Balik ke WiFi Scanner
        else if (btn == BTN_RIGHT || btn == BTN_OK) {
            totalStation = 0;
            scannerStateSta = 1;
            triggerScanSta = true;
            scanStaDone = false;
        }
    } 
    else if (scannerStateSta == 2) {
        if (btn == BTN_LEFT) scannerStateSta = 0; // Mau scan lagi
        else if (btn == BTN_UP) {
            if (cursorInScanSta > 0) cursorInScanSta--; 
            else if (scrollPosScanner > 0) scrollPosScanner--;
        } 
        else if (btn == BTN_DOWN) {
            if (cursorInScanSta < 2 && (scrollPosScanner + cursorInScanSta) < (totalStation - 1)) cursorInScanSta++; 
            else if ((scrollPosScanner + 3) < totalStation) scrollPosScanner++;
        }
        else if (btn == BTN_OK) {
            if (totalStation > 0) {
                targetLockedIdx = scrollPosScanner + cursorInScanSta; 
                targetSta = listStation[targetLockedIdx];
                adaTargetSta = true; 
                scannerStateSta = 4;   
                contextCursor = 0;  
            }
        }
    } 
    else if (scannerStateSta == 3) {
        if (btn == BTN_LEFT || btn == BTN_OK) scannerStateSta = 4; 
    } 
    else if (scannerStateSta == 4) {
        if (btn == BTN_UP) contextCursor = (contextCursor > 0) ? contextCursor - 1 : 1; 
        else if (btn == BTN_DOWN) contextCursor = (contextCursor < 1) ? contextCursor + 1 : 0;
        else if (btn == BTN_OK) {
            if (contextCursor == 0) {
                // MULAI ATTACK KE HP
                isDeauthSta = true; 
                scannerStateSta = 0;
                appMode = 2; // Pindah layar ke animasi Deauth
            } 
            else if (contextCursor == 1) {
                scannerStateSta = 3; // Lihat Detail
            }
        }
        else if (btn == BTN_LEFT) scannerStateSta = 2; 
    }
}


void handleNavigasiDeauth(int btn) {
    if (deauthState == 0) { 
        if (btn == BTN_LEFT) appMode = 0; 
        else if (btn == BTN_RIGHT || btn == BTN_OK) { 
            deauthState = 1;
            isDeauthing = true;
        }
    } 
    else if (deauthState == 1) { 
        if (btn == BTN_LEFT) { 
            esp_wifi_stop(); 
            isDeauthing = false;
            deauthState = 0;
            appMode = 0; 
        }
    }
}

void handleNavigasiSpam(int btn) {
    if (spamState == 0) { 
        if (btn == BTN_RIGHT || btn == BTN_OK) { 
            spamState = 1;
            isSpamming = true; 
        } 
        else if (btn == BTN_LEFT) { 
            esp_wifi_stop(); 
            appMode = 0; 
            isSpamming = false;
            aktifModeSpam = 0; 
        }
    } 
    else if (spamState == 1) { 
        if (btn == BTN_LEFT) { 
            isSpamming = false;
            spamState = 0;
            appMode = 0; 
            aktifModeSpam = 0;
        }
    }
}
