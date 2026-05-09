#include "tvbgone_engine.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Tarik fungsi nembak IR lu dari file lain
extern void transmit_ir_raw(uint16_t* pulses, int num_pulses);

void send_tvbgone_code(const struct IrCode *irCode) {
    uint16_t raw_pulses[250]; // Buffer maksimal (biasanya kode TV gak lebih dari 200 pulsa)
    int pulse_idx = 0;

    // Ambil info dari struct
    int numpairs = irCode->numpairs;
    int bitcompression = irCode->bitcompression;
    const uint16_t *times = irCode->times;
    const uint8_t *codes = irCode->codes;

    int bit_idx = 0;  // Penunjuk bit ke berapa di dalam 1 byte
    int byte_idx = 0; // Penunjuk array byte ke berapa

    // BONGKAR KOMPRESI (Decompressor)
    for (int i = 0; i < numpairs; i++) {
        int code_index = 0;
        
        // Baca bit sebanyak "bitcompression" (biasanya 2 bit atau 4 bit)
        for (int b = 0; b < bitcompression; b++) {
            // Geser bit dan ambil nilainya (1 atau 0)
            int bit = (codes[byte_idx] >> (7 - bit_idx)) & 1;
            code_index = (code_index << 1) | bit; // Rangkai bit-nya jadi angka index

            bit_idx++;
            if (bit_idx == 8) { // Kalo udah nyampe 8 bit, pindah ke byte berikutnya
                bit_idx = 0;
                byte_idx++;
            }
        }

        // Kalau indexnya udah dapet, cari di tabel waktu (times)
        uint16_t onTime = times[code_index * 2];       // Waktu nyala (Mark)
        uint16_t offTime = times[code_index * 2 + 1];  // Waktu mati (Space)

        raw_pulses[pulse_idx++] = onTime;
        raw_pulses[pulse_idx++] = offTime;
    }

    // TERJEMAHAN SELESAI! LANGSUNG TEMBAK!
    // (Opsional: Kalau fungsi lu butuh setting frekuensi, ambil dari irCode->freq)
    transmit_ir_raw(raw_pulses, pulse_idx);
}
