#ifndef CRC_H
#define CRC_H
#include <stddef.h>
#include <stdint.h>

#define POLYNOMIAL 0xEDB88320

extern uint32_t crc_32_table[256];

void init_crc_table();
uint32_t crc32_update(uint32_t crc, uint8_t b);
uint32_t crc32_update_from_buf(uint32_t crc, const uint8_t *data, size_t len);

#endif