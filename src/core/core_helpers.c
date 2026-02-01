#include "core_helpers.h"
#include <stdint.h>

/**
 * Converts a 32-bit unsigned integer to a Big-Endian byte order.
 *
 * @param buf Destination buffer (at least 4 bytes)
 * @param val The 32-bit value to convert.
 */
void to_be32(uint8_t *buf, uint32_t val) {
    buf[0] = (uint8_t)(val >> 24);
    buf[1] = (uint8_t)(val >> 16);
    buf[2] = (uint8_t)(val >> 8);
    buf[3] = (uint8_t)(val);
}

/**
 * Converts a Big-Endian sequence to a 32-bit unsigned integer
 *
 * @param buf Source buffer containing at least 4 bytes
 * @return The 32-bit unsigneg integer
 */
uint32_t from_be32(const uint8_t *buf) {
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) |
           ((uint32_t)buf[3]);
}

/**
 * Converts a 16-bit unsigned integer to a Big-Endian byte order.
 *
 * @param buf Destination buffer (at least 2 bytes)
 * @param val The 16-bit value to convert.
 */
void to_be16(uint8_t *buf, uint16_t val) {
    buf[0] = (uint8_t)(val >> 8);
    buf[1] = (uint8_t)(val);
}

/**
 * Converts a Big-Endian sequence to a 16-bit unsigned integer.
 *
 * @param buf Source buffer containing at least 2 bytes
 * @return The 16-bit unsigned integer
 */
uint16_t from_be16(const uint8_t *buf) {
    return ((uint16_t)buf[0] << 8) |
           ((uint16_t)buf[1]);
}