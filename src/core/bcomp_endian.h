#ifndef BCOMP_ENDIAN_H
#define BCOMP_ENDIAN_H

#include <stdint.h>

/*
 * Little-endian read/write utilities.
 *
 * These functions convert between host byte order and
 * little-endian byte buffers in a portable way.
 */

/* Read from little-endian buffers */

uint32_t read_uint32_le(const uint8_t *buf);
uint16_t read_uint16_le(const uint8_t *buf);

/* Write to little-endian buffers */

void write_uint32_le(uint8_t *buf, uint32_t val);
void write_uint16_le(uint8_t *buf, uint16_t val);

#endif /* BCOMP_ENDIAN_H */