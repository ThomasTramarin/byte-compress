#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>

#define ARR_SIZE(x) (sizeof(x) / sizeof((x)[0]))

void to_be32(uint8_t *buf, uint32_t val);
uint32_t from_be32(const uint8_t *buf);

void to_be16(uint8_t *buf, uint16_t val);
uint16_t from_be16(const uint8_t *buf);

#endif