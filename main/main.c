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
#include "sdkconfig.h"
#include "esp_rom_sys.h" // Header buat CPU Speed di v5.x

// Ganti TAG biar dipake, atau hapus. Gue pakein biar gak error.
static const char *TAG = "RootX_Hardcore";

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting System Diagnostics..."); // Pake TAG biar gak error

    printf("\n\n");
    printf("################################################\n");
    printf("##      ROOTX HARDCORE SYSTEM DIAGNOSTICS     ##\n");
    printf("################################################\n");

    /* 1. DETAIL CHIP & CPU */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("[*] CHIP MODEL   : %s\n", CONFIG_IDF_TARGET);
    printf("[*] CPU CORES    : %d\n", chip_info.cores);
    
    // Pake esp_rom_get_cpu_ticks_per_us() biar pasti dapet 240MHz
    printf("[*] CPU SPEED    : %d MHz\n", (int)esp_rom_get_cpu_ticks_per_us());
    printf("[*] CHIP REV     : v%d.%d\n", chip_info.revision / 100, chip_info.revision % 100);

    /* 2. MAC ADDRESS */
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    printf("[*] MAC ADDRESS  : %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* 3. FLASH (N16) */
    uint32_t flash_size;
    esp_flash_get_size(NULL, &flash_size);
    printf("[*] FLASH SIZE   : %" PRIu32 " MB\n", flash_size / (1024 * 1024));

    /* 4. PSRAM (R8) */
    if (esp_psram_is_initialized()) {
        printf("[*] PSRAM STATUS : ENABLED\n");
        printf("[*] PSRAM TOTAL  : %d MB\n", (int)esp_psram_get_size() / (1024 * 1024));
    } else {
        printf("[!] PSRAM STATUS : DISABLED\n");
    }

    /* 5. MEMORY */
    printf("[*] DRAM FREE    : %d KB\n", (int)heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
    printf("[*] MIN FREE EVER: %d KB\n", (int)esp_get_minimum_free_heap_size() / 1024);

    /* 6. SOFTWARE */
    printf("[*] IDF VERSION  : %s\n", esp_get_idf_version());

    printf("################################################\n\n");

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Heartbeat - Free Heap: %d KB", (int)esp_get_free_heap_size() / 1024);
    }
}
