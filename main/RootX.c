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

extern void loopWiFi(void *pvParameters);
extern void task_display(void *pvParameters);
extern void init_ir_system(void);

// ==========================================
// THE BYPASSER: JIMAT SAKTI DEAUTH & BEACON
// ==========================================
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) { 
    return 0; // Loloskan semua paket 0xC0 & 0x80 tanpa dicek!
}


// --- DEFINISI VARIABEL GLOBAL ---
// Di bagian definisi variabel atas
bool triggerTrack = false;

// Di dalam while(1)
// --- VARIABEL KONEKSI WIFI (BIAR LINKER GAK NGAMUK) ---
// --- VARIABEL ENGINE GAME ---
extern int baca_highscore_dino();
extern void simpan_highscore_dino(int hs);
extern int baca_highscore_snake();
extern void simpan_highscore_snake(int hs);

float rawScore = 0;
int dinoScore = 0, dinoHighScore = -1;
int dinoY = 36;        // Posisi tanah baru buat Dino 24px
float dinoVy = 0;      
bool isJumping = false;
int obstacleX = 128, obstacleY = 32, obstacleType = 0; // 0=Kaktus1, 1=Kaktus2, 2=Burung
float gameSpeed = 3.0; 
int dinoState = 0, endTimer = 0;      
int skyX = 128; // Posisi matahari/bulan

// Posisi Bintang (Latar Belakang)
int starX[5] = {20, 50, 80, 100, 120};
int starY[5] = {5, 15, 10, 20, 8};

// Definisi asli variabel Evil Twin
bool isEvilTwin = false;
int evilTwinState = 0; 
char stolenPassword[64] = "";
bool triggerEvilTwin = false;


int tetrisState = 0;
int tetrisScore = 0;
int tetrisHighScore = -1;
bool isTetrisInitialized = false;


char inputPassword[64] = {0};
int cursorPass = 0;
int statusKoneksi = 0;
bool isWiFiConnected = false;
char connSSID[33] = {0};
int connCH = 0;
int connRSSI = 0;
bool triggerConnect = false;
bool triggerDisconnect = false;

int snakeDir = 0;
int snakeState = 0;
int snakeScore = 0;
int snakeHighScore = -1;
bool isSnakeInitialized = false;

int batteryPercent = 0;
int dinoLimit = 500;   

int deauthProgress = 0;
bool adaTargetSta = false;
bool isDeauthSta = false;
bool inSubMenu = false;
int currentMenu = 0;
int currentSub = 0;
int topMenu = 0;
WiFiData listWiFi[30];
StationInfo listStation[30];
int brightnessValue = 150;
int spamState = 0; 
bool isSpamming = false;
int aktifModeSpam = 0;
bool spamUdahSetup = false;
bool deauthUdahSetup = false;
int scannerState = 0; 
int scannerStateSta = 0; 
uint32_t popUpTimer = 0; 
bool triggerScan = false; 
bool triggerScanSta = false; 
bool scanDone = false;    
bool scanStaDone = false;    
int totalWiFi = 0;
int totalStation = 0;
int cursorInScanner = 0; 
int cursorInScanSta = 0; 
int scrollPosScanner = 0;
int targetLockedIdx = -1;
int contextCursor = 0;
StationInfo targetSta;
WiFiData targetTerkunci; 
bool adaTarget = false;  
int deauthState = 0;
bool isDeauthing = false;
bool sedang_scan = false;
int appMode = 0;

TaskHandle_t TaskWiFi;

// --- FUNGSI SETUP JOYSTICK C-MURNI ---


// ==========================================
// JANTUNG UTAMA FIRMWARE LU (Pengganti Setup & Loop)
// ==========================================
void app_main(void) {
    ESP_LOGI("RootX", "System Booting...");
        xTaskCreatePinnedToCore(
    task_display,    // Nama fungsinya
    "DisplayTask",   // Nama bebas buat debug
    16384,            // Ukuran memori (8KB cukup kok)
    NULL,            // Gak ada parameter tambahan
    1,               // Prioritas (rendah aja gapapa)
    NULL,            // Gak butuh handle
    1                // <--- INI KUNCINYA (1 berarti Core 1)
);


extern bool init_sdcard(); // Kasih tau compiler fungsinya ada di file lain
    if (init_sdcard()) {
        ESP_LOGI("RootX", "Mantap, SD Card Jalan!");
    } else {
        // Kalau gagal, minimal lu tau dari log serial
        ESP_LOGE("RootX", "Yah, SD Card Gagal... Cek kabel GPIO 3 lu!");
    }
    init_ir_system(); 
    init_battery();
    xTaskCreatePinnedToCore(
        loopWiFi,     /* Fungsi task (ada di wifi_system.c) */
        "TaskWiFi",   /* Nama task */
        16384,         /* Stack size (di ESP-IDF dikecilin aja cukup) */
        NULL,         /* Parameter */
        1,            /* Prioritas */
        &TaskWiFi,    /* Handle */
        0             /* Core 0 */
    );


    // 5. Pengganti loop()

}
