#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "globals.h"

 // Wajib biar nyambung ke globals.h
adc_oneshot_unit_handle_t adc1_handle;

void init_battery() {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12, // Supaya bisa baca sampe 3.3V
    };
    // ADC_CHANNEL_0 = GPIO 1 di ESP32-S3
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config);
}

int read_battery_percentage() {
    int adc_raw = 0;
    long total_adc = 0;

    // 1. MULTISAMPLING: Tembak 20 kali cepet banget terus dirata-rata 
    // Biar noise listriknya langsung hilang
    for(int i = 0; i < 20; i++) {
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw);
        total_adc += adc_raw;
    }
    adc_raw = total_adc / 20;

    // 2. Konversi Raw ke Voltase (Resistor divider 1:1)
    float voltage = (adc_raw / 4095.0) * 7.8;

    // 3. Konversi ke Persentase Murni (3.3V - 4.2V)
    float currentPercent = ((voltage - 3.3) / (4.2 - 3.3)) * 100.0;
    if (currentPercent > 100.0) currentPercent = 100.0;
    if (currentPercent < 0.0) currentPercent = 0.0;

    // 4. SMOOTHING MATEMATIKA (Anti Goyang tanpa timer)
    static float smoothedPercent = -1.0; 
    if (smoothedPercent == -1.0) {
        smoothedPercent = currentPercent; // Tembakan pertama
    } else {
        // Tahan 95% angka lama, tambah 5% angka baru 
        // Efeknya baterai lu geraknya selaaaaw banget, anti joget
        smoothedPercent = (smoothedPercent * 0.95) + (currentPercent * 0.05);
    }

    batteryPercent = (int)smoothedPercent;
    return batteryPercent;
}
