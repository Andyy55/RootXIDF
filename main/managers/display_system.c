#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "globals.h"
#include "photo_data.h"
#include "ssd1306.h"
#include "i2c.h"
#include <math.h>


#define WHITE 1
#define BLACK 0


// --- DATA UNTUK ANIMASI BINTANG ---
typedef struct {
    float x, y, z;
} Star;

#define MAX_STARS 15
Star stars[MAX_STARS];
bool starInit = false;

// Inisialisasi bintang pertama kali
extern void oled_draw_bitmap(uint8_t id, int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, ssd1306_color_t color);

uint32_t millis() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// Fungsi animasi wave
void drawWave() {
    for (int x = 0; x < 128; x++) {
        int y = 60 + (int)(sin((x + (int)millis() / 10) * 0.1) * 3);
        ssd1306_draw_pixel(0, x, y, WHITE);
    }
}

// Fungsi bounce buat icon
int getBounce(int speed, int range) {
    return (int)(sin(millis() / (float)speed) * range);
}

// Fungsi loading bar yang bener
void drawLoadingBar(int x, int y, int w, int h, int progress) {
    ssd1306_draw_rectangle(0, x, y, w, h, WHITE);
    int fillW = (w * progress) / 100;
    if (fillW > w) fillW = w;
    ssd1306_fill_rectangle(0, x, y, fillW, h, WHITE);
    
    int offset = (millis() / 50) % 20;
    for(int i = -20; i < fillW; i += 15) {
        int lineX = x + i + offset;
        if(lineX > x && lineX < x + fillW) {
            // Kalau ssd1306_draw_line gak ada, pake vline aja buat efek
            for(int j=0; j<h; j++) ssd1306_draw_pixel(0, lineX, y+j, BLACK);
        }
    }
}
// Fungsi gambar bintang gerak (Starfield)
void drawStarfield() {
    if (!starInit) initStars();
    
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].z -= 0.5; // Kecepatan bintang maju
        if (stars[i].z <= 1) {
            stars[i].z = 64;
            stars[i].x = (rand() % 128) - 64;
            stars[i].y = (rand() % 64) - 32;
        }

        // Proyeksi 3D ke 2D
        int sx = (int)(stars[i].x / stars[i].z * 64 + 64);
        int sy = (int)(stars[i].y / stars[i].z * 32 + 32);

        if (sx >= 0 && sx < 128 && sy >= 10 && sy < 54) { // Filter biar gak kena header/footer
            ssd1306_draw_pixel(0, sx, sy, WHITE);
        }
    }
}

// Deklarasi bitmap solver yang ada di boot_system.c biar file ini bisa make juga

// ==========================================
// FUNGSI PEMBANTU PENGGANTI ARDUINO
// ==========================================


long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ==========================================
// DATA MENU 
// ==========================================
const unsigned char* iconListWiFi[] = { iconSmall_scan, iconSmall_spam, iconSmall_sniff, iconSmall_wifi };
const unsigned char* iconListBLE[]  = { iconSmall_scan, iconSmall_apple, iconSmall_android };
const unsigned char* iconListIR[]   = { iconSmall_ir, iconSmall_tv, iconSmall_ac, iconSmall_lock, iconSmall_saved };
const unsigned char* iconListSet[]  = { iconSmall_bright, iconSmall_wifi, iconSmall_info, iconSmall_repeat };

const char* subMenuWiFi[] = { "Scan WiFi", "Beacon Spam", "List Scan", "RickRoll SSID" };
const char* subMenuBLE[]  = { "BLE Scanner", "Spam Apple", "Spam Android" };
const char* subMenuIR[]   = { "Read Signal", "TV B-Gone", "AC Remote", "Brute Force", "Saved Remotes" };
const char* subMenuSet[]  = { "Brightness", "WiFi Setup", "About RootX", "Reboot" };

// ==========================================
// LOGIKA TAMPILAN
// ==========================================

void tampilkanMenuLogo() {
    ssd1306_clear(0);
    drawStarfield();
    drawWave();
    
    if(currentMenu == 0)      ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: WIFI", WHITE, BLACK);
    else if(currentMenu == 1) ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: BLE", WHITE, BLACK);
    else if(currentMenu == 2) ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: IR", WHITE, BLACK);
    else                      ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: SETS", WHITE, BLACK);
    
    ssd1306_draw_hline(0, 0, 9, 128, WHITE);

    const unsigned char* bigIcon;
    if(currentMenu == 0)      bigIcon = logo_wifi_32; 
    else if(currentMenu == 1) bigIcon = logo_ble_32;
    else if(currentMenu == 2) bigIcon = logo_ir_32;
    else                      bigIcon = logo_settings_32;

    int iconBounce = getBounce(250, 2); // Loncat 2 pixel
    oled_draw_bitmap(0, 2, 22, + iconBounce, bigIcon, 32, 32, WHITE);

    // Font library ini ukurannya fix, jadi kita akalin kursornya aja
    ssd1306_draw_string_adafruit(0, 20, 30, "<", WHITE, BLACK);
    ssd1306_draw_string_adafruit(0, 95, 30, ">", WHITE, BLACK);
    ssd1306_draw_string_adafruit(0, 40, 56, ">SELECT<", WHITE, BLACK); 
    
    ssd1306_refresh(0, true);
}

void tampilkanMenuUtama() { 
    ssd1306_clear(0);
    drawStarfield();
    drawWave();
    int totalSub = 0; 

    if(currentMenu == 0)      { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: WIFI", WHITE, BLACK); totalSub = 4; }
    else if(currentMenu == 1) { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: BLE ", WHITE, BLACK); totalSub = 3; }
    else if(currentMenu == 2) { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: IR", WHITE, BLACK);   totalSub = 5; }
    else                      { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: SETS", WHITE, BLACK); totalSub = 4; }
    
    ssd1306_draw_hline(0, 0, 9, 128, WHITE);

    for(int i = 0; i < 5; i++) {
        int itemIndex = topMenu + i;
        if(itemIndex >= totalSub) break; 

        int yPos = 13 + (i * 10); 
        int textColor = WHITE;
        int bgColor = BLACK;
        
        if(itemIndex == currentSub) { 
            ssd1306_fill_rectangle(0, 0, yPos - 1, 128, 10, WHITE);
            textColor = BLACK; 
            bgColor = WHITE;
        } 
        
        const unsigned char* iconSmall;
        if(currentMenu == 0)      iconSmall = iconListWiFi[itemIndex]; 
        else if(currentMenu == 1) iconSmall = iconListBLE[itemIndex];
        else if(currentMenu == 2) iconSmall = iconListIR[itemIndex];
        else                      iconSmall = iconListSet[itemIndex]; 

        
        int iconBounce = getBounce(250, 2); // Loncat 2 pixel
        oled_draw_bitmap(0, 2, (yPos - 1) + iconBounce, iconSmall, 10, 10, textColor);
        
        const char* textToPrint = "";
        if(currentMenu == 0)      textToPrint = subMenuWiFi[itemIndex];
        else if(currentMenu == 1) textToPrint = subMenuBLE[itemIndex];
        else if(currentMenu == 2) textToPrint = subMenuIR[itemIndex];
        else                      textToPrint = subMenuSet[itemIndex];

        ssd1306_draw_string_adafruit(0, 18, yPos, (char*)textToPrint, textColor, bgColor);
    }
    ssd1306_refresh(0, true);
}

void tampilkanTrackScreen() {
    ssd1306_clear(0);
    char buf[32];
    
    // --- ANIMASI FLOATING ICON (Icon WiFi naik turun pelan) ---
    int floatY = 15 + (int)(sin(millis() / 300.0) * 3);
    oled_draw_bitmap(0, 105, floatY, iconSmall_wifi, 10, 10, WHITE);

    // Header
    ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
    ssd1306_draw_string_adafruit(0, 30, 1, "TRACKING RSSI", BLACK, WHITE);
    
    // --- ANIMASI RADAR PULSING ---
    static int r = 0;
    r++; if(r > 20) r = 0;
    ssd1306_draw_circle(0, 64, 32, r, WHITE);
    if(r > 10) ssd1306_draw_circle(0, 64, 32, r - 10, WHITE);

    // Data RSSI
    snprintf(buf, sizeof(buf), "%d", targetTerkunci.rssi);
    ssd1306_draw_string_adafruit(0, 50, 30, buf, WHITE, BLACK);

    // Footer
    ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
    ssd1306_draw_string_adafruit(0, 5, 55, "< BACK", BLACK, WHITE);
    
    ssd1306_refresh(0, true);
}


void tampilkanWifiScanner() {
    ssd1306_clear(0);
    char buf[64]; 

    if (scannerState == 0) {
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 1, "WIFI SCANNER", BLACK, WHITE);

        ssd1306_draw_string_adafruit(0, 40, 25, "Yakin??", WHITE, BLACK);

        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< CANCEL", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 95, 55, "YES >", BLACK, WHITE);
    }
    else if (scannerState == 1) {
        ssd1306_draw_string_adafruit(0, 20, 25, "Scanning Air...", WHITE, BLACK);
        if (scanDone) scannerState = 2; 
    }
    else if (scannerState == 2) {
        if (totalWiFi == 0) {
            ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
            ssd1306_draw_string_adafruit(0, 2, 1, "SAVED NETWORKS", BLACK, WHITE);
            ssd1306_draw_string_adafruit(0, 15, 25, "BELUM ADA DATA!", WHITE, BLACK);
            ssd1306_draw_string_adafruit(0, 10, 35, "Scan WiFi dulu", WHITE, BLACK);
            ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
            ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
        } else {
            ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
            snprintf(buf, sizeof(buf), "SCANNER - %d", totalWiFi);
            ssd1306_draw_string_adafruit(0, 2, 1, buf, BLACK, WHITE);
            
            for (int i = 0; i < 3; i++) {
                int itemIdx = scrollPosScanner + i;
                if (itemIdx < totalWiFi) {
                    int yPos = 14 + (i * 13);
                    int textColor = WHITE;
                    int bgColor = BLACK;
                    
                    if (i == cursorInScanner) {
                        ssd1306_fill_rectangle(0, 0, yPos - 1, 128, 12, WHITE);
                        textColor = BLACK;
                        bgColor = WHITE;
                    }

                    snprintf(buf, sizeof(buf), "%d.", listWiFi[itemIdx].id);
                    ssd1306_draw_string_adafruit(0, 1, yPos + 1, buf, textColor, bgColor);
                    
                    int maxChar = 8;
                    int len = strlen(listWiFi[itemIdx].ssid);
                    char textShow[16] = {0};

                    if (i == cursorInScanner && len > maxChar) {
                        int kelebihan = len - maxChar;
                        int offset = (millis() / 300) % (kelebihan + 4); 
                        if (offset > kelebihan) offset = kelebihan; 
                        strncpy(textShow, listWiFi[itemIdx].ssid + offset, maxChar);
                    } else {
                        if (len > maxChar) strncpy(textShow, listWiFi[itemIdx].ssid, maxChar);
                        else               strcpy(textShow, listWiFi[itemIdx].ssid);
                    }
                    ssd1306_draw_string_adafruit(0, 20, yPos + 1, textShow, textColor, bgColor);

                    snprintf(buf, sizeof(buf), "C:%d", listWiFi[itemIdx].channel);
                    ssd1306_draw_string_adafruit(0, 65, yPos + 1, buf, textColor, bgColor);
                    snprintf(buf, sizeof(buf), "%ddB", listWiFi[itemIdx].rssi);
                    ssd1306_draw_string_adafruit(0, 95, yPos + 1, buf, textColor, bgColor);
                }
            }
            ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
            ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
            ssd1306_draw_string_adafruit(0, 53, 55, "[OK]", BLACK, WHITE);
        }
    }
    else if (scannerState == 3) {
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 22, 1, "DETAIL TARGET", BLACK, WHITE);

        int xSide = 5; 
        
        ssd1306_draw_string_adafruit(0, xSide, 13, "SSID: ", WHITE, BLACK);
        int lenSSID = strlen(targetTerkunci.ssid);
        char tmpSSID[20] = {0};
        if (lenSSID > 14) {
            int kelebihan = lenSSID - 10; 
            int offset = (millis() / 250) % (kelebihan + 4);
            if (offset > kelebihan) offset = kelebihan;
            strncpy(tmpSSID, targetTerkunci.ssid + offset, 14);
        } else {
            strcpy(tmpSSID, targetTerkunci.ssid);
        }
        ssd1306_draw_string_adafruit(0, xSide + 35, 13, tmpSSID, WHITE, BLACK);

        ssd1306_draw_string_adafruit(0, xSide, 23, "MAC : ", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, xSide + 35, 23, targetTerkunci.mac, WHITE, BLACK);
        
        snprintf(buf, sizeof(buf), "CH  : %d", targetTerkunci.channel);
        ssd1306_draw_string_adafruit(0, xSide, 33, buf, WHITE, BLACK);

        snprintf(buf, sizeof(buf), "SIG : %d dBm", targetTerkunci.rssi);
        ssd1306_draw_string_adafruit(0, xSide, 43, buf, WHITE, BLACK);

        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "[<] BACK", BLACK, WHITE);
    } 
            else if (scannerState == 4) {
            drawStarfield();
        // --- 1. HEADER (Tetap) ---
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 43, 1, "ACTIONS", BLACK, WHITE);

        // --- 2. BLOK PUTIH FOKUS (DI TENGAH) ---
        // Posisinya statis di tengah layar buat nandain menu yang kepilih
        ssd1306_fill_rectangle(0, 0, 24, 128, 18, WHITE);
        
        drawSmartSelection(24);

        // Kita looping semua menu (ada 4: Deauth, Scan Client, Track, Details)
        for(int i = 0; i < 4; i++) {
            const char* teks;
            const unsigned char* icon;
            
            // Setting Teks & Icon
            if(i == 0)      { teks = "DEAUTH";  icon = iconSmall_skull; }
            else if(i == 1) { teks = "CLIENTS"; icon = iconSmall_sniff; }
            else if(i == 2) { teks = "TRACK";   icon = iconSmall_wifi;  } // Pakai icon wifi/signal
            else            { teks = "DETAILS"; icon = iconSmall_info;  }

            // --- 3. LOGIKA POSISI DINAMIS ---
            // Jarak antar menu (kalo lagi fokus beda sama lagi gak fokus)
            int yPos;
            int diff = i - contextCursor; // Jarak index menu dari kursor sekarang

            // Rumus posisi: Tengah ada di Y=28. 
            // Menu di atasnya dikasih jarak -15, menu di bawahnya +18
            yPos = 28 + (diff * 18); 

            // Biar menu yang jauh gak ngetimpa header/footer, kita filter yang muncul aja
            if (yPos > 10 && yPos < 50) {
                if (i == contextCursor) {
                    // --- MENU TERPILIH (GEDE + ICON) ---
                    oled_draw_bitmap(0, 5, yPos - 2, icon, 10, 10, BLACK); // Icon di blok putih
                    ssd1306_draw_string_adafruit(0, 22, yPos, (char*)teks, BLACK, WHITE); // Font Size 2 (Kalo lib lu support size di param 3)
                    // Note: Kalo ssd1306_draw_string_adafruit param ke-3 itu size, ganti jadi 2
                } else {
                    // --- MENU GAK TERPILIH (KECIL) ---
                    // Digambar lebih minggir dan font ukuran 1
                    ssd1306_draw_string_adafruit(0, 10, yPos + 2, (char*)teks, WHITE, BLACK);
                }
            }
        }

        // --- 4. FOOTER (Tetap) ---
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
        
        ssd1306_draw_string_adafruit(0, 90, 55, "OK: GO", BLACK, WHITE);
    }


    ssd1306_refresh(0, true);
}


void tampilkanStationScanner() {
    ssd1306_clear(0);
    char buf[64]; 

    if (scannerStateSta == 0) {
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 1, "STATION SCANNER", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 30, 25, "Scan Clients?", WHITE, BLACK);
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 95, 55, "YES >", BLACK, WHITE);
    }
    else if (scannerStateSta == 1) {
        ssd1306_draw_string_adafruit(0, 10, 20, "SNIFFING TARGET:", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 10, 35, targetTerkunci.ssid, WHITE, BLACK); 
        if (scanStaDone) scannerStateSta = 2; 
    }
    else if (scannerStateSta == 2) {
        if (totalStation == 0) {
            ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
            ssd1306_draw_string_adafruit(0, 2, 1, "CLIENT LIST", BLACK, WHITE);
            ssd1306_draw_string_adafruit(0, 15, 25, "NO CLIENTS FOUND!", WHITE, BLACK);
            ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
            ssd1306_draw_string_adafruit(0, 2, 55, "< RESCAN", BLACK, WHITE);
        } else {
            ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
            snprintf(buf, sizeof(buf), "CLIENTS: %d", totalStation);
            ssd1306_draw_string_adafruit(0, 2, 1, buf, BLACK, WHITE);
            
            for (int i = 0; i < 3; i++) {
                int itemIdx = scrollPosScanner + i;
                if (itemIdx < totalStation) {
                    int yPos = 14 + (i * 13);
                    int txtCol = (i == cursorInScanSta) ? BLACK : WHITE;
                    int bgCol = (i == cursorInScanSta) ? WHITE : BLACK;
                    
                    if (i == cursorInScanSta) ssd1306_fill_rectangle(0, 0, yPos - 1, 128, 12, WHITE);

                    snprintf(buf, sizeof(buf), "%d.", listStation[itemIdx].id);
                    ssd1306_draw_string_adafruit(0, 1, yPos + 1, buf, txtCol, bgCol);

                    snprintf(buf, sizeof(buf), "%02X:%02X..%02X:%02X", 
                             listStation[itemIdx].mac[0], listStation[itemIdx].mac[1],
                             listStation[itemIdx].mac[4], listStation[itemIdx].mac[5]);
                    ssd1306_draw_string_adafruit(0, 20, yPos + 1, buf, txtCol, bgCol);

                    snprintf(buf, sizeof(buf), "%ddBm", listStation[itemIdx].rssi);
                    ssd1306_draw_string_adafruit(0, 90, yPos + 1, buf, txtCol, bgCol);
                }
            }
            ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
            ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
            ssd1306_draw_string_adafruit(0, 53, 55, "[OK] ACTION", BLACK, WHITE);
        }
    }
    else if (scannerStateSta == 3) { 
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 22, 1, "TARGET DETAILS", BLACK, WHITE);

        snprintf(buf, sizeof(buf), "MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
                 targetSta.mac[0], targetSta.mac[1], targetSta.mac[2],
                 targetSta.mac[3], targetSta.mac[4], targetSta.mac[5]);
        ssd1306_draw_string_adafruit(0, 5, 15, buf, WHITE, BLACK);

        snprintf(buf, sizeof(buf), "RSSI: %d dBm", targetSta.rssi);
        ssd1306_draw_string_adafruit(0, 5, 25, buf, WHITE, BLACK);

        snprintf(buf, sizeof(buf), "PACKETS: %d", targetSta.paket_count);
        ssd1306_draw_string_adafruit(0, 5, 35, buf, WHITE, BLACK);

        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
    } 
    else if (scannerStateSta == 4) { 
        drawStarfield();
        // --- 1. HEADER (Tetap) ---
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 43, 1, "ACTIONS", BLACK, WHITE);

        // --- 2. BLOK PUTIH FOKUS (DI TENGAH) ---
        // Posisinya statis di tengah layar buat nandain menu yang kepilih
        ssd1306_fill_rectangle(0, 0, 24, 128, 18, WHITE);
        
        drawSmartSelection(24);

        // Kita looping semua menu (ada 4: Deauth, Scan Client, Track, Details)
        for(int i = 0; i < 4; i++) {
            const char* teks;
            const unsigned char* icon;
            
            // Setting Teks & Icon
            if(i == 0)      { teks = "KICK CLIENT";  icon = iconSmall_skull; }
            else            { teks = "DETAILS"; icon = iconSmall_info;  }

            // --- 3. LOGIKA POSISI DINAMIS ---
            // Jarak antar menu (kalo lagi fokus beda sama lagi gak fokus)
            int yPos;
            int diff = i - contextCursor; // Jarak index menu dari kursor sekarang

            // Rumus posisi: Tengah ada di Y=28. 
            // Menu di atasnya dikasih jarak -15, menu di bawahnya +18
            yPos = 28 + (diff * 18); 

            // Biar menu yang jauh gak ngetimpa header/footer, kita filter yang muncul aja
            if (yPos > 10 && yPos < 50) {
                if (i == contextCursor) {
                    // --- MENU TERPILIH (GEDE + ICON) ---
                    oled_draw_bitmap(0, 5, yPos - 2, icon, 10, 10, BLACK); // Icon di blok putih
                    ssd1306_draw_string_adafruit(0, 22, yPos, (char*)teks, BLACK, WHITE); // Font Size 2 (Kalo lib lu support size di param 3)
                    // Note: Kalo ssd1306_draw_string_adafruit param ke-3 itu size, ganti jadi 2
                } else {
                    // --- MENU GAK TERPILIH (KECIL) ---
                    // Digambar lebih minggir dan font ukuran 1
                    ssd1306_draw_string_adafruit(0, 10, yPos + 2, (char*)teks, WHITE, BLACK);
                }
            }
        }

        // --- 4. FOOTER (Tetap) ---
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 90, 55, "OK: GO", BLACK, WHITE);
    }
    ssd1306_refresh(0, true);
}





void drawLoadingBar(int x, int y, int w, int h, int progress) {
    ssd1306_draw_rectangle(0, x, y, w, h, WHITE);
    int fillW = (w * progress) / 100;
    ssd1306_fill_rectangle(0, x, y, fillW, h, WHITE);
    
    // Animasi garis miring jalan di dalem bar (Efek Glossy)
    int offset = (millis() / 50) % 20;
    for(int i = 0; i < fillW; i += 15) {
        int lineX = x + i + offset;
        if(lineX < x + fillW) {
            ssd1306_draw_line(0, lineX, y, lineX - 5, y + h, BLACK);
        }
    }
}


void tampilkanDeauthScreen() {
    ssd1306_clear(0);
    char buf[64];
    
    if (deauthState == 0) {
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 26, 1, "DEAUTH ATTACK", BLACK, WHITE);
        
        ssd1306_draw_string_adafruit(0, 10, 25, "Attack Target?", WHITE, BLACK);
        
        char shortSsid[16];
        strncpy(shortSsid, targetTerkunci.ssid, 15);
        shortSsid[15] = '\0';
        ssd1306_draw_string_adafruit(0, 10, 35, shortSsid, WHITE, BLACK);

        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< NO", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 95, 55, "YES >", BLACK, WHITE);
    } 
    else if (deauthState == 1) {
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 1, "ATTACKING...", BLACK, WHITE);

        snprintf(buf, sizeof(buf), "Target: %s", targetTerkunci.ssid);
        ssd1306_draw_string_adafruit(0, 0, 20, buf, WHITE, BLACK);
        snprintf(buf, sizeof(buf), "Ch: %d", targetTerkunci.channel);
        ssd1306_draw_string_adafruit(0, 0, 30, buf, WHITE, BLACK);
        
        
        drawLoadingBar(14, 45, 100, 8, deauthProgress);
        
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< STOP ATTACK", BLACK, WHITE);
    }
    ssd1306_refresh(0, true);
}

void tampilkanBrightness() {
    ssd1306_clear(0);
    char buf[16];

    ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
    ssd1306_draw_string_adafruit(0, 35, 1, "BRIGHTNESS", BLACK, WHITE);

    ssd1306_draw_rectangle(0, 14, 28, 100, 12, WHITE); 
    
    int barWidth = map(brightnessValue, 0, 255, 0, 96);
    ssd1306_fill_rectangle(0, 16, 30, barWidth, 8, WHITE);

    int persen = map(brightnessValue, 0, 255, 0, 100);
    snprintf(buf, sizeof(buf), "%d%%", persen);
    ssd1306_draw_string_adafruit(0, 55, 45, buf, WHITE, BLACK);

    ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
    ssd1306_draw_string_adafruit(0, 5, 55, "[<] BACK", BLACK, WHITE);
    ssd1306_draw_string_adafruit(0, 75, 55, "[UP/DN] SET", BLACK, WHITE);

    ssd1306_refresh(0, true);
}

void setOledBrightness(uint8_t level) {
    // Biarin kosong dulu, belum prioritas
i2c_write(0x3C << 1); // Alamat OLED (0x3C) + Write bit
    i2c_write(0x00);      // 0x00 artinya byte selanjutnya adalah COMMAND
    i2c_write(0x81);      // Register Kontras (Brightness)
    i2c_write(level);     // Nilai dari lu (0-255)
    i2c_stop();
}

void tampilkanSpamScreen(const char* judul, const char* subTeks) {
    ssd1306_clear(0);
    char buf[64];
    
    if (spamState == 0) {
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 1, (char*)judul, BLACK, WHITE);
        
        ssd1306_draw_string_adafruit(0, 10, 25, (char*)subTeks, WHITE, BLACK);

        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< NO", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 95, 55, "YES >", BLACK, WHITE);
    } 
    else if (spamState == 1) {
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 1, "RUNNING...", BLACK, WHITE);

        snprintf(buf, sizeof(buf), "Mode: %s", subTeks);
        ssd1306_draw_string_adafruit(0, 0, 25, buf, WHITE, BLACK);
        
        int bar = (millis() * 30) % 128;
        ssd1306_draw_hline(0, 0, 45, bar, WHITE);
        
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< STOP", BLACK, WHITE);
    }
    ssd1306_refresh(0, true);
}
