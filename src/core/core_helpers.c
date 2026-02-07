#include "core_helpers.h"
#include <stdint.h>

/**
 * Swap the byte order of a 32-bit unsigned integer
 *
 * Example: 0x12345678 becomes 0x78563412
 */
static inline uint32_t swap32(uint32_t val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val << 8) & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

/**
 * Swap the byte order of a 16-bit unsigned integer
 *
 * Example: 0x1234 becomes 0x3412
 */
static inline uint16_t swap16(uint16_t val) {
    return (val >> 8) | (val << 8);
}

/**
 * Detect if the current CPU is little-endian
 *
 * This function checks the memory layout of a 16-bit value.
 * - If the lowest byte is 1, CPU is little-endian
 * - Oterwise, CPU is big-endian
 *
 * @return 1 if little-endian, 0 if big-endian
 */
static inline int is_little_endian(void) {
    uint16_t x = 1;
    return *((uint8_t *)&x) == 1;
}

/**
 * Read a 32-bit unsigned integer from a byte buffer containing little-endian data.
 *
 * @param buf Pointer to at leas 4 bytes containing LE data
 * @return The 32-bit value in host byte order
 */
uint32_t read_uint32(uint8_t *buf) {
    uint32_t val = (uint32_t)buf[0] |
                   ((uint32_t)buf[1] << 8) |
                   ((uint32_t)buf[2] << 16) |
                   ((uint32_t)buf[3] << 24);
    if (!is_little_endian())
        val = swap32(val);
    return val;
}

/**
 * Read a 16-bit unsigned integer from a byte buffer containing little-endian data.
 *
 * @param buf Pointer to at leas 4 bytes containing LE data
 * @return The 16-bit value in host byte order
 */
uint16_t read_uint16(uint8_t *buf) {
    uint16_t val = (uint16_t)buf[0] |
                   ((uint16_t)buf[1] << 8);
    if (!is_little_endian())
        val = swap16(val);
    return val;
}

/**
 * Writes a 32-bit unsigned integer to a buffer in little-endian format
 *
 * - If the CPU is big-endian, swap the bytes so the result is LE
 * - Otherwise, the function has no effect
 *
 * @param buf Pointer to at least 4 bytes where the value will be written
 * @param val The 32-bit value in host byte order
 */
void write_uint32(uint8_t *buf, uint32_t val) {
    if (!is_little_endian())
        val = swap32(val);
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

/**
 * Writes a 16-bit unsigned integer to a buffer in little-endian format
 *
 * - If the CPU is big-endian, swap the bytes so the result is LE
 * - Otherwise, the function has no effect
 *
 * @param buf Pointer to at least 2 bytes where the value will be written
 * @param val The 16-bit value in host byte order
 */
void write_uint16(uint8_t *buf, uint16_t val) {
    if (!is_little_endian())
        val = swap16(val);
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}