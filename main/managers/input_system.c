#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "globals.h"
#include "esp_wifi.h"
#include <string.h>
#include "ssd1306.h"

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
void handleNavigasiScanSta(int btn);
void handleInputPassword(int btn);
void handleDinoInput(int btn);







void handleJoystick() {
    static uint32_t lastPress = 0;
    uint32_t debounceLimit = (appMode == 8) ? 120 : 250; 
    if (input_millis() - lastPress < debounceLimit) return; 

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
     if (appMode == 8) { handleInputPassword(btn); lastPress = input_millis(); return; }
     if (appMode == 11) { handleDinoInput(btn); lastPress = input_millis(); return; }
    
     
    if (appMode == 9) { // Misal appMode 11 itu Connected WiFi
    if (btn == BTN_LEFT) appMode = 0; // Back ke menu utama
    if (btn == BTN_RIGHT && isWiFiConnected) {
        triggerDisconnect = true; // Putusin WiFi
    }
    lastPress = input_millis();
        return;
}
     if (appMode == 6) {
if (btn == BTN_LEFT) { scannerState = 4;  appMode = 1; triggerTrack = false; }
lastPress = input_millis();
        return;
}
     if (appMode == 7) {
        if (btn == BTN_LEFT) { 
            esp_wifi_stop(); 
            isDeauthSta = false;
            appMode = 5; 
    }
    lastPress = input_millis();
        return;
}
    // Mode 3 (Brightness)
    if (appMode == 3) {
        if (btn == BTN_UP) {
            if (brightnessValue < 245) brightnessValue += 10;
            setOledBrightness(brightnessValue);
            printf("Brightness set to: %d\n", brightnessValue);
        }
        else if (btn == BTN_DOWN) {
            if (brightnessValue > 10) brightnessValue -= 10;
            setOledBrightness(brightnessValue);
            printf("Brightness set to: %d\n", brightnessValue);
        }
        else if (btn == BTN_LEFT) appMode = 0;
        
        lastPress = input_millis();
        return;
    }










    // --- BATAS TAMBAHAN MENU UTAMA ---
    if (!inSubMenu) {
        if (btn == BTN_RIGHT) {
            currentMenu = (currentMenu + 1) % 5; 
            lastPress = input_millis();
        }
        else if (btn == BTN_LEFT) {
            currentMenu = (currentMenu - 1 + 5) % 5; 
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
            if(currentMenu == 0)      limitMenu = 5; 
            else if(currentMenu == 1) limitMenu = 3;
            else if(currentMenu == 2) limitMenu = 5;
            else if(currentMenu == 3) limitMenu = 4;
            else limitMenu = 1;

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
            } else if (currentMenu == 0 && currentSub == 1) {
                appMode = 1;
                scannerState = 2;     
                cursorInScanner = 0;  
                scrollPosScanner = 0; 
            } else if (currentMenu == 3 && currentSub == 0) { 
                appMode = 3; 
            } else if (currentMenu == 0 && currentSub == 2) {
                appMode = 9;
            } else if (currentMenu == 0 && currentSub == 3) {
                aktifModeSpam = 1; 
                appMode = 4;       
                spamState = 0;
            } else if (currentMenu == 0 && currentSub == 4) {
                aktifModeSpam = 2; 
                appMode = 4;
                spamState = 0;
            } else if (currentMenu == 4 && currentSub == 0) {
                appMode = 11;
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
               // triggerTrackWifi = true;
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
            contextCursor = (contextCursor > 0) ? contextCursor - 1 : 4; // Batas atas ke 2
        } 
        else if (btn == BTN_DOWN) {
            contextCursor = (contextCursor < 4) ? contextCursor + 1 : 0; // Batas bawah ke 2
        }
        
        else if (btn == BTN_OK) {
        if (contextCursor == 0) { appMode = 2; deauthState = 0; } 
        else if (contextCursor == 1) {
        if (targetTerkunci.is_open) { 
            triggerConnect = true; 
            inputPassword[0] = '\0'; // Kosongin pass
        } else {
            appMode = 8; // Pindah ke screen input password (roller)
        }
        }
        else if (contextCursor == 2) { appMode = 5; scannerStateSta = 0; contextCursor = 0; }
        else if (contextCursor == 3) { // TRACK
            appMode = 6; 
            triggerTrack = true; // Nyalain update RSSI
        }
        else if (contextCursor == 4) { scannerState = 3; }
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
        if (btn == BTN_LEFT) { appMode = 1;  triggerScanSta = false; }// Mau scan lagi
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
                appMode = 7; // Pindah layar ke animasi Deauth
            } 
            else if (contextCursor == 1) {
                scannerStateSta = 3; // Lihat Detail
            }
        }
        else if (btn == BTN_LEFT) { scannerStateSta = 2;}
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
            appMode = 1; 
        }
    }
}

// --- TARUH INI DI LUAR FUNGSI (Di atas handleInputPassword) ---
int kbX = 0;
int kbY = 0;
int kbPage = 0; // 0 = Huruf Kecil, 1 = Huruf Gede/Simbol

// Layout QWERTY Mini (4 Baris x 10 Kolom)
const char kbLayout[2][4][10] = {
    { // PAGE 0: Huruf Kecil & Angka
        {'1','2','3','4','5','6','7','8','9','0'},
        {'q','w','e','r','t','y','u','i','o','p'},
        {'a','s','d','f','g','h','j','k','l','@'},
        {'z','x','c','v','b','n','m','<','*','>'}
    },
    { // PAGE 1: Huruf Gede & Simbol
        {'!','#','$','%','&','-','+','(',')','='},
        {'Q','W','E','R','T','Y','U','I','O','P'},
        {'A','S','D','F','G','H','J','K','L','_'},
        {'Z','X','C','V','B','N','M','<','*','>'}
    }
};

// --- GANTI FUNGSI INI FULL ---
void handleInputPassword(int btn) {
    if (btn == BTN_UP) {
        kbY--; if(kbY < 0) kbY = 3; // Teleport ke bawah
    } 
    else if (btn == BTN_DOWN) {
        kbY++; if(kbY > 3) kbY = 0; // Teleport ke atas
    } 
    else if (btn == BTN_LEFT) {
        kbX--; if(kbX < 0) kbX = 9; // Teleport ke kanan
    } 
    else if (btn == BTN_RIGHT) {
        kbX++; if(kbX > 9) kbX = 0; // Teleport ke kiri
    } 
    else if (btn == BTN_OK) {
        char selected = kbLayout[kbPage][kbY][kbX];
        int len = strlen(inputPassword);
        
        if (selected == '<') { 
            // TOMBOL BACKSPACE (Hapus 1 huruf)
            if(len > 0) inputPassword[len - 1] = '\0';
            else appMode = 1; // Kalo udah kosong dipencet backspace, balik ke menu
        } 
        else if (selected == '*') { 
            // TOMBOL SHIFT (Ganti halaman keyboard)
            kbPage = !kbPage; 
        }
        else if (selected == '>') { 
            // TOMBOL CONNECT (ENTER)
            if (len >= 8) {
                triggerConnect = true;
                appMode = 10; // Pindah ke layar Connecting
            }
        }
        else { 
            // NGETIK HURUF
            if (len < 63) {
                inputPassword[len] = selected;
                inputPassword[len + 1] = '\0';
            }
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




void handleDinoInput(int btn) {
    if (dinoState == 0) {
        if ((btn == BTN_OK || btn == BTN_UP) && !isJumping) {
            dinoVy = -8.5; // Lompatan disesuaikan buat berat Dino 24px
            isJumping = true;
        }
    } else {
        if (btn == BTN_OK) { 
            // Reset Game
            dinoScore = 0;
            rawScore = 0;
            gameSpeed = 3.0;
            obstacleX = 128;
            dinoY = 36;
            dinoVy = 0;
            isJumping = false;
            dinoState = 0;
            endTimer = 0;
        }
    }

    // Tombol Keluar (Kiri)
    if (btn == BTN_LEFT) {
        ssd1306_invert_display(0, false); // Pastiin layar gak nyangkut item
        appMode = 0; 
        dinoScore = 0; rawScore = 0;
        obstacleX = 128;
        dinoState = 0;
    }
}
