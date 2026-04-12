#ifndef CRC_H
#define CRC_H
#include <stddef.h>
#include <stdint.h>

#define POLYNOMIAL 0xEDB88320

typedef uint32_t crc32_t;

crc32_t crc32_init();
crc32_t crc32_update_byte(crc32_t crc, uint8_t b);
crc32_t crc32_update_buf(crc32_t crc, const void *data, size_t len);
crc32_t crc32_finalize(crc32_t crc);

crc32_t crc32_calculate(const void *data, size_t len);

#endif