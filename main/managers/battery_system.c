#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "globals.h"

int batteryPercent = 0;
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
    // Pin 36 biasanya ADC1_CH0 di ESP32. Kalau di S3 cek pinoutnya ya!
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config);
}

int read_battery_percentage() {
    int adc_raw;
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw);

    // Konversi Raw ke Voltase (ESP32-S3 ADC itu 12-bit: 0-4095)
    // Tegangan di pin = (adc_raw / 4095) * 3.3V
    // Tegangan Baterai asli = Tegangan di pin * 2 (karena resistor divider 1:1)
    float voltage = (adc_raw / 4095.0) * 3.3 * 2.0;

    // Map voltase baterai Li-ion: 3.3V (Habis) - 4.2V (Full)
    int percent = (int)((voltage - 3.3) / (4.2 - 3.3) * 100);

    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;

    batteryPercent = percent;
    return batteryPercent;
}
