#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt.h"
#include "esp_log.h"
#include "globals.h"
#include "ssd1306.h"

#define IR_RX_PIN GPIO_NUM_4  
#define RMT_RX_CHANNEL RMT_CHANNEL_0
#define IR_TX_PIN GPIO_NUM_5



ir_read_state_t currentIRState = IR_STATE_CONFIRM;
ir_data_t last_ir_data = {0};
bool triggerReadIR = false;

// Fungsi SAKTI buat nulis ke SD Card
void simpan_ir_ke_sd() {
    FILE* f = fopen("/sdcard/ir_log.txt", "a");
    if (f == NULL) return;
    
    // Format: Nama|Proto|Hex|Bits
    // Contoh: Remote_1|NEC|00FF45BC|32
    fprintf(f, "Remote_%d|%s|%08lX|%d\n", 
            totalSavedRemotes + 1, 
            last_ir_data.protocol, 
            last_ir_data.hex_code, 
            last_ir_data.bits);
    fclose(f);
    totalSavedRemotes++; 
}

void hapus_remote_di_sd(int index_target) {
    FILE* f = fopen("/sdcard/ir_log.txt", "r");
    FILE* f_temp = fopen("/sdcard/temp.txt", "w");
    
    char line[64];
    int current_idx = 0;
    while (fgets(line, sizeof(line), f)) {
        if (current_idx != index_target) {
            fprintf(f_temp, "%s", line);
        }
        current_idx++;
    }
    
    fclose(f);
    fclose(f_temp);
    
    remove("/sdcard/ir_log.txt");
    rename("/sdcard/temp.txt", "/sdcard/ir_log.txt");
    totalSavedRemotes--;
}

// SULAP: Nerjemahin sinyal kedip (RMT) jadi angka Hex (Protokol NEC)
bool decode_nec(rmt_item32_t* item, size_t item_num) {
    if (item_num < 68) return false; // NEC butuh minimal 68 pulse
    
    uint32_t data = 0;
    int bit_count = 0;

    // Lewati pulse awal (Leader code) dan mulai baca bit data (Index 1 atau 2)
    for (int i = 2; i < 66; i += 2) {
        int mark = item[i].duration;
        int space = item[i+1].duration;
        
        // Logika NEC: Space pendek = 0, Space panjang = 1
        if (space > 1000) { 
            data |= (1UL << bit_count); 
        }
        bit_count++;
    }

    strcpy(last_ir_data.protocol, "NEC");
    last_ir_data.hex_code = data;
    last_ir_data.bits = bit_count;
    return true;
}
void build_nec_items(uint32_t hex_code, int bits, rmt_item32_t* items, size_t* num_items) {
    int i = 0;

    // 1. LEADER CODE (Awal mula sinyal: Mark 9000us, Space 4500us)
    items[i].level0 = 1; items[i].duration0 = 9000;
    items[i].level1 = 0; items[i].duration1 = 4500;
    i++;

    // 2. DATA BITS (Bongkar Hex jadi 1 dan 0)
    // Berdasarkan fungsi decode lu sebelumnya, kita baca dari bit terendah (LSB) ke tertinggi
    for (int b = 0; b < bits; b++) {
        bool bit_val = (hex_code & (1UL << b)) != 0;
        
        items[i].level0 = 1;
        items[i].duration0 = 560; // Pulsa nyala selalu 560us

        items[i].level1 = 0;
        if (bit_val) {
            items[i].duration1 = 1690; // Kalau '1', spasi-nya panjang (1690us)
        } else {
            items[i].duration1 = 560;  // Kalau '0', spasi-nya pendek (560us)
        }
        i++;
    }

    // 3. STOP BIT (Penutup sinyal biar mesin tahu data udah abis)
    items[i].level0 = 1; items[i].duration0 = 560;
    items[i].level1 = 0; items[i].duration1 = 0; // Space bebas karena udah kelar
    i++;

    *num_items = i; // Balikin jumlah item yang udah dirakit
}


void transmit_ir(uint32_t hex, int bits) {
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(IR_TX_PIN, RMT_CHANNEL_1);
    
    // --- SETTINGAN SAKTI 38 kHz ---
    config.tx_config.carrier_en = true;
    config.tx_config.carrier_freq_hz = 38000; // Standar remote TV/AC
    config.tx_config.carrier_duty_percent = 33; // Duty cycle 1/3 biar LED awet
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = 0;
    config.clk_div = 80; // Divider 80 di ESP32 bikin 1 tick RMT = 1 microsecond
    
    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);

    // Siapin peluru (Item RMT)
    size_t num_items = 0;
    rmt_item32_t items[100]; 
    
    // Panggil fungsi perakit peluru
    build_nec_items(hex, bits, items, &num_items);
    
    // Tembak!
    rmt_write_items(RMT_CHANNEL_1, items, num_items, true);
    rmt_wait_tx_done(RMT_CHANNEL_1, pdMS_TO_TICKS(100)); // Tunggu ampe kelar
    
    // Bersihin sisa driver biar gak makan RAM
    rmt_driver_uninstall(RMT_CHANNEL_1);
    printf("JLEBB!! Sinyal 0x%08lX (%d bits) Berhasil Ditembak!\n", hex, bits);
}

// Task di background buat nungguin remote dipencet
void ir_sniffer_task(void *pvParameter) {
    rmt_config_t rmt_rx_config = RMT_DEFAULT_CONFIG_RX(IR_RX_PIN, RMT_RX_CHANNEL);
    rmt_config(&rmt_rx_config);
    rmt_driver_install(rmt_rx_config.channel, 1000, 0);

    ringbuf_handle_t rb = NULL;
    rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rb);
    rmt_rx_start(RMT_RX_CHANNEL, true);

    for(;;) {
        if (currentIRState == IR_STATE_WAITING && triggerReadIR) {
            size_t rx_size = 0;
            // Nunggu sinyal masuk...
            rmt_item32_t *item = (rmt_item32_t *)xRingbufferReceive(rb, &rx_size, pdMS_TO_TICKS(1000));
            
            if (item) {
                int item_num = rx_size / sizeof(rmt_item32_t);
                
                // Coba pecahin kode NEC
                if (decode_nec(item, item_num)) {
                    simpan_ir_ke_sd(); // Langsung amankan ke memori!
                    currentIRState = IR_STATE_RESULT;
                    triggerReadIR = false;
                }
                vRingbufferReturnItem(rb, (void *)item);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void init_ir_system() {
    xTaskCreate(ir_sniffer_task, "ir_sniff", 4096, NULL, 5, NULL);
}

// --- TAMPILAN OLED (128x64 SAFE) ---
