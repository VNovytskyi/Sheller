/*
    @file       sheller.h
    @author     vladyslavN
    @version    0.4
*/
#ifndef SHELLER_H
#define SHELLER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SHELLER_OK 1
#define SHELLER_ERROR 0

#define SHELLER_SERVICE_BYTES_COUNT 3

typedef struct {
    uint8_t start_byte;
    uint8_t usefull_data_length;
    uint16_t rx_buff_length;
    uint8_t package_length;

    uint8_t  rx_buff_empty_flag;
    uint8_t  *rx_buff;
    uint16_t rx_buff_begin;
    uint16_t rx_buff_end;
    uint16_t start_byte_pos;
} sheller_t;

uint8_t sheller_init(sheller_t *desc, uint8_t start_byte, uint8_t usefull_data_length, uint16_t rx_buff_length);
uint8_t sheller_push(sheller_t *desc, const uint8_t byte);
uint8_t sheller_read(sheller_t *desc, uint8_t *dest);
uint8_t sheller_wrap(sheller_t *desc, uint8_t *data, const uint8_t data_length, uint8_t *dest);

uint8_t sheller_get_package_length(sheller_t *desc);

#endif // SHELLER_H
