#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "globals.h"
#include "photo_data.h"
#include "ssd1306.h"

#define WHITE 1
#define BLACK 0

// Deklarasi bitmap solver yang ada di boot_system.c biar file ini bisa make juga
extern void oled_draw_bitmap(uint8_t id, int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, ssd1306_color_t color);

// ==========================================
// FUNGSI PEMBANTU PENGGANTI ARDUINO
// ==========================================
uint32_t millis() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

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

    oled_draw_bitmap(0, 48, 22, bigIcon, 32, 32, WHITE);

    // Font library ini ukurannya fix, jadi kita akalin kursornya aja
    ssd1306_draw_string_adafruit(0, 20, 30, "<", WHITE, BLACK);
    ssd1306_draw_string_adafruit(0, 95, 30, ">", WHITE, BLACK);
    ssd1306_draw_string_adafruit(0, 40, 56, ">SELECT<", WHITE, BLACK); 
    
    ssd1306_refresh(0, true);
}

void tampilkanMenuUtama() { 
    ssd1306_clear(0);
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

        oled_draw_bitmap(0, 2, yPos - 1, iconSmall, 10, 10, textColor);
        
        const char* textToPrint = "";
        if(currentMenu == 0)      textToPrint = subMenuWiFi[itemIndex];
        else if(currentMenu == 1) textToPrint = subMenuBLE[itemIndex];
        else if(currentMenu == 2) textToPrint = subMenuIR[itemIndex];
        else                      textToPrint = subMenuSet[itemIndex];

        ssd1306_draw_string_adafruit(0, 18, yPos, (char*)textToPrint, textColor, bgColor);
    }
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
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 43, 1, "ACTIONS", BLACK, WHITE);

        for(int i = 0; i < 2; i++) {
            int yPos = 20 + (i * 15); 
            int colorTheme = (contextCursor == i) ? BLACK : WHITE;
            int bgColor = (contextCursor == i) ? WHITE : BLACK;
            
            if(contextCursor == i) ssd1306_fill_rectangle(0, 0, yPos - 2, 128, 14, WHITE);
            
            const unsigned char* currentIcon = (i == 0) ? iconSmall_skull : iconSmall_info;
            oled_draw_bitmap(0, 25, yPos, currentIcon, 10, 10, colorTheme); 
            
            const char* teksAction = (i == 0) ? "ATTACK" : "DETAILS";
            ssd1306_draw_string_adafruit(0, 45, yPos + 1, (char*)teksAction, colorTheme, bgColor);
        }

        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 53, 55, "[OK]", BLACK, WHITE); 
    }
    ssd1306_refresh(0, true);
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
        
        int bar = (millis() * 20) % 128; 
        ssd1306_draw_hline(0, 0, 45, bar, WHITE);
        
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
