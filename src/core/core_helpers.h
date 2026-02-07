#ifndef CORE_HELPERS_H
#define CORE_HELPERS_H

#include <stdint.h>

uint32_t read_uint32(uint8_t *buf);
uint16_t read_uint16(uint8_t *buf);

void write_uint32(uint8_t *buf, uint32_t val);
void write_uint16(uint8_t *buf, uint16_t val);

#endif