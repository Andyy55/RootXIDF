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
#include "driver/gpio.h"
#include "esp_log.h"


#define WHITE 1
#define BLACK 0
#define MAX_STARS 15

extern void handleJoystick(void);
extern void tampilkanLogoDulu(void);
extern void tampilkanIntroAnime(void);
extern void tampilkanTeksSplash(void);
void tampilkanMenuLogo(void);
void tampilkanMenuUtama(void);

void tampilkanMenuLogo(void);
void tampilkanMenuUtama(void);
void tampilkanWifiScanner(void);
void tampilkanDeauthScreen(void);
void tampilkanBrightness(void);
void tampilkanSpamScreen(const char* judul, const char* subTeks);

void tampilkanStationScanner(void);
void tampilkanTrackScreen(void);
void tampilkandeauthsta(void);
void tampilkanEvilTwinScreen(void);
void renderDinoGame(void);
void tampilkanMenuIR(void);
void tampilkanMenuSavedIR(void);

bool introDone = false; // Penanda intro sudah lewat

void init_joystick() {
    int pins[] = {PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT, PIN_OK};
    for(int i = 0; i < 5; i++) {
        gpio_set_direction(pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(pins[i], GPIO_PULLUP_ONLY);
    }
}

void task_display(void *pvParameters) {
init_joystick();
if (ssd1306_init(0, 9, 8)) {
    vTaskDelay(pdMS_TO_TICKS(100)); // <--- KUNCI SAKTI: Kasih napas 100ms
    ssd1306_select_font(0, 0);
    ssd1306_clear(0);
    ssd1306_refresh(0, true);
    ESP_LOGI("RootX", "OLED Ready!");
} else {
    ESP_LOGE("RootX", "OLED Gagal Inisialisasi!");
}

    tampilkanLogoDulu();
    tampilkanIntroAnime();
    tampilkanTeksSplash();
    introDone = true;
    for(;;) {
        // Cek mode aplikasi dan tampilkan layar yang pas
handleJoystick(); // Cek input

        if (appMode == 0) {
            if (inSubMenu == false) tampilkanMenuLogo();
            else tampilkanMenuUtama();
        } 
        else if (appMode == 1) {
            tampilkanWifiScanner(); 
        } 
        else if (appMode == 2) {
            tampilkanDeauthScreen(); 
        } 
        else if (appMode == 3) { 
            tampilkanBrightness();
        } 
        else if (appMode == 4) {
            if (aktifModeSpam == 1) tampilkanSpamScreen("BEACON SPAM", "Start Spam?");
            else if (aktifModeSpam == 2) tampilkanSpamScreen("RICKROLL", "Start Spam?");
        } else if (appMode == 5) { 
            tampilkanStationScanner();
        } else if (appMode == 6) { // Kita kasih mode 6 buat Track
    tampilkanTrackScreen();
} else if (appMode == 7) {
tampilkandeauthsta();
} else if (appMode == 8) {
tampilkanEvilTwinScreen();
} else if (appMode == 11) {
renderDinoGame();
}else if (appMode == MODE_IR_SNIFFER) {    // <--- TAMBAHIN INI
            tampilkanMenuIR();
        } else if (appMode == MODE_SAVED_REMOTE) {  // <--- TAMBAHIN INI
            tampilkanMenuSavedIR();
        }


        // Kasih jeda dikit biar gak rakus CPU (kira-kira 30 FPS)
        vTaskDelay(pdMS_TO_TICKS(33)); 
    }
}

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
    
    read_battery_percentage(); // Baca tiap kali refresh layar
    char batBuf[10];
    snprintf(batBuf, sizeof(batBuf), "%d%%", batteryPercent);
    
    // Tampilkan Persentase di pojok kanan (X=100, Y=0)
    ssd1306_draw_string_adafruit(0, 95, 0, batBuf, WHITE, BLACK);

    // --- DRAW ICON BATERAI (10x10) ---
    // Kotak luar baterai
    ssd1306_draw_rectangle(0, 116, 0, 10, 6, WHITE); 
    ssd1306_draw_pixel(0, 126, 2, WHITE); // Kepala baterai
    
    // Isi baterai berdasarkan persen
    int barWidth = batteryPercent / 12; // Skala 100% ke 8 pixel
    if (barWidth > 8) barWidth = 8;
    ssd1306_fill_rectangle(0, 117, 1, barWidth, 4, WHITE);
    
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

    if(currentMenu == 0)      { ssd1306_draw_string_adafruit(0, 0, 0, "#> RootX: WIFI", WHITE, BLACK); totalSub = 4; }
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
            else if(i == 1) { teks = "EVIL TWIN"; icon = iconSmall_conn; }
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
        for(int i = 0; i < 2; i++) {
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
    if (dinoHighScore == -1) dinoHighScore = baca_highscore_dino();
    ssd1306_clear(0);

    // ==========================================
    // VARIABEL SUTRADARA (STATE MACHINE ENDING)
    // ==========================================
    static int endingState = 0;   
    /* 0: Main Biasa
       1: Skor 5000 (Melambat, musuh ilang)
       2: Berhenti & Zoom Wajah + Bunga
       3: Flashback 2 Dino + Love
       4: Zoom Wajah + Nangis 3 Tetes
       5: Zoom Out, Bunga Jatuh
       6: Balik Badan & Bunuh Diri
       7: Terjatuh (Tidur) -> Game Over
    */
    static int endingTimer = 0;
    static float tearY = 22.0;
    static int tearCount = 0;
    static int dinoActionX = 15;
    static float flowerY = 25.0;
    static int flowerX = 25;

    // --- LOGIC SIANG MALAM ---
    int cycle = dinoScore % 1000;
    bool isNight = (cycle >= 0 && cycle < 300 && dinoScore >= 1000); 
    ssd1306_invert_display(0, isNight);

    // --- UI: SKOR ---
    char scoreBuf[32];
    snprintf(scoreBuf, sizeof(scoreBuf), "HI %05d  %05d", dinoHighScore, dinoScore);
    ssd1306_draw_string_adafruit(0, 35, 0, scoreBuf, WHITE, BLACK);

    static int obs2X = 300; 
    static int obs2Y = 44;
    static int obs2Type = 0;

    if (dinoState == 0) { // SEDANG MAIN (ATAU SEDANG ANIMASI ENDING)
        
        // --- TRIGGER ENDING DRAMA ---
        if (dinoScore >= 200 && endingState == 0) {
            endingState = 1; // Mulai pelan
        }

        // ==========================================
        // LOGIKA PERGERAKAN BERDASARKAN STATE
        // ==========================================
        if (endingState == 0) {
            // MAIN NORMAL
            rawScore += (gameSpeed * 0.15);
            dinoScore = (int)rawScore;
            gameSpeed = 4.0 + (dinoScore / 500.0);
            if (gameSpeed > 8.5) gameSpeed = 8.5;
        } 
        else if (endingState == 1) {
            // 1. SLOWDOWN & BERSIHIN LAYAR
            gameSpeed -= 0.05; // Makin pelan
            if (gameSpeed <= 0) {
                gameSpeed = 0;
                // Pastikan obstacle udah bener-bener hilang dari layar (X < -24)
                if (obstacleX < -24 && obs2X < -24) {
                    endingTimer++;
                    if (endingTimer > 30) { // Jeda bentar sebelum zoom
                        endingState = 2; 
                        endingTimer = 0;
                    }
                }
            }
        }

        // --- FISIKA LOMPAT DINO ---
        dinoY += dinoVy;
        if (isJumping) dinoVy += 1.6; 
        if (dinoY >= 36) { dinoY = 36; isJumping = false; dinoVy = 0; }

        // --- BACKGROUND ---
        skyX -= (gameSpeed * 0.1);
        if (skyX < -20) skyX = 128;
        if (isNight) oled_draw_bitmap(0, (int)skyX, 8, bulan_16, 16, 16, WHITE);
        else oled_draw_bitmap(0, (int)skyX, 8, matahari_16, 16, 16, WHITE);

             // --- SISTEM MUSUH 1 (Logika Pergerakan) ---
        obstacleX -= (int)gameSpeed;
        if (obstacleX < -24) {
            // Musuh 1 muncul dari kanan (128)
            // Tapi kita kasih jarak random dari posisi Musuh 2
            int jarakAman = obs2X + 50 + (rand() % 90); 
            // Syarat mutlak: Gak boleh kurang dari 128 biar gak muncul di tengah!
            obstacleX = (jarakAman > 128) ? jarakAman : 128 + (rand() % 40);
            
            obstacleType = rand() % 3; 
            if (obstacleType == 2) { int h[] = {15, 32}; obstacleY = h[rand()%2]; } 
            else { obstacleY = (obstacleType == 0) ? 44 : 38; }
        }

        // --- SISTEM MUSUH 2 (Logika Pergerakan) ---
        obs2X -= (int)gameSpeed;
        if (obs2X < -24) {
            // Musuh 2 juga sama, harus nunggu di belakang Musuh 1
            int jarakAman2 = obstacleX + 50 + (rand() % 90);
            // Tetep harus muncul minimal di koordinat 128 (ujung kanan)
            obs2X = (jarakAman2 > 128) ? jarakAman2 : 128 + (rand() % 40);
            
            obs2Type = rand() % 3; 
            if (obs2Type == 2) { int h[] = {15, 32}; obs2Y = h[rand()%2]; } 
            else { obs2Y = (obs2Type == 0) ? 44 : 38; }
        }


        // --- DRAW GROUND ---
        ssd1306_draw_hline(0, 0, 60, 128, WHITE);
        if (gameSpeed > 0) {
            for (int i = 0; i < 128; i += 16) {
                int scrollX = (i - ((int)rawScore % 128));
                if (scrollX < 0) scrollX += 128;
                ssd1306_draw_pixel(0, scrollX, 62, WHITE);
                ssd1306_draw_pixel(0, (scrollX + 7) % 128, 61, WHITE);
                if (i % 32 == 0) ssd1306_draw_hline(0, scrollX, 61, 4, WHITE);
            }
        }


        // ==========================================
        // SUTRADARA: RENDER SCENE (THE MOVIE)
        // ==========================================
        
        if (endingState == 0 || endingState == 1) {
            // SCENE: GAMEPLAY NORMAL
            const unsigned char* pteroFrame = ((millis() / 200) % 2 == 0) ? ptero_up : ptero_down;
            if (obstacleType == 0) oled_draw_bitmap(0, (int)obstacleX, 44, kaktus_16, 16, 16, WHITE);
            else if (obstacleType == 1) oled_draw_bitmap(0, (int)obstacleX, 38, kaktus_besar, 24, 24, WHITE);
            else if (obstacleType == 2) oled_draw_bitmap(0, (int)obstacleX, obstacleY, pteroFrame, 16, 16, WHITE);
            
            if (obs2Type == 0) oled_draw_bitmap(0, (int)obs2X, 44, kaktus_16, 16, 16, WHITE);
            else if (obs2Type == 1) oled_draw_bitmap(0, (int)obs2X, 38, kaktus_besar, 24, 24, WHITE);
            else if (obs2Type == 2) oled_draw_bitmap(0, (int)obs2X, obs2Y, pteroFrame, 16, 16, WHITE);

            const unsigned char* dinoFrame = (dinoY < 36) ? dino_lari1 : (((millis()/100)%2==0) ? dino_lari1 : dino_lari2);
            if (gameSpeed == 0) dinoFrame = dino_lari1; // Kalo berhenti, frame diam
            oled_draw_bitmap(0, 15, (int)dinoY, dinoFrame, 24, 24, WHITE);
        }
        
        else if (endingState == 2) {
            // SCENE: ZOOM WAJAH + BUNGA
            ssd1306_draw_rectangle(0, 40, 10, 40, 40, WHITE); // Muka Zoom
            ssd1306_draw_pixel(0, 70, 20, WHITE); // Mata
            ssd1306_draw_string_adafruit(0, 50, 35, "*", WHITE, BLACK); // Bunga

            endingTimer++;
            if (endingTimer > 80) { endingState = 3; endingTimer = 0; }
        }

        else if (endingState == 3) {
            // SCENE: FLASHBACK 2 DINO + LOVE
            oled_draw_bitmap(0, 30, 36, dino_lari1, 24, 24, WHITE); // Dino 1 (Kiri)
            oled_draw_bitmap(0, 70, 36, dino_lari1, 24, 24, WHITE); // Dino 2 (Kanan)
            ssd1306_draw_string_adafruit(0, 58, 30, "<3", WHITE, BLACK); // Love

            endingTimer++;
            if (endingTimer > 100) { endingState = 4; endingTimer = 0; tearY = 22.0; tearCount = 0; }
        }

        else if (endingState == 4) {
            // SCENE: ZOOM WAJAH + NANGIS 3 TETES PELAN
            ssd1306_draw_rectangle(0, 40, 10, 40, 40, WHITE); 
            ssd1306_draw_pixel(0, 70, 20, WHITE); // Mata
            
            // Logika Netes Pelan
            tearY += 0.4; // Kecepatan air mata (makin kecil makin pelan)
            if (tearY > 40.0) { // Kalo udah nyampe dagu
                tearY = 22.0; // Balik ke mata
                tearCount++;
            }
            ssd1306_draw_pixel(0, 70, (int)tearY, WHITE); // Gambar air mata
            
            if (tearCount >= 3) { 
                endingState = 5; 
                endingTimer = 0;
                // Setup buat scene bunuh diri
                obstacleX = 100; // Obstacle muncul di depan/belakang buat ditabrak
                obstacleType = 1; // Kaktus Gede biar dramatis
                dinoActionX = 15;
                flowerX = 25;
                flowerY = 40.0; // Bunga di tangan
            }
        }

        else if (endingState >= 5) {
            // SCENE: ZOOM OUT, JATUHIN BUNGA, BUNUH DIRI
            
            // Gambar Obstacle Kematian
            oled_draw_bitmap(0, (int)obstacleX, 38, kaktus_besar, 24, 24, WHITE);
            
            // Bunga dijatuhin ke tanah
            if (endingState == 5) {
                if (flowerY < 55.0) flowerY += 1.0; // Bunga jatuh
                else {
                    endingTimer++;
                    if (endingTimer > 30) endingState = 6; // Jeda bentar terus lari
                }
            }
            ssd1306_draw_string_adafruit(0, flowerX, (int)flowerY, "*", WHITE, BLACK); // Bunga

            if (endingState == 6) {
                // Lari tubrukin diri
                dinoActionX += 3; // Kecepatan lari nyamperin maut
                if (dinoActionX + 20 >= obstacleX) {
                    // TABRAKAN!
                    endingState = 7;
                    endingTimer = 0;
                }
            }

            // Gambar Dino (Kalo state 7, dinonya agak turun kayak tidur/terjatuh)
            int renderY = (endingState == 7) ? 46 : 36;
            oled_draw_bitmap(0, dinoActionX, renderY, dino_lari1, 24, 24, WHITE);

            if (endingState == 7) {
                endingTimer++;
                if (endingTimer > 50) { // Jeda bentar pas tiduran, terus GAME OVER
                    dinoState = 1; 
                    ssd1306_invert_display(0, false);
                    
                    // --- SAVE HIGHSCORE ---
                    if (dinoScore > dinoHighScore) {
                        dinoHighScore = dinoScore;
                        simpan_highscore_dino(dinoHighScore); 
                    }
                }
            }
        }

        // ==========================================
        // COLLISION DETECTION (HANYA BUAT GAMEPLAY NORMAL)
        // ==========================================
        if (endingState == 0) {
            // Cek Musuh 1
            if (obstacleX > 12 && obstacleX < 30) { 
                int d_top = dinoY + 4; int d_bottom = dinoY + 20;
                int o_top = (obstacleType == 2) ? obstacleY + 4 : ((obstacleType == 0) ? 44 : 38);
                int o_bottom = (obstacleType == 2) ? obstacleY + 12 : 60;
                if (!(d_bottom < o_top || d_top > o_bottom)) { 
                    dinoState = 1; ssd1306_invert_display(0, false); 
                    if (dinoScore > dinoHighScore) { dinoHighScore = dinoScore; simpan_highscore_dino(dinoHighScore); }
                }
            }
            // Cek Musuh 2
            if (obs2X > 12 && obs2X < 30) { 
                int d_top = dinoY + 4; int d_bottom = dinoY + 20;
                int o_top = (obs2Type == 2) ? obs2Y + 4 : ((obs2Type == 0) ? 44 : 38);
                int o_bottom = (obs2Type == 2) ? obs2Y + 12 : 60;
                if (!(d_bottom < o_top || d_top > o_bottom)) { 
                    dinoState = 1; ssd1306_invert_display(0, false); 
                    if (dinoScore > dinoHighScore) { dinoHighScore = dinoScore; simpan_highscore_dino(dinoHighScore); }
                }
            }
        }

    } 
    else { 
        // ==========================================
        // GAME OVER SCREEN
        // ==========================================
        ssd1306_draw_string_adafruit(0, 20, 25, "G A M E  O V E R", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 28, 50, "[OK] RESTART", WHITE, BLACK);
        
        // RESET SUTRADARA BUAT GAME BARU

                // RESET SUTRADARA BUAT GAME BARU
        endingState = 0;
        endingTimer = 0;
        tearCount = 0;
        flowerY = 25.0;
        dinoActionX = 15;
        obstacleX = 128; // Musuh 1 di ujung
        obs2X = 250;     // Musuh 2 nunggu jauh di luar layar
        
    }
    
    ssd1306_refresh(0, true);
}



void tampilkanEvilTwinScreen() {
    ssd1306_clear(0);
    drawStarfield();
    
    if (evilTwinState == 0) {
    ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 1, "EVIL TWIN", BLACK, WHITE);
        
        ssd1306_draw_string_adafruit(0, 10, 25, "Start Evil Twin?", WHITE, BLACK);

     
        
        ssd1306_draw_string_adafruit(0, 10, 25, targetTerkunci.ssid, WHITE, BLACK);
        ssd1306_fill_rectangle(0, 0, 54, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 2, 55, "< NO", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 95, 55, "YES >", BLACK, WHITE);
    } 
    else if (evilTwinState == 1) {
        ssd1306_draw_string_adafruit(0, 15, 20, "WAITING FOR DATA...", WHITE, BLACK);
        int bounce = (millis() / 200) % 5;
        ssd1306_draw_string_adafruit(0, 50, 40 + bounce, "...", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 2, 55, "< NO", WHITE, BLACK);
    }
    else if (evilTwinState == 2) {
        ssd1306_fill_rectangle(0, 0, 0, 128, 10, WHITE);
        ssd1306_draw_string_adafruit(0, 20, 1, "PW EXPLOITED!", BLACK, WHITE);
        ssd1306_draw_string_adafruit(0, 5, 25, "Target:", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 50, 25, targetTerkunci.ssid, WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 5, 40, "Pass  :", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 50, 40, stolenPassword, WHITE, BLACK);
    }
    ssd1306_refresh(0, true);
}


ir_saved_state_t currentIRSavedState = IR_SAVED_STATE_LIST;
SavedRemote_t listSavedRemotes[20];
int totalSavedRemotes = 0;
int savedRemoteIndex = 0;
int actionMenuIndex = 0; // 0 = Transmit, 1 = Hapus

// Panggil fungsi ini pas PERTAMA KALI masuk menu Saved Remote
// Fungsi Parse Data Mentah dari SD Card
void loadSavedRemotes() {
    totalSavedRemotes = 0;
    FILE* f = fopen("/sdcard/ir_log.txt", "r");
    if (!f) return; 

    char line[1500]; // Buffer gede buat baca array
    while (fgets(line, sizeof(line), f) && totalSavedRemotes < 20) {
        char* token = strtok(line, "|");
        if (!token) continue;
        strcpy(listSavedRemotes[totalSavedRemotes].nama, token);
        
        token = strtok(NULL, "|");
        if (!token) continue;
        listSavedRemotes[totalSavedRemotes].num_pulses = atoi(token);
        
        token = strtok(NULL, "|");
        char* p_token = strtok(token, ",");
        int idx = 0;
        while (p_token != NULL && idx < 200) {
            listSavedRemotes[totalSavedRemotes].pulses[idx] = atoi(p_token);
            p_token = strtok(NULL, ",");
            idx++;
        }
        totalSavedRemotes++;
    }
    fclose(f);
}

// --- Potongan buat nampilin layar Hasil ---


void tampilkanMenuSavedIR() {
    ssd1306_clear(0); // Bersihin layar (ID 0)

    if (currentIRSavedState == IR_SAVED_STATE_LIST) {
        // --- HEADER (BLOK PUTIH) ---
        // Format library lu: (ID, X, Y, Teks, Warna Teks, Warna Background)
        ssd1306_draw_string_adafruit(0, 0, 0, " == SAVED REMOTES ==", BLACK, WHITE);

        if (totalSavedRemotes == 0) {
            ssd1306_draw_string_adafruit(0, 10, 25, "Data Kosong!", WHITE, BLACK);
        } else {
            // Tampilkan max 3 item biar rapi (Paging logic)
            int startIdx = (savedRemoteIndex / 3) * 3;
            for (int i = 0; i < 3; i++) {
                int curr = startIdx + i;
                if (curr >= totalSavedRemotes) break;

                char buf[32];
                if (curr == savedRemoteIndex) {
                    snprintf(buf, sizeof(buf), "> %s", listSavedRemotes[curr].nama);
                } else {
                    snprintf(buf, sizeof(buf), "  %s", listSavedRemotes[curr].nama);
                }
                ssd1306_draw_string_adafruit(0, 0, 16 + (i * 12), buf, WHITE, BLACK);
            }
        }

        // --- FOOTER (BLOK PUTIH) ---
        ssd1306_draw_string_adafruit(0, 0, 52, " [OK]Pilih   [<]Back", BLACK, WHITE);
    } 
    else if (currentIRSavedState == IR_SAVED_STATE_ACTION) {
        char buf[32];
        snprintf(buf, sizeof(buf), " ACTION: %s ", listSavedRemotes[savedRemoteIndex].nama);
        // Header
        ssd1306_draw_string_adafruit(0, 0, 0, buf, BLACK, WHITE);

        // Menu Transmit / Hapus
        if (actionMenuIndex == 0) {
            ssd1306_draw_string_adafruit(0, 15, 25, "> 1. TRANSMIT", WHITE, BLACK);
            ssd1306_draw_string_adafruit(0, 15, 40, "  2. HAPUS", WHITE, BLACK);
        } else {
            ssd1306_draw_string_adafruit(0, 15, 25, "  1. TRANSMIT", WHITE, BLACK);
            ssd1306_draw_string_adafruit(0, 15, 40, "> 2. HAPUS", WHITE, BLACK);
        }
    } 
    else if (currentIRSavedState == IR_SAVED_STATE_SENDING) {
        // Layar Polos, Tulisan di Tengah!
        ssd1306_draw_string_adafruit(0, 25, 25, "IR SEND!", WHITE, BLACK);
    }

    // Refresh layar ID 0, dan force update (true)
    ssd1306_refresh(0, true); 
}

void tampilkanMenuIR() {
    ssd1306_clear(0);
    char buf[32];

    if (currentIRState == IR_STATE_CONFIRM) {
        ssd1306_draw_string_adafruit(0, 10, 10, "SNIFF IR SIGNAL", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 30, 30, "YAKIN??", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 0, 50, "[OK] Gas   [X] Back", WHITE, BLACK);
    } 
    else if (currentIRState == IR_STATE_WAITING) {
        ssd1306_draw_string_adafruit(0, 5, 20, "Menunggu", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 5, 40, "sinyal masuk...", WHITE, BLACK);
    } 
    else if (currentIRState == IR_STATE_RESULT) {
        ssd1306_draw_string_adafruit(0, 0, 0, "== IR RESULT ==", WHITE, BLACK);
        ssd1306_draw_string_adafruit(0, 0, 16, "Type: RAW CLONER", WHITE, BLACK);
        
        snprintf(buf, sizeof(buf), "Pulses: %d", last_ir_data.num_pulses);
        ssd1306_draw_string_adafruit(0, 0, 30, buf, WHITE, BLACK);
        
        ssd1306_draw_string_adafruit(0, 0, 56, "> SD Card Saved <", WHITE, BLACK);
    }
    ssd1306_refresh(0, true);
}