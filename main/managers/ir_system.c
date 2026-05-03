#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_rx.h"
#include "driver/rmt_encoder.h"
#include "esp_log.h"
#include "globals.h"
#include "ssd1306.h"

// Ganti sesuai kaki ESP32 lu
#define IR_RX_PIN GPIO_NUM_4  
#define IR_TX_PIN GPIO_NUM_5

ir_read_state_t currentIRState = IR_STATE_CONFIRM;
ir_data_t last_ir_data = {0};
bool triggerReadIR = false;

// --- FUNGSI SD CARD ---
void simpan_ir_ke_sd() {
    FILE* f = fopen("/sdcard/ir_log.txt", "a");
    if (f == NULL) return;
    
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
    if (!f || !f_temp) {
        if(f) fclose(f);
        if(f_temp) fclose(f_temp);
        return;
    }
    
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

// ==========================================
// RMT ESP-IDF V5 ENGINE (TRANSMIT & RECEIVE)
// ==========================================

// Builder V5 untuk ngubah Hex jadi Sinyal NEC
void build_nec_items(uint32_t hex_code, int bits, rmt_symbol_word_t* items, size_t* num_items) {
    int i = 0;
    // 1. Leader Code
    items[i].level0 = 1; items[i].duration0 = 9000;
    items[i].level1 = 0; items[i].duration1 = 4500;
    i++;

    // 2. Data Bits
    for (int b = 0; b < bits; b++) {
        bool bit_val = (hex_code & (1UL << b)) != 0;
        items[i].level0 = 1; items[i].duration0 = 560;
        items[i].level1 = 0; items[i].duration1 = bit_val ? 1690 : 560;
        i++;
    }

    // 3. Stop Bit
    items[i].level0 = 1; items[i].duration0 = 560;
    items[i].level1 = 0; items[i].duration1 = 5000;
    i++;

    *num_items = i;
}

// Penembak IR V5 (Udah Built-in 38kHz Carrier)
void transmit_ir(uint32_t hex, int bits) {
    rmt_tx_channel_handle_t tx_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, // Resolusi 1us
        .mem_block_symbols = 64,
        .gpio_num = IR_TX_PIN,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_chan));

    // Hidupkan Frekuensi 38kHz biar TV kerasa
    rmt_carrier_config_t carrier_cfg = {
        .duty_cycle = 0.33,
        .frequency_hz = 38000,
        .flags = { .polarity_active_low = false }
    };
    ESP_ERROR_CHECK(rmt_apply_carrier(tx_chan, &carrier_cfg));

    rmt_encoder_handle_t copy_encoder = NULL;
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &copy_encoder));

    ESP_ERROR_CHECK(rmt_enable(tx_chan));

    size_t num_items = 0;
    rmt_symbol_word_t items[100];
    build_nec_items(hex, bits, items, &num_items);

    rmt_transmit_config_t tx_config = { .loop_count = 0 };
    ESP_ERROR_CHECK(rmt_transmit(tx_chan, copy_encoder, items, num_items * sizeof(rmt_symbol_word_t), &tx_config));
    
    // Nunggu ampe kelar nembak
    rmt_tx_wait_all_done(tx_chan, -1);

    // Hapus dari memori biar enteng
    rmt_disable(tx_chan);
    rmt_del_encoder(copy_encoder);
    rmt_del_channel(tx_chan);
    
    printf("JLEBB!! Sinyal 0x%08lX Berhasil Ditembak!\n", hex);
}

// Decoder V5 dari Sinyal ke Hex
bool decode_nec(rmt_symbol_word_t* item, size_t item_num) {
    if (item_num < 33) return false; 
    uint32_t data = 0;
    int bit_count = 0;

    for (int i = 1; i < item_num; i++) {
        if (item[i].level0 == 1 && item[i].level1 == 0) {
            int space = item[i].duration1;
            if (space > 1000) {
                data |= (1UL << bit_count);
            }
            bit_count++;
        }
        if (bit_count >= 32) break;
    }

    if (bit_count >= 16) {
        strcpy(last_ir_data.protocol, "NEC");
        last_ir_data.hex_code = data;
        last_ir_data.bits = bit_count;
        return true;
    }
    return false;
}

// Callback pas sinyal masuk
static bool rmt_rx_done_cb(rmt_rx_channel_handle_t rx_chan, const rmt_rx_done_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_wakeup = pdFALSE;
    xQueueSendFromISR((QueueHandle_t)user_ctx, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

// Task Sniffer V5
void ir_sniffer_task(void *pvParameter) {
    QueueHandle_t rx_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));

    rmt_rx_channel_handle_t rx_chan = NULL;
    rmt_rx_channel_config_t rx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000,
        .mem_block_symbols = 256,
        .gpio_num = IR_RX_PIN,
        .flags = {
            .invert_in = true, // Sensor TSOP aktifnya Low, kita balik biar jadi High!
        }
    };
    rmt_new_rx_channel(&rx_chan_config, &rx_chan);

    rmt_rx_event_callbacks_t cbs = { .on_recv_done = rmt_rx_done_cb };
    rmt_rx_register_event_callbacks(rx_chan, &cbs, rx_queue);
    rmt_enable(rx_chan);

    rmt_symbol_word_t raw_symbols[256];
    rmt_receive_config_t receive_config = { .signal_delay_tout_ms = 100 }; // Delay timeout 100ms

    for(;;) {
        if (currentIRState == IR_STATE_WAITING && triggerReadIR) {
            // Mulai nguping...
            rmt_receive(rx_chan, raw_symbols, sizeof(raw_symbols), &receive_config);

            rmt_rx_done_event_data_t rx_data;
            // Kalau dalam 1 detik dapet data...
            if (xQueueReceive(rx_queue, &rx_data, pdMS_TO_TICKS(1000)) == pdTRUE) {
                if (decode_nec(raw_symbols, rx_data.num_symbols)) {
                    simpan_ir_ke_sd();
                    currentIRState = IR_STATE_RESULT;
                    triggerReadIR = false;
                }
            } else {
                // Restart sistem nguping kalau sepi
                rmt_disable(rx_chan);
                rmt_enable(rx_chan);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

void init_ir_system() {
    xTaskCreate(ir_sniffer_task, "ir_sniff", 4096, NULL, 5, NULL);
}

// --- TAMPILAN OLED ---
extern ssd1306_handle_t dev; // Asumsi struct lu namanya dev

