#ifndef SHELLER_H
#define SHELLER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SHELLER_OK 1
#define SHELLER_ERROR 0

#define SHELLER_START_BYTE 0x23
#define SHELLER_MESSAGE_LENGTH 8
#define SHELLER_RX_BUFF_LENGTH 32
#define SHELLER_SERVICE_BYTES_COUNT 2

typedef struct {
    uint8_t  rx_buff_empty_flag;
    uint8_t  rx_buff[SHELLER_RX_BUFF_LENGTH];
    uint16_t rx_buff_begin;
    uint16_t rx_buff_end;
    uint16_t start_byte_pos;
} sheller_t;

uint8_t sheller_init(sheller_t *desc);
uint8_t sheller_push(sheller_t *desc, uint8_t byte);
uint8_t sheller_read(sheller_t *desc, uint8_t *dest);
uint8_t sheller_wrap(sheller_t *desc, uint8_t *data, uint8_t data_length, uint8_t *dest);

#endif // SHELLER_H
