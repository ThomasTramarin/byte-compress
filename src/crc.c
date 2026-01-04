#include "crc.h"
#include <stdint.h>
#include <stdlib.h>

uint32_t crc_32_table[256];

void init_crc_table() {
    // for each possible byte configuration
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        // shift bit
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 1) // if the last bit is 1
                crc = (crc >> 1) ^ POLYNOMIAL;
            else
                crc >>= 1;
        }
        crc_32_table[i] = crc;
    }
}

uint32_t crc32_update(uint32_t crc, uint8_t b) {
    uint8_t index = (crc ^ b) & 0xFF;
    return (crc >> 8) ^ crc_32_table[index];
}

uint32_t crc32_update_from_buf(uint32_t crc, const uint8_t *data, size_t len) {
    while (len--) {
        uint8_t index = (crc ^ *data++) & 0xFF;
        crc = (crc >> 8) ^ crc_32_table[index];
    }
    return crc;
}