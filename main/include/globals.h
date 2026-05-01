#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include <stdint.h>

// --- SETTING PIN JOYSTICK ---
#define PIN_UP    42
#define PIN_DOWN  41
#define PIN_LEFT  40
#define PIN_RIGHT 39
#define PIN_OK    38

// --- STRUKTUR WIFI (String diganti char array) ---
typedef struct {
  int id;
  char ssid[33]; // Max SSID length itu 32 + 1 null terminator
  int rssi;
  int channel;
  char encrypt[20];
  char mac[18];
} WiFiData;

typedef struct {
    int id;          // <--- Tambahin ID
    uint8_t mac[6];
    int rssi;
    int paket_count;
} StationInfo;

// --- EXTERN VARIABEL GLOBAL ---
// --- EXTERN VARIABEL GLOBAL ---
extern bool triggerTrack; // Tambahin ini di deretan extern bool
extern int deauthProgress;
extern bool inSubMenu;
extern int currentMenu;
extern int currentSub;
extern int topMenu;
extern WiFiData listWiFi[30];
extern StationInfo listStation[30];
extern StationInfo targetSta;
extern WiFiData targetTerkunci; 
extern int brightnessValue;
extern int spamState; 
extern bool isSpamming;
extern int aktifModeSpam;
extern bool spamUdahSetup;
extern bool deauthUdahSetup;
extern int scannerState; 
extern int scannerStateSta;  // Udah bener gak pake 'b'
extern uint32_t popUpTimer; 
extern bool triggerScan; 
extern bool triggerScanSta;  // Tambahan
extern bool scanDone;    
extern bool scanStaDone;     // Tambahan
extern int totalWiFi;
extern int totalStation;
extern int cursorInScanner; 
extern int cursorInScanSta;
extern int scrollPosScanner;
extern int targetLockedIdx;
extern int contextCursor;
extern bool adaTarget;  
extern bool adaTargetSta;    // Tambahan
extern int deauthState;
extern bool isDeauthing;
extern bool isDeauthSta;     // Tambahan
extern bool sedang_scan;
extern int appMode;

#endif
