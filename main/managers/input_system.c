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
extern void tampilkanMenuSavedIR(void);

extern void transmit_ir_raw(uint16_t* pulses, int num_pulses);

extern void loadSavedRemotes(void);
extern void hapus_remote_di_sd(int index_target);


// Pengganti millis()
uint32_t input_millis() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}
static uint32_t pressTime = 0;






// Deklarasi fungsi di bawah
void handleNavigasiScanner(int btn);
void handleNavigasiDeauth(int btn);
void handleNavigasiSpam(int btn);
void handleNavigasiScanSta(int btn);
void handleEvilTwinInput(int btn);
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

    // ==========================================
    // 2. HANDLER SUB-APLIKASI (DIPISAH & DI-REM)
    // ==========================================
    if (appMode == 12) {
        static uint32_t pressTime = 0; // Buat ngitung lama ditahan

        // 1. LOGIKA BELOK ULAR
        if (btn == BTN_RIGHT && snakeDir != 2) {
            snakeDir = 0; // Kanan
        }
        else if (btn == BTN_DOWN && snakeDir != 3) {
            snakeDir = 1; // Bawah
        }
        else if (btn == BTN_LEFT && snakeDir != 0 && snakeState == 0) {
            snakeDir = 2; // Kiri
        }
        else if (btn == BTN_UP && snakeDir != 1) {
            snakeDir = 3; // Atas
        }

        // 2. LOGIKA KELUAR PAS GAME OVER (Mati)
        if (snakeState == 1) {
            if (btn == BTN_OK) { // Restart Game
                snakeState = 0;     
                isSnakeInitialized = false; 
            } else if (btn == BTN_LEFT) { // Keluar Ke Menu Utama
                appMode = 0; 
                isSnakeInitialized = false;
            }
        }

        // 3. LOGIKA PANIC BUTTON (Tahan OK buat keluar pas lagi main)
        if (btn == BTN_OK && snakeState == 0) {
            if (pressTime == 0) pressTime = input_millis(); // Mulai itung
            
            // Kalau ditahan lebih dari 1500ms (1.5 detik)
            if (input_millis() - pressTime > 1500) {
                appMode = 0; // Langsung mental ke Menu Utama
                isSnakeInitialized = false;
                pressTime = 0;
            }
        } else {
            pressTime = 0; // Kalau tombol dilepas, reset itungan
        }

        lastPress = input_millis(); // Update waktu terakhir dipencet
        return; 
    

    }

    if (appMode == MODE_IR_SNIFFER) { 
        if (btn == BTN_OK || btn == BTN_RIGHT) {
            if (currentIRState == IR_STATE_CONFIRM) {
                currentIRState = IR_STATE_WAITING;
                triggerReadIR = true; 
            }
            else if (currentIRState == IR_STATE_RESULT) {
                currentIRState = IR_STATE_CONFIRM; // Scan ulang
            }
        }
        else if (btn == BTN_LEFT) { 
            if (currentIRState == IR_STATE_WAITING) {
                triggerReadIR = false;
                currentIRState = IR_STATE_CONFIRM; // Batalin nunggu
            } 
            else if (currentIRState == IR_STATE_RESULT) {
                appMode = 0;// Kembali dari hasil scan
            }
            else {
                appMode = 0; // Balik ke Submenu dengan benar
            }
        }
        lastPress = input_millis();
        return; // <--- KUNCI SAKTI BIAR GAK BOCOR KE MENU UTAMA
    }

    if (appMode == MODE_SAVED_REMOTE) {
        if (currentIRSavedState == IR_SAVED_STATE_LIST) {
            if (btn == BTN_DOWN) {
                if (savedRemoteIndex < totalSavedRemotes - 1) savedRemoteIndex++;
            } 
            else if (btn == BTN_UP) {
                if (savedRemoteIndex > 0) savedRemoteIndex--;
            } 
            else if (btn == BTN_OK || btn == BTN_RIGHT) {
                if (totalSavedRemotes > 0) {
                    currentIRSavedState = IR_SAVED_STATE_ACTION;
                    actionMenuIndex = 0; 
                }
            } 
            else if (btn == BTN_LEFT) { 
                appMode = 0; // Balik ke Submenu
            }
        }
        else if (currentIRSavedState == IR_SAVED_STATE_ACTION) {
            if (btn == BTN_DOWN || btn == BTN_UP) {
                actionMenuIndex = !actionMenuIndex; 
            } 
            else if (btn == BTN_LEFT) { 
                currentIRSavedState = IR_SAVED_STATE_LIST; 
            } 
            else if (btn == BTN_OK) {
                if (actionMenuIndex == 0) {
                    currentIRSavedState = IR_SAVED_STATE_SENDING;
                    tampilkanMenuSavedIR(); 
                    transmit_ir_raw(listSavedRemotes[savedRemoteIndex].pulses, listSavedRemotes[savedRemoteIndex].num_pulses);
                    vTaskDelay(pdMS_TO_TICKS(500)); 
                    currentIRSavedState = IR_SAVED_STATE_ACTION; 
                } 
                else if (actionMenuIndex == 1) {
                    hapus_remote_di_sd(savedRemoteIndex); 
                    loadSavedRemotes(); 
                    if (savedRemoteIndex >= totalSavedRemotes) savedRemoteIndex = totalSavedRemotes - 1;
                    if (savedRemoteIndex < 0) savedRemoteIndex = 0;
                    currentIRSavedState = IR_SAVED_STATE_LIST; 
                }
            }
        }
        lastPress = input_millis();
        return; // <--- KUNCI SAKTI 
    }

    // --- HELPER LAINNYA (UDAH AMAN KARENA ADA RETURN) ---
    if (appMode == 1) { handleNavigasiScanner(btn); lastPress = input_millis(); return; }
    if (appMode == 2) { handleNavigasiDeauth(btn);  lastPress = input_millis(); return; }
    if (appMode == 4) { handleNavigasiSpam(btn);    lastPress = input_millis(); return; } 
    if (appMode == 5) { handleNavigasiScanSta(btn); lastPress = input_millis(); return; }
    if (appMode == 8) { handleEvilTwinInput(btn);   lastPress = input_millis(); return; }
    if (appMode == 11){ handleDinoInput(btn);       lastPress = input_millis(); return; }
    if (appMode == 13) { handleTetrisInput(btn);  lastPress = input_millis();  return;  }
    
    
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

    // ==========================================
    // 3. LOGIKA MENU UTAMA (APPMODE == 0)
    // ==========================================
    // DIBUNGKUS IF(0) BIAR MENU LAIN GAK NGERUSAK KURSOR DI SINI!

    if (appMode == 0) {
        if (!inSubMenu) {
            if (btn == BTN_RIGHT) {
                currentMenu = (currentMenu + 1) % 5; 
            }
            else if (btn == BTN_LEFT) {
                currentMenu = (currentMenu - 1 + 5) % 5; 
            }
            else if (btn == BTN_OK) {
                inSubMenu = true; 
                currentSub = 0;   
                topMenu = 0;
            }
        } 
        else { // DI DALAM LIST SUBMENU
            if (btn == BTN_DOWN) {
                int limitMenu = 0; 
                if(currentMenu == 0)      limitMenu = 4; 
                else if(currentMenu == 1) limitMenu = 3;
                else if(currentMenu == 2) limitMenu = 5;
                else if(currentMenu == 3) limitMenu = 4;
                else limitMenu = 3;

                if (currentSub < (limitMenu - 1)) { 
                    currentSub++;
                    if (currentSub >= topMenu + 5) topMenu++;
                }
            }
            else if (btn == BTN_UP) {
                if (currentSub > 0) {
                    currentSub--;
                    if (currentSub < topMenu) topMenu--;
                }
            }
            else if (btn == BTN_LEFT) {
                inSubMenu = false; // BALIK KE LOGO RootX
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
                } else if (currentMenu == 2 && currentSub == 0) { 
                    appMode = MODE_IR_SNIFFER; 
                    currentIRState = IR_STATE_CONFIRM; // Reset tiap masuk menu
                } else if (currentMenu == 2 && currentSub == 4) { 
                    loadSavedRemotes(); // Baca memori SD
                    currentIRSavedState = IR_SAVED_STATE_LIST; 
                    savedRemoteIndex = 0; 
                    appMode = MODE_SAVED_REMOTE; 
                } else if (currentMenu == 0 && currentSub == 2) {
                    aktifModeSpam = 1; 
                    appMode = 4;       
                    spamState = 0;
                } else if (currentMenu == 0 && currentSub == 3) {
                    aktifModeSpam = 2; 
                    appMode = 4;
                    spamState = 0;
                } else if (currentMenu == 4 && currentSub == 0) {
                    appMode = 11;
                } else if (currentMenu == 4 && currentSub == 1) {
                    appMode = 12;
                    } else if (currentMenu == 4 && currentSub == 2) {
                    appMode = 13;
                    }
            }
        }
        lastPress = input_millis();
        return;
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
           appMode = 8;
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


void handleEvilTwinInput(int btn) {
    if (evilTwinState == 0) {
        if (btn == BTN_RIGHT || btn == BTN_OK) {
            triggerEvilTwin = true;
            
        } else if (btn == BTN_LEFT) {
            appMode = 1; // Balik ke menu scanner
        }
    } else if (evilTwinState == 2) {
        if (btn == BTN_LEFT || btn == BTN_OK) {
            isEvilTwin = false;
            esp_wifi_stop();
            appMode = 1;
        } 
    } else if (evilTwinState == 1) { if (btn == BTN_LEFT) {
            appMode = 1; // Balik ke menu scanner
            isEvilTwin = false;
            esp_wifi_stop();
        }}
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



// Variabel statis untuk mesin gamenya
static uint8_t tetris_grid[20][10]; // 20 baris (X), 10 kolom (Y)
static int t_shape, t_rot, t_x, t_y; 
static uint32_t lastFallTime = 0;

// Data 7 Balok Tetris (Bitmask 16-bit biar enteng)
static const uint16_t tetris_shapes[7][4] = {
    {0x0F00, 0x2222, 0x00F0, 0x4444}, // I (Lurus)
    {0x8E00, 0x6440, 0x0E20, 0x4C40}, // J
    {0x2E00, 0x4460, 0x0E80, 0xC440}, // L
    {0xCC00, 0xCC00, 0xCC00, 0xCC00}, // O (Kotak)
    {0x6C00, 0x4620, 0x06C0, 0x8C40}, // S
    {0x4E00, 0x4640, 0x0E40, 0x4C40}, // T
    {0xC600, 0x2640, 0x0C60, 0x4C80}  // Z
};

// Fungsi Cek Tabrakan
bool check_tetris_col(int shape, int rot, int px, int py) {
    uint16_t p = tetris_shapes[shape][rot];
    for (int i=0; i<16; i++) {
        if (p & (0x8000 >> i)) {
            int grid_x = px + (i % 4);
            int grid_y = py + (i / 4);
            if (grid_x < 0 || grid_x >= 10 || grid_y >= 20) return true; // Nabrak tembok
            if (grid_y >= 0 && tetris_grid[grid_y][grid_x]) return true; // Nabrak balok lain
        }
    }
    return false;
}

// Handler Input biar input_system lu bersih!
void handleTetrisInput(int btn) {
    if (tetrisState == 0) {
        if (btn == BTN_UP) { // Geser ke "Kiri" (Naik di OLED)
            if (!check_tetris_col(t_shape, t_rot, t_x - 1, t_y)) t_x--;
        } else if (btn == BTN_DOWN) { // Geser ke "Kanan" (Turun di OLED)
            if (!check_tetris_col(t_shape, t_rot, t_x + 1, t_y)) t_x++;
        } else if (btn == BTN_RIGHT) { // Mempercepat jatuh (Soft Drop)
            if (!check_tetris_col(t_shape, t_rot, t_x, t_y + 1)) t_y++;
        } else if (btn == BTN_OK) { // Putar Balok
            int next_rot = (t_rot + 1) % 4;
            if (!check_tetris_col(t_shape, next_rot, t_x, t_y)) t_rot = next_rot;
        } else if (btn == BTN_LEFT) { // Keluar Game
            extern int appMode; appMode = 0; isTetrisInitialized = false;
        }
    } else { // Pas Game Over
        if (btn == BTN_OK) { tetrisState = 0; isTetrisInitialized = false; }
        else if (btn == BTN_LEFT) { extern int appMode; appMode = 0; isTetrisInitialized = false; }
    }
}