#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_ota_ops.h"
#include "esp_netif.h"
#include "esp_mac.h"

static const char *TAG = "RootX_Hardcore";

void app_main(void) {
    // 1. Inisialisasi NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("\n\n");
    printf("################################################\n");
    printf("##      ROOTX HARDCORE SYSTEM DIAGNOSTICS     ##\n");
    printf("################################################\n");

    /* 1. DETAIL CHIP & CPU */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("[*] CHIP MODEL   : %s (S3)\n", CONFIG_IDF_TARGET);
    printf("[*] CPU CORES    : %d\n", chip_info.cores);
    printf("[*] CPU SPEED    : %" PRIu32 " MHz\n", esp_clk_cpu_freq() / 1000000);
    printf("[*] CHIP REV     : v%d.%d\n", chip_info.revision / 100, chip_info.revision % 100);

    /* 2. DETAIL MAC ADDRESS (ID Unik Alat Lu) */
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    printf("[*] MAC ADDRESS  : %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* 3. DETAIL FLASH STORAGE (N16 - 16MB) */
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    printf("[*] FLASH SIZE   : %" PRIu32 " MB (%s)\n", flash_size / (1024 * 1024), 
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded" : "External");

    /* 4. DETAIL PSRAM (R8 - 8MB) */
    if (esp_psram_is_initialized()) {
        size_t psram_total = esp_psram_get_size();
        size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        printf("[*] PSRAM STATUS : ENABLED\n");
        printf("[*] PSRAM TOTAL  : %d MB\n", (int)psram_total / (1024 * 1024));
        printf("[*] PSRAM FREE   : %d KB\n", (int)psram_free / 1024);
    } else {
        printf("[!] PSRAM STATUS : DISABLED / NOT FOUND (Cek sdkconfig lu!)\n");
    }

    /* 5. MEMORY MANAGEMENT (RAM Internal) */
    printf("[*] DRAM FREE    : %d KB\n", (int)heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
    printf("[*] IRAM FREE    : %d KB\n", (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024);
    printf("[*] MIN FREE EVER: %d KB (Watermark)\n", (int)esp_get_minimum_free_heap_size() / 1024);

    /* 6. SOFTWARE INFO */
    const esp_app_desc_t *app_desc = esp_app_get_description();
    printf("[*] IDF VERSION  : %s\n", esp_get_idf_version());
    printf("[*] PROJECT NAME : %s\n", app_desc->project_name);
    printf("[*] BUILD DATE   : %s %s\n", app_desc->date, app_desc->time);

    printf("################################################\n");
    printf("##  STATUS: S3 READY TO INJECT RAW PACKETS!   ##\n");
    printf("################################################\n\n");

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        // Monitor RAM biar ketauan kalo bocor (memory leak)
        printf("[Monitor] Free Heap: %d KB | PSRAM Free: %d KB\n", 
               (int)esp_get_free_heap_size() / 1024,
               (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
    }
}
