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
#define MAX_STARS 15



// --- DATA UNTUK ANIMASI BINTANG ---
typedef struct {
    float x, y, z;
} Star;
Star stars[MAX_STARS];
bool starInit = false;

void initStars() {
    for (int i = 0; i < 15; i++) {
        stars[i].x = (rand() % 128) - 64;
        stars[i].y = (rand() % 64) - 32;
        stars[i].z = (rand() % 64) + 1;
    }
    starInit = true;
}


// Inisialisasi bintang pertama kali
extern void oled_draw_bitmap(uint8_t id, int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, ssd1306_color_t color);

uint32_t millis() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static float visualY = 24.0; // Variabel buat simpen posisi kursor sementara

void drawSmartSelection(int targetY) {
    // 0.3 itu kecepatan luncur, makin kecil makin lambat/smooth
    visualY += (targetY - visualY) * 0.3; 
    ssd1306_fill_rectangle(0, 0, (int)visualY, 128, 18, WHITE);
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
const unsigned char* iconListWiFi[] = {
iconSmall_scan,
iconSmall_sniff,
iconSmall_conn,
iconSmall_spam,
iconSmall_wifi
};

const unsigned char* iconListBLE[]  = {
iconSmall_scan,
iconSmall_apple,
iconSmall_android
};

const unsigned char* iconListIR[]   = {
iconSmall_ir,
iconSmall_tv,
iconSmall_ac,
iconSmall_lock,
iconSmall_saved 
};

const unsigned char* iconListSet[]  = {
iconSmall_bright,
iconSmall_wifi,
iconSmall_info,
iconSmall_repeat 
};

const unsigned char* iconListGame[]  = {
iconSmall_bright
};





const char* subMenuWiFi[] = { 
"Scan WiFi", 
"List Scan", 
"Connected WiFi", 
"Beacon Spam", 
"RickRoll SSID"
 };
 
const char* subMenuBLE[]  = {
"BLE Scanner",
"Spam Apple",
"Spam Android"
 };
 
const char* subMenuIR[]   = {
"Read Signal",
"TV B-Gone",
"AC Remote",
"Brute Force",
"Saved Remotes"
 };

const char* subMenuSet[]  = {
"Brightness",
"WiFi Setup",
"About RootX",
"Reboot" 
};

const char* subMenuGame[] = {
"Dinosaur Game"
 };

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
    else if(currentMenu == 3) ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: SETS", WHITE, BLACK);
    else                      ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: GAME", WHITE, BLACK);
    
    ssd1306_draw_hline(0, 0, 9, 128, WHITE);

    const unsigned char* bigIcon;
    if(currentMenu == 0)      bigIcon = logo_wifi_32; 
    else if(currentMenu == 1) bigIcon = logo_ble_32;
    else if(currentMenu == 2) bigIcon = logo_ir_32;
    else if(currentMenu == 3) bigIcon = logo_settings_32;
    else                      bigIcon = logo_game_32;
    

    int iconBounce = getBounce(300, 2); // Loncat 2 pixel
    oled_draw_bitmap(0, 47, 20 + iconBounce, bigIcon, 32, 32, WHITE);

    // Font library ini ukurannya fix, jadi kita akalin kursornya aja
    ssd1306_draw_string_adafruit(0, 20, 30, "<", WHITE, BLACK);
    ssd1306_draw_string_adafruit(0, 102, 30, ">", WHITE, BLACK);
    ssd1306_draw_string_adafruit(0, 40, 56, ">SELECT<", WHITE, BLACK); 
    
    ssd1306_refresh(0, true);
}

void tampilkanMenuUtama() { 
    ssd1306_clear(0);
    drawStarfield();
    drawWave();
    int totalSub = 0; 

    if(currentMenu == 0)      { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: WIFI", WHITE, BLACK); totalSub = 5; }
    else if(currentMenu == 1) { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: BLE ", WHITE, BLACK); totalSub = 3; }
    else if(currentMenu == 2) { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: IR", WHITE, BLACK);   totalSub = 5; }
    else if(currentMenu == 3)  { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: SETS", WHITE, BLACK); totalSub = 4; }
    else                      { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: GAME", WHITE, BLACK); totalSub = 1; }
    
    ssd1306_draw_hline(0, 0, 9, 128, WHITE);

        for(int i = 0; i < 5; i++) {
        int itemIndex = topMenu + i;
        if(itemIndex >= totalSub) break; 

        int yPos = 13 + (i * 10); 
        int textColor = WHITE;
        int bgColor = BLACK;
        int iconBounce = 0; // Default: Icon diem kaga gerak
        int tambahansu = 0;
        
        // --- LOGIKA MENU YANG DIPILIH (YANG ADA BLOK PUTIHNYA) ---
        if(itemIndex == currentSub) { 
            // 1. Gambar blok putih dasar
            ssd1306_fill_rectangle(0, 0, yPos - 1, 128, 10, WHITE);
            
            // 2. Animasi "Data Stream / Glitch" di ujung kanan blok
            // Bikin garis hitam jalan mundur dari X=125 ke X=105
            int slide = (millis() / 40) % 20; 
            int animX = 125 - slide;
            
            // Garis tipis ngalir
            ssd1306_fill_rectangle(0, animX, yPos - 1, 2, 10, BLACK); 
            // Kotak agak tebel ngikutin di belakangnya
            ssd1306_fill_rectangle(0, animX + 6, yPos - 1, 4, 10, BLACK); 
            
            // 3. Icon loncat cuma buat menu ini aja
            iconBounce = getBounce(200, 2); 
            tambahansu = 4;
            
            textColor = BLACK; 
            bgColor = WHITE;
        } 
        
        // Setting Icon
        const unsigned char* iconSmall;
        if(currentMenu == 0)      iconSmall = iconListWiFi[itemIndex]; 
        else if(currentMenu == 1) iconSmall = iconListBLE[itemIndex];
        else if(currentMenu == 2) iconSmall = iconListIR[itemIndex];
        else if(currentMenu == 3) iconSmall = iconListSet[itemIndex]; 
        else iconSmall = iconListGame[itemIndex];

        // Gambar Icon (Kalo diselect dia ada iconBounce-nya, kalo ngga ya + 0)
        oled_draw_bitmap(0, 2 + tambahansu, (yPos - 1) + iconBounce, iconSmall, 10, 10, textColor);
        
        // Setting Teks
        const char* textToPrint = "";
        if(currentMenu == 0)      textToPrint = subMenuWiFi[itemIndex];
        else if(currentMenu == 1) textToPrint = subMenuBLE[itemIndex];
        else if(currentMenu == 2) textToPrint = subMenuIR[itemIndex];
        else if(currentMenu == 3) textToPrint = subMenuSet[itemIndex];
        else textToPrint = subMenuGame[itemIndex];

        // Gambar Teks (Kalo diselect, text-nya ikutan goyang dikit biar asik)
        ssd1306_draw_string_adafruit(0, 18 + tambahansu, yPos, (char*)textToPrint, textColor, bgColor);
    }

    ssd1306_refresh(0, true);
}

// --- TARUH INI DI ATAS FUNGSI ---
extern int kbX;
extern int kbY;
extern int kbPage;
extern const char kbLayout[2][4][10];

// --- GANTI FUNGSI INI FULL ---
void tampilkanInputPassword() {
    ssd1306_clear(0);
    
    // 1. Gambar Kotak Teks di Atas
    ssd1306_draw_rectangle(0, 0, 0, 128, 14, WHITE);
    
    // Trik Biar Teks Gak Nabrak (Nampilin 18 karakter terakhir doang)
    int len = strlen(inputPassword);
    char* textToPrint = inputPassword;
    if (len > 18) textToPrint = inputPassword + (len - 18);
    ssd1306_draw_string_adafruit(0, 3, 3, textToPrint, WHITE, BLACK);
    
    // Kursor Teks (Kedip-kedip di sebelah huruf terakhir)
    int cursorTeks = 3 + (strlen(textToPrint) * 6);
    if ((millis() / 300) % 2 == 0) {
        ssd1306_fill_rectangle(0, cursorTeks, 4, 6, 8, WHITE);
    }

    // 2. Gambar Grid Keyboard QWERTY
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 10; c++) {
            char ch[2] = { kbLayout[kbPage][r][c], '\0' };
            
            // Hitung kordinat X dan Y per huruf
            int xPos = 4 + (c * 12); 
            int yPos = 18 + (r * 11);
            
            // Ubah simbol sistem jadi teks ikon biar paham
            const char* label = ch;
            if (ch[0] == '<') label = "<";      // Ikon Backspace
            else if (ch[0] == '*') label = "^"; // Ikon Shift/Page
            else if (ch[0] == '>') label = "OK"; // Ikon Enter/Connect

            // Kalo huruf ini lagi disorot (ditunjuk joystick)
            if (r == kbY && c == kbX) {
                // Kasih blok putih, hurufnya jadi hitam (Inverse)
                ssd1306_fill_rectangle(0, xPos - 1, yPos - 1, 11, 10, WHITE);
                ssd1306_draw_string_adafruit(0, xPos, yPos, (char*)label, BLACK, WHITE);
            } else {
                // Huruf biasa
                ssd1306_draw_string_adafruit(0, xPos, yPos, (char*)label, WHITE, BLACK);
            }
        }
    }
    
    ssd1306_refresh(0, true);
}

void tampilkanStatusKoneksi() {
    ssd1306_clear(0);
    drawWave(); // Kasih animasi wave biar keren

    // Header tetep blok putih
    ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
    ssd1306_draw_string_adafruit(0, 30, 1, "WIFI STATUS", BLACK, WHITE);

    if (statusKoneksi == 0) {
        // TAMPILAN LAGI PROSES
        int bounce = getBounce(200, 3);
        oled_draw_bitmap(0, 54, 20 + bounce, iconSmall_wifi, 10, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 30, 40, "CONNECTING...", WHITE, BLACK);
        drawLoadingBar(24, 52, 80, 6, (millis() / 20) % 100);
    } 
    else if (statusKoneksi == 1) {
        // TAMPILAN BERHASIL
        ssd1306_draw_string_adafruit(0, 35, 25, "CONNECTED!", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 20, 35, targetTerkunci.ssid, WHITE, BLACK);
        // Icon Ceklis (pake icon info aja kalo gada)
        oled_draw_bitmap(0, 59, 45, iconSmall_info, 10, 10, WHITE);
    } 
    else if (statusKoneksi == 2) {
        // TAMPILAN GAGAL
        ssd1306_draw_string_adafruit(0, 40, 25, "FAILED!", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 15, 35, "Wrong Password?", WHITE, BLACK);
        oled_draw_bitmap(0, 59, 45, iconSmall_skull, 10, 10, WHITE);
    } else if (statusKoneksi == 3) {
        // --- TAMPILAN DISCONNECTING ---
        ssd1306_draw_string_adafruit(0, 25, 25, "DISCONNECTING...", WHITE, BLACK);
        drawLoadingBar(24, 40, 80, 6, (millis() / 15) % 100);
    }
    else if (statusKoneksi == 4) {
        // --- TAMPILAN DISCONNECTED (BERHASIL CABUT) ---
        ssd1306_draw_string_adafruit(0, 22, 30, "DISCONNECTED!", WHITE, BLACK);
        oled_draw_bitmap(0, 59, 45, iconSmall_saved, 10, 10, WHITE); 
        // Pake icon saved atau centang biar keliatan sukses cabut
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
    ssd1306_draw_circle(0, 108, 18, r, WHITE);
    if(r > 5) ssd1306_draw_circle(0, 105, 15, r - 5, WHITE);

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
                    ssd1306_draw_string_adafruit(0, 16, yPos + 1, textShow, textColor, bgColor);

                    snprintf(buf, sizeof(buf), "C:%d", listWiFi[itemIdx].channel);
                    ssd1306_draw_string_adafruit(0, 66, yPos + 1, buf, textColor, bgColor);
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
                else if (scannerState == 4) { // Atau scannerStateSta == 4, sesuaikan aja
        // --- 1. HEADER ---
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 42, 1, "ACTIONS", BLACK, WHITE);

        // --- 2. BLOK PUTIH STATIS DI TENGAH ---
        // Y mulai dari 24, tinggi 16 piksel (Bener-bener pas di center OLED 128x64)
        ssd1306_fill_rectangle(0, 0, 24, 128, 16, WHITE);

        // --- 3. LOGIKA ROLLING MENU ---
        for(int i = 0; i < 5; i++) {
            const char* teks;
            const unsigned char* icon;
            
            // Set Teks dan Icon
            if(i == 0)      { teks = "DEAUTH "; icon = iconSmall_skull; }
            else if(i == 1) { teks = "CONNECT"; icon = iconSmall_conn; }
            else if(i == 2) { teks = "CLIENTS"; icon = iconSmall_sniff; }
            else if(i == 3) { teks = "TRACK  "; icon = iconSmall_wifi;  } 
            else            { teks = "DETAILS"; icon = iconSmall_info;  }

            // Hitung jarak index ini dari kursor yang lagi aktif
            int diff = i - contextCursor; 
            
            // Jarak antar baris 15 piksel. Posisi tengah (diff=0) ada di Y=27
            int yPos = 27 + (diff * 15); 

            // Cuma gambar menu yang posisinya ada di area pandang (antara header & footer)
            if (yPos > 10 && yPos < 45) {
                
                if (diff == 0) { 
                    // --- MENU TERPILIH (DI TENGAH BLOK PUTIH) ---
                    // Warna dibalik (Hitam di atas Putih)
                    oled_draw_bitmap(0, 26, yPos - 1, icon, 10, 10, BLACK); 
                    ssd1306_draw_string_adafruit(0, 42, yPos, (char*)teks, BLACK, WHITE);
                    
                    // Tambahan efek panah biar kelihatan lebih "Gede/Lebar"
                    ssd1306_draw_string_adafruit(0, 10, yPos, ">", BLACK, WHITE);
                    ssd1306_draw_string_adafruit(0, 110, yPos, "<", BLACK, WHITE);
                } 
                else { 
                    // --- MENU GAK TERPILIH (DI ATAS / DI BAWAH) ---
                    // Warna normal (Putih di atas Hitam)
                    // Posisinya digeser X-nya (+4) biar seakan-akan mundur/mengecil
                    oled_draw_bitmap(0, 30, yPos, icon, 10, 10, WHITE);
                    ssd1306_draw_string_adafruit(0, 46, yPos + 1, (char*)teks, WHITE, BLACK);
                }
            }
        }

        // --- 4. FOOTER ---
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 85, 55, "[OK] GO", BLACK, WHITE);
    }



    ssd1306_refresh(0, true);
}


void tampilkanConnectedWiFi() {
    ssd1306_clear(0);
    
    // HEADER BLOK
    ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
    ssd1306_draw_string_adafruit(0, 22, 1, "CONNECTED INFO", BLACK, WHITE);

    if (!isWiFiConnected) {
        ssd1306_draw_string_adafruit(0, 15, 30, "BELUM KONEK WIFI", WHITE, BLACK);
        
        // FOOTER BACK
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 5, 55, "< BACK", BLACK, WHITE);
    } else {
        // Tampilkan Detail
        char buf[64];
        snprintf(buf, sizeof(buf), "SSID: %s", connSSID);
        ssd1306_draw_string_adafruit(0, 5, 15, buf, WHITE, BLACK);
        
        snprintf(buf, sizeof(buf), "CH  : %d", connCH);
        ssd1306_draw_string_adafruit(0, 5, 25, buf, WHITE, BLACK);
        
        snprintf(buf, sizeof(buf), "RSSI: %d dBm", connRSSI);
        ssd1306_draw_string_adafruit(0, 5, 35, buf, WHITE, BLACK);

        // FOOTER BACK & DISC
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 5, 55, "< BACK", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 95, 55, "DISC >", BLACK, WHITE);
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
                   
                  
                    ssd1306_draw_string_adafruit(0, 18, yPos + 1, buf, txtCol, bgCol);

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

        snprintf(buf, sizeof(buf), "MC:%02X:%02X:%02X:%02X:%02X:%02X", 
                 targetSta.mac[0], targetSta.mac[1], targetSta.mac[2],
                 targetSta.mac[3], targetSta.mac[4], targetSta.mac[5]);
                 
        
        ssd1306_draw_string_adafruit(0, 5, 17, buf, WHITE, BLACK);
        

        snprintf(buf, sizeof(buf), "RSSI: %d dBm", targetSta.rssi);
        ssd1306_draw_string_adafruit(0, 5, 25, buf, WHITE, BLACK);

        snprintf(buf, sizeof(buf), "PACKETS: %d", targetSta.paket_count);
        ssd1306_draw_string_adafruit(0, 5, 35, buf, WHITE, BLACK);

        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
    } 
    
        else if (scannerStateSta == 4) { // Atau scannerStateSta == 4, sesuaikan aja
        // --- 1. HEADER ---
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 42, 1, "ACTIONS", BLACK, WHITE);

        // --- 2. BLOK PUTIH STATIS DI TENGAH ---
        // Y mulai dari 24, tinggi 16 piksel (Bener-bener pas di center OLED 128x64)
        ssd1306_fill_rectangle(0, 0, 24, 128, 16, WHITE);

        // --- 3. LOGIKA ROLLING MENU ---
        for(int i = 0; i < 4; i++) {
            const char* teks;
            const unsigned char* icon;
            
            // Set Teks dan Icon
              if(i == 0)      { teks = "KICK CLIENT";  icon = iconSmall_skull; }
            else            { teks = "DETAILS"; icon = iconSmall_info;  }

            // Hitung jarak index ini dari kursor yang lagi aktif
            int diff = i - contextCursor; 
            
            // Jarak antar baris 15 piksel. Posisi tengah (diff=0) ada di Y=27
            int yPos = 27 + (diff * 15); 

            // Cuma gambar menu yang posisinya ada di area pandang (antara header & footer)
            if (yPos > 10 && yPos < 45) {
                
                if (diff == 0) { 
                    // --- MENU TERPILIH (DI TENGAH BLOK PUTIH) ---
                    // Warna dibalik (Hitam di atas Putih)
                    oled_draw_bitmap(0, 26, yPos - 1, icon, 10, 10, BLACK); 
                    ssd1306_draw_string_adafruit(0, 42, yPos, (char*)teks, BLACK, WHITE);
                    
                    // Tambahan efek panah biar kelihatan lebih "Gede/Lebar"
                    ssd1306_draw_string_adafruit(0, 10, yPos, ">", BLACK, WHITE);
                    ssd1306_draw_string_adafruit(0, 110, yPos, "<", BLACK, WHITE);
                } 
                else { 
                    // --- MENU GAK TERPILIH (DI ATAS / DI BAWAH) ---
                    // Warna normal (Putih di atas Hitam)
                    // Posisinya digeser X-nya (+4) biar seakan-akan mundur/mengecil
                    oled_draw_bitmap(0, 30, yPos, icon, 10, 10, WHITE);
                    ssd1306_draw_string_adafruit(0, 46, yPos + 1, (char*)teks, WHITE, BLACK);
                }
            }
        }

        // --- 4. FOOTER ---
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< BACK", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 85, 55, "[OK] GO", BLACK, WHITE);
    }
    
   
        // --- 4. FOOTER (Tetap) ---

    ssd1306_refresh(0, true);
}





void tampilkandeauthsta() {
    ssd1306_clear(0);
    char buf[64];
    
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 1, "ATTACKING STATION...", BLACK, WHITE);
        
        snprintf(buf, sizeof(buf), "Target:%02X:%02X:%02X:%02X:%02X:%02X", 
                 targetSta.mac[0], targetSta.mac[1], targetSta.mac[2],
                 targetSta.mac[3], targetSta.mac[4], targetSta.mac[5]);

        ssd1306_draw_string_adafruit(0, 0, 20, buf, WHITE, BLACK);
        snprintf(buf, sizeof(buf), "Ch: %d", targetTerkunci.channel);
        ssd1306_draw_string_adafruit(0, 0, 30, buf, WHITE, BLACK);
        
        int animasiProgress = (millis() / 30) % 100; 

        drawLoadingBar(14, 42, 100, 8, animasiProgress);
        
        
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< STOP ATTACK", BLACK, WHITE);
    
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
        
        
        int animasiProgress = (millis() / 30) % 100; 

        drawLoadingBar(14, 42, 100, 8, animasiProgress);
        
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
    // Kita pake fungsi manual dari i2c.c lu
    i2c_start();
    
    // 0x3C << 1 jadi 0x78 (Alamat OLED)
    i2c_write(0x78); 
    
    // 0x00 artinya byte berikutnya adalah COMMAND
    i2c_write(0x00); 
    
    // Perintah 0x81 (Contrast Control)
    i2c_write(0x81); 
    
    // Kirim nilai kecerahan (0-255)
    i2c_write(level); 
    
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
        
        
        int animasiProgress = (millis() / 30) % 100; 
        drawLoadingBar(14, 42, 100, 8, animasiProgress);
        
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< STOP", BLACK, WHITE);
    }
    ssd1306_refresh(0, true);
}





void renderDinoGame() {
    if (dinoHighScore == -1) dinoHighScore = baca_highscore_dino(); // Load pertama kali

    ssd1306_clear(0);

    // --- RENDER FOOTER HILANG, GANTI UI ATAS ---
    ssd1306_draw_string_adafruit(0, 0, 0, "< BACK", WHITE, BLACK);
    
    // Kedip Skor tiap kelipatan 100
    bool blinkScore = ((dinoScore % 100) < 10 && dinoScore > 0 && dinoState == 0);
    if (!blinkScore) {
        char scoreBuf[32];
        snprintf(scoreBuf, sizeof(scoreBuf), "HI:%05d %05d", dinoHighScore, dinoScore);
        ssd1306_draw_string_adafruit(0, 45, 0, scoreBuf, WHITE, BLACK);
    }

    // --- STATE 0: MAIN ---
    if (dinoState == 0) {
        // Sistem Siang Malam (Tiap 500 skor)
        bool isNight = ((dinoScore / 500) % 2 == 1);
        ssd1306_invert_display(0, isNight);

        // Fisika & Kecepatan
        rawScore += (gameSpeed * 0.1); 
        dinoScore = (int)rawScore;
        gameSpeed = 3.0 + (dinoScore / 200.0);
        if (gameSpeed > 8.0) gameSpeed = 8.0; // Batas max biar mata gak picek

        // Langit & Bintang
        skyX -= 1;
        if (skyX < -20) skyX = 128;
        if (isNight) {
            oled_draw_bitmap(0, skyX, 5, bulan_16, 16, 16, WHITE);
            for(int i=0; i<5; i++) {
                starX[i] -= 1;
                if(starX[i] < 0) starX[i] = 128;
                ssd1306_draw_pixel(0, starX[i], starY[i], WHITE);
            }
        } else {
            oled_draw_bitmap(0, skyX, 5, matahari_16, 16, 16, WHITE);
        }

        // Fisika Lompat
        dinoY += dinoVy;
        if (isJumping) dinoVy += 1.2; // Gravitasi
        if (dinoY >= 32) { // Lantai
            dinoY = 32;
            isJumping = false;
            dinoVy = 0;
        }

        // Musuh Jalan
        obstacleX -= (int)gameSpeed; 
        if (obstacleX < -20) {
            obstacleX = 128 + (rand() % 60); 
            obstacleType = rand() % 3; // Acak musuh
            
            if (obstacleType == 2) { 
                // Kalo Burung, acak tinggi (bawah, tengah, atas)
                int h[3] = {32, 20, 10}; 
                obstacleY = h[rand() % 3];
            } else {
                obstacleY = 40; // Kaktus di lantai
            }
        }

        // --- RENDER GROUND (TANAH) ---
        ssd1306_draw_hline(0, 0, 56, 128, WHITE); // Garis solid
        // Tambahan detail tanah putus-putus
        for(int p=0; p<128; p+=10) {
            ssd1306_draw_hline(0, p - ((int)(rawScore*10)%10), 58 + (rand()%3), rand()%5, WHITE);
        }

        // Gambar Musuh
        if (obstacleType == 0) oled_draw_bitmap(0, obstacleX, obstacleY, kaktus_16, 16, 16, WHITE);
        else if (obstacleType == 1) oled_draw_bitmap(0, obstacleX, obstacleY, kaktus_banyak, 16, 16, WHITE);
        else oled_draw_bitmap(0, obstacleX, obstacleY, ptero_16, 16, 16, WHITE);

        // Gambar Dino 24x24
        oled_draw_bitmap(0, 10, dinoY, dino_24, 24, 24, WHITE);

        // --- HITBOX PRESISI ---
        int dinoRight = 10 + 16; 
        int dinoBottom = dinoY + 20;
        int obsRight = obstacleX + 12;
        int obsBottom = obstacleY + 14;

        if (obstacleX < dinoRight && obsRight > 15 && dinoY < obsBottom && dinoBottom > obstacleY) {
            dinoState = 1; // MODAR
            ssd1306_invert_display(0, false); // Normalin layar pas mati
            if (dinoScore > dinoHighScore) {
                dinoHighScore = dinoScore;
                simpan_highscore_dino(dinoHighScore);
            }
        }

        if (dinoScore >= dinoLimit) { // BATAS SKOR KETEMU CEWEK
            dinoState = 2; 
            endTimer = 0;
            ssd1306_invert_display(0, false);
        }

    } 
    // --- STATE 1: MATI (UI BERSIH) ---
    else if (dinoState == 1) {
        // Layar udah di-clear di atas, jadi teks gak numpuk
        ssd1306_draw_string_adafruit(0, 30, 15, "GAME OVER", WHITE, BLACK);
        char finalSc[32];
        snprintf(finalSc, sizeof(finalSc), "Score: %d", dinoScore);
        ssd1306_draw_string_adafruit(0, 35, 30, finalSc, WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 20, 45, "[OK] RESTART", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 25, 55, "[<] KELUAR", WHITE, BLACK);
    } 
    // --- STATE 2: ENDING CINEMATIC ---
    else if (dinoState == 2) {
        endTimer++;
        int walkX = 10;
        if (endTimer < 50) walkX = 10 + (endTimer * 20 / 50);
        else walkX = 30;

        if (endTimer > 30) oled_draw_bitmap(0, 80, 32, dino_24, 24, 24, WHITE); // Ceweknya

        oled_draw_bitmap(0, walkX, 32, dino_24, 24, 24, WHITE);

        if (endTimer > 60 && endTimer < 110) oled_draw_bitmap(0, 55, 15, heart_16, 16, 16, WHITE);
        else if (endTimer >= 110) {
            oled_draw_bitmap(0, 55, 15, broken_16, 16, 16, WHITE);
            ssd1306_draw_string_adafruit(0, 25, 0, "KITA TEMENAN", WHITE, BLACK);
            ssd1306_draw_string_adafruit(0, 35, 10, "AJA YAA..", WHITE, BLACK);
        }

        if (endTimer > 160) {
            ssd1306_draw_string_adafruit(0, 25, 55, "[OK] MOVE ON", WHITE, BLACK);
        }
    }

    ssd1306_refresh(0, true);
}
