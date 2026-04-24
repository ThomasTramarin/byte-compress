#include "bcf_endian.h"
#include <stdint.h>

/**
 * Read a 32-bit unsigned integer from a little-endian buffer.
 *
 * @param buf Pointer to at least 4 bytes (LE order).
 * @return Value converted to host byte order.
 */
uint32_t read_uint32_le(const uint8_t *buf) {
    return ((uint32_t)buf[0]) |
           ((uint32_t)buf[1] << 8) |
           ((uint32_t)buf[2] << 16) |
           ((uint32_t)buf[3] << 24);
}

/**
 * Read a 16-bit unsigned integer from a little-endian buffer.
 *
 * @param buf Pointer to at least 2 bytes (LE order).
 * @return Value converted to host byte order.
 */
uint16_t read_uint16_le(const uint8_t *buf) {
    return ((uint16_t)buf[0]) |
           ((uint16_t)buf[1] << 8);
}

/**
 * Write a 32-bit unsigned integer to a buffer in little-endian order.
 *
 * @param buf Pointer to at least 4 writable bytes.
 * @param val Value in host byte order.
 */
void write_uint32_le(uint8_t *buf, uint32_t val) {
    buf[0] = (uint8_t)(val);
    buf[1] = (uint8_t)(val >> 8);
    buf[2] = (uint8_t)(val >> 16);
    buf[3] = (uint8_t)(val >> 24);
}

/**
 * Write a 16-bit unsigned integer to a buffer in little-endian order.
 *
 * @param buf Pointer to at least 2 writable bytes.
 * @param val Value in host byte order.
 */
void write_uint16_le(uint8_t *buf, uint16_t val) {
    buf[0] = (uint8_t)(val);
    buf[1] = (uint8_t)(val >> 8);
}