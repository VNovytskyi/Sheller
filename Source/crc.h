#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#define FAST_CRC

#ifdef FAST_CRC
extern const uint16_t crc16_table[256];
#endif

uint16_t get_crc(const uint8_t *data, const uint16_t length);
void get_crc_by_byte(uint16_t *crc_value, const uint8_t byte);

#endif // CRC_H
