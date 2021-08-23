/*
    @file       sheller.h
    @author     vladyslavN
    @version    0.2  
*/
#ifndef SHELLER_H
#define SHELLER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SHELLER_OK 1
#define SHELLER_ERROR 0

#define SHELLER_START_BYTE 0x23
#define SHELLER_USEFULL_DATA_LENGTH 8
#define SHELLER_RX_BUFF_LENGTH 128
#define SHELLER_SERVICE_BYTES_COUNT 3
#define SHELLER_PACKAGE_LENGTH SHELLER_USEFULL_DATA_LENGTH + SHELLER_SERVICE_BYTES_COUNT

typedef struct {
    uint8_t  rx_buff_empty_flag;
    uint8_t  rx_buff[SHELLER_RX_BUFF_LENGTH];
    uint16_t rx_buff_begin;
    uint16_t rx_buff_end;
    uint16_t start_byte_pos;
} sheller_t;

uint8_t sheller_init(sheller_t *desc);
uint8_t sheller_push(sheller_t *desc, const uint8_t byte);
uint8_t sheller_read(sheller_t *desc, uint8_t *dest);
uint8_t sheller_wrap(sheller_t *desc, uint8_t *data, const uint8_t data_length, uint8_t *dest);

#endif // SHELLER_H
