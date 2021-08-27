#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#ifdef STM8S103
	#include "stm8s.h"
#endif

#define TABLE_CRC

uint16_t get_crc(const uint8_t *data, const uint16_t length);
void get_crc_by_byte(uint16_t *crc_value, const uint8_t byte);

#endif // CRC_H
