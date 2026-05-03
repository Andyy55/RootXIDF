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

#define IR_RX_PIN GPIO_NUM_4  
#define IR_TX_PIN GPIO_NUM_5

ir_read_state_t currentIRState = IR_STATE_CONFIRM;
ir_data_t last_ir_data = {0};
bool triggerReadIR = false;

// --- FUNGSI SD CARD ---
void simpan_ir_ke_sd() {
    FILE* f = fopen("/sdcard/ir_log.txt", "a");
    if (f == NULL) return;
    fprintf(f, "Remote_%d|%s|%08lX|%d\n", totalSavedRemotes + 1, last_ir_data.protocol, last_ir_data.hex_code, last_ir_data.bits);
    fclose(f);
    totalSavedRemotes++; 
}

void hapus_remote_di_sd(int index_target) {
    FILE* f = fopen("/sdcard/ir_log.txt", "r");
    FILE* f_temp = fopen("/sdcard/temp.txt", "w");
    if (!f || !f_temp) { if(f) fclose(f); if(f_temp) fclose(f_temp); return; }
    char line[64]; int current_idx = 0;
    while (fgets(line, sizeof(line), f)) {
        if (current_idx != index_target) fprintf(f_temp, "%s", line);
        current_idx++;
    }
    fclose(f); fclose(f_temp);
    remove("/sdcard/ir_log.txt"); rename("/sdcard/temp.txt", "/sdcard/ir_log.txt");
    totalSavedRemotes--;
}

// --- RMT V5 ENGINE ---
void build_nec_items(uint32_t hex_code, int bits, rmt_symbol_word_t* items, size_t* num_items) {
    int i = 0;
    items[i].level0 = 1; items[i].duration0 = 9000; items[i].level1 = 0; items[i].duration1 = 4500; i++;
    for (int b = 0; b < bits; b++) {
        bool bit_val = (hex_code & (1UL << b)) != 0;
        items[i].level0 = 1; items[i].duration0 = 560; items[i].level1 = 0; items[i].duration1 = bit_val ? 1690 : 560; i++;
    }
    items[i].level0 = 1; items[i].duration0 = 560; items[i].level1 = 0; items[i].duration1 = 5000; i++;
    *num_items = i;
}

void transmit_ir(uint32_t hex, int bits) {
    rmt_channel_handle_t tx_chan = NULL; // Perubahan nama di V5 terbaru
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000,
        .mem_block_symbols = 64,
        .gpio_num = IR_TX_PIN,
        .trans_queue_depth = 4,
    };
    rmt_new_tx_channel(&tx_chan_config, &tx_chan);
    rmt_carrier_config_t carrier_cfg = { .duty_cycle = 0.33, .frequency_hz = 38000 };
    rmt_apply_carrier(tx_chan, &carrier_cfg);
    rmt_encoder_handle_t copy_encoder = NULL;
    rmt_copy_encoder_config_t copy_encoder_config = {};
    rmt_new_copy_encoder(&copy_encoder_config, &copy_encoder);
    rmt_enable(tx_chan);
    size_t num_items = 0; rmt_symbol_word_t items[100];
    build_nec_items(hex, bits, items, &num_items);
    rmt_transmit_config_t tx_config = { .loop_count = 0 };
    rmt_transmit(tx_chan, copy_encoder, items, num_items, &tx_config); // num_items gak perlu dikali sizeof lagi di V5
    rmt_tx_wait_all_done(tx_chan, -1);
    rmt_disable(tx_chan); rmt_del_encoder(copy_encoder); rmt_del_channel(tx_chan);
}

bool decode_nec(rmt_symbol_word_t* item, size_t item_num) {
    if (item_num < 33) return false; 
    uint32_t data = 0; int bit_count = 0;
    for (int i = 1; i < item_num; i++) {
        if (item[i].level0 == 1 && item[i].level1 == 0) {
            if (item[i].duration1 > 1000) data |= (1UL << bit_count);
            bit_count++;
        }
        if (bit_count >= 32) break;
    }
    if (bit_count >= 16) {
        strcpy(last_ir_data.protocol, "NEC"); last_ir_data.hex_code = data; last_ir_data.bits = bit_count;
        return true;
    }
    return false;
}

static bool rmt_rx_done_cb(rmt_channel_handle_t rx_chan, const rmt_rx_done_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_wakeup = pdFALSE;
    xQueueSendFromISR((QueueHandle_t)user_ctx, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void ir_sniffer_task(void *pvParameter) {
    QueueHandle_t rx_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    rmt_channel_handle_t rx_chan = NULL;
    bool rmt_is_enabled = false;
    bool is_receiving = false; // INI KUNCI SAKTINYA BIAR GAK SPAM ERROR

    rmt_rx_channel_config_t rx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, 
        .mem_block_symbols = 64,
        .gpio_num = IR_RX_PIN,
        .flags.invert_in = true, 
    };

    if (rmt_new_rx_channel(&rx_chan_config, &rx_chan) != ESP_OK) {
        vTaskDelete(NULL);
        return;
    }

    rmt_rx_event_callbacks_t cbs = { .on_recv_done = rmt_rx_done_cb };
    rmt_rx_register_event_callbacks(rx_chan, &cbs, rx_queue);

    rmt_symbol_word_t raw_symbols[64];
    rmt_receive_config_t receive_config = { }; 

    for(;;) {
        if (currentIRState == IR_STATE_WAITING && triggerReadIR) {
            
            // 1. Nyalain RMT HANYA kalau masih mati
            if (!rmt_is_enabled) {
                rmt_enable(rx_chan);
                rmt_is_enabled = true;
                is_receiving = false;
                xQueueReset(rx_queue); // Bersihin sisa sinyal sampah
            }

            // 2. Suruh nangkep HANYA kalau lagi gak sibuk
            if (!is_receiving) {
                esp_err_t err = rmt_receive(rx_chan, raw_symbols, sizeof(raw_symbols), &receive_config);
                if (err == ESP_OK) {
                    is_receiving = true; // Tandain kalau dia lagi kerja!
                } else {
                    vTaskDelay(pdMS_TO_TICKS(50));
                    continue; // Kalo gagal, coba lagi pelan-pelan
                }
            }

            // 3. Sabar nungguin Sinyal Masuk (Disini dia gak bakal manggil receive lagi)
            rmt_rx_done_event_data_t rx_data;
            if (xQueueReceive(rx_queue, &rx_data, pdMS_TO_TICKS(200)) == pdTRUE) {
                is_receiving = false; // Sinyal masuk, status nerima kelar

                if (decode_nec(raw_symbols, rx_data.num_symbols)) {
                    simpan_ir_ke_sd();
                    currentIRState = IR_STATE_RESULT;
                    triggerReadIR = false;
                    
                    // Matiin RMT karena udah dapet sinyalnya
                    rmt_disable(rx_chan);
                    rmt_is_enabled = false;
                }
                // Catatan: Kalo dapet sinyal tapi gagal decode (noise/cahaya lampu), 
                // kodenya bakal muter dan otomatis manggil rmt_receive lagi.
            }
        } else {
            // Kalau lu keluar menu / batalin
            if (rmt_is_enabled) {
                rmt_disable(rx_chan);
                rmt_is_enabled = false;
                is_receiving = false;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}



void init_ir_system() { xTaskCreate(ir_sniffer_task, "ir_sniff", 4096, NULL, 5, NULL); }

// --- TAMPILAN OLED (VERSI LIBRARY LU) ---
extern void tampilkanMenuIR(); // Dipanggil dari display_system

// Kode tampilan dipindah ke display_system.c biar sinkron sama library lu
