#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// --- HEADER LU ---
#include "globals.h"
#include "photo_data.h"
#include "ssd1306.h"  // Sesuaikan sama nama file dari library OLED lu

// --- DEKLARASI FUNGSI DARI MANAGER LAIN ---
extern void handleJoystick(void);
extern void tampilkanLogoDulu(void);
extern void tampilkanIntroAnime(void);
extern void tampilkanTeksSplash(void);
extern void tampilkanMenuLogo(void);
extern void tampilkanMenuUtama(void);
extern void tampilkanWifiScanner(void);
extern void tampilkanDeauthScreen(void);
extern void tampilkanBrightness(void);
extern void tampilkanSpamScreen(const char* judul, const char* subTeks);
extern void loopWiFi(void *pvParameters);


// ==========================================
// THE BYPASSER: JIMAT SAKTI DEAUTH & BEACON
// ==========================================
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) { 
    return 0; // Loloskan semua paket 0xC0 & 0x80 tanpa dicek!
}


// --- DEFINISI VARIABEL GLOBAL ---
bool inSubMenu = false;
int currentMenu = 0;
int currentSub = 0;
int topMenu = 0;
WiFiData listWiFi[30];
int brightnessValue = 150;
int spamState = 0; 
bool isSpamming = false;
int aktifModeSpam = 0;
bool spamUdahSetup = false;
bool deauthUdahSetup = false;
int scannerState = 0; 
uint32_t popUpTimer = 0; 
bool triggerScan = false; 
bool scanDone = false;    
int totalWiFi = 0;
int cursorInScanner = 0; 
int scrollPosScanner = 0;
int targetLockedIdx = -1;
int contextCursor = 0;
WiFiData targetTerkunci; 
bool adaTarget = false;  
int deauthState = 0;
bool isDeauthing = false;
bool sedang_scan = false;
int appMode = 0;

TaskHandle_t TaskWiFi;

// --- FUNGSI SETUP JOYSTICK C-MURNI ---
void init_joystick() {
    int pins[] = {PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT, PIN_OK};
    for(int i = 0; i < 5; i++) {
        gpio_set_direction(pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(pins[i], GPIO_PULLUP_ONLY);
    }
}

// ==========================================
// JANTUNG UTAMA FIRMWARE LU (Pengganti Setup & Loop)
// ==========================================
void app_main(void) {
    ESP_LOGI("RootX", "System Booting...");

    // 1. Setup Input Joystick
    init_joystick();

    // 2. Setup Layar (Pake library Software I2C lu)
    // NOTE: Cek fungsi aslinya di file ssd1306.h lu, biasanya namanya ssd1306_init()
    // ssd1306_init(ID, SCL_PIN, SDA_PIN);
// 1. Nyalain Mesin OLED
if (ssd1306_init(0, 9, 8)) {
    vTaskDelay(pdMS_TO_TICKS(100)); // <--- KUNCI SAKTI: Kasih napas 100ms
    ssd1306_select_font(0, 0);
    ssd1306_clear(0);
    ssd1306_refresh(0, true);
    ESP_LOGI("RootX", "OLED Ready!");
} else {
    ESP_LOGE("RootX", "OLED Gagal Inisialisasi!");
}


    
    // (Biar layar muter 180 derajat, nanti cari fungsi flip di library barunya)

    // 3. Booting Screen
    tampilkanLogoDulu();
    tampilkanIntroAnime();
    tampilkanTeksSplash();

    // 4. Bikin Task WiFi buat Core 0
    xTaskCreatePinnedToCore(
        loopWiFi,     /* Fungsi task (ada di wifi_system.c) */
        "TaskWiFi",   /* Nama task */
        4096,         /* Stack size (di ESP-IDF dikecilin aja cukup) */
        NULL,         /* Parameter */
        1,            /* Prioritas */
        &TaskWiFi,    /* Handle */
        0             /* Core 0 */
    );

    // 5. Pengganti loop()
    while (1) {
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
        }

        // Delay wajib FreeRTOS biar Core 1 gak crash (Watchdog Timeout)
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}
