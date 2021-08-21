#include "sheller.h"
#include <QDebug>

static const uint8_t crc8_table[256] = {
    0x00, 0x31, 0x62, 0x53, 0xC4, 0xF5, 0xA6, 0x97,
    0xB9, 0x88, 0xDB, 0xEA, 0x7D, 0x4C, 0x1F, 0x2E,
    0x43, 0x72, 0x21, 0x10, 0x87, 0xB6, 0xE5, 0xD4,
    0xFA, 0xCB, 0x98, 0xA9, 0x3E, 0x0F, 0x5C, 0x6D,
    0x86, 0xB7, 0xE4, 0xD5, 0x42, 0x73, 0x20, 0x11,
    0x3F, 0x0E, 0x5D, 0x6C, 0xFB, 0xCA, 0x99, 0xA8,
    0xC5, 0xF4, 0xA7, 0x96, 0x01, 0x30, 0x63, 0x52,
    0x7C, 0x4D, 0x1E, 0x2F, 0xB8, 0x89, 0xDA, 0xEB,
    0x3D, 0x0C, 0x5F, 0x6E, 0xF9, 0xC8, 0x9B, 0xAA,
    0x84, 0xB5, 0xE6, 0xD7, 0x40, 0x71, 0x22, 0x13,
    0x7E, 0x4F, 0x1C, 0x2D, 0xBA, 0x8B, 0xD8, 0xE9,
    0xC7, 0xF6, 0xA5, 0x94, 0x03, 0x32, 0x61, 0x50,
    0xBB, 0x8A, 0xD9, 0xE8, 0x7F, 0x4E, 0x1D, 0x2C,
    0x02, 0x33, 0x60, 0x51, 0xC6, 0xF7, 0xA4, 0x95,
    0xF8, 0xC9, 0x9A, 0xAB, 0x3C, 0x0D, 0x5E, 0x6F,
    0x41, 0x70, 0x23, 0x12, 0x85, 0xB4, 0xE7, 0xD6,
    0x7A, 0x4B, 0x18, 0x29, 0xBE, 0x8F, 0xDC, 0xED,
    0xC3, 0xF2, 0xA1, 0x90, 0x07, 0x36, 0x65, 0x54,
    0x39, 0x08, 0x5B, 0x6A, 0xFD, 0xCC, 0x9F, 0xAE,
    0x80, 0xB1, 0xE2, 0xD3, 0x44, 0x75, 0x26, 0x17,
    0xFC, 0xCD, 0x9E, 0xAF, 0x38, 0x09, 0x5A, 0x6B,
    0x45, 0x74, 0x27, 0x16, 0x81, 0xB0, 0xE3, 0xD2,
    0xBF, 0x8E, 0xDD, 0xEC, 0x7B, 0x4A, 0x19, 0x28,
    0x06, 0x37, 0x64, 0x55, 0xC2, 0xF3, 0xA0, 0x91,
    0x47, 0x76, 0x25, 0x14, 0x83, 0xB2, 0xE1, 0xD0,
    0xFE, 0xCF, 0x9C, 0xAD, 0x3A, 0x0B, 0x58, 0x69,
    0x04, 0x35, 0x66, 0x57, 0xC0, 0xF1, 0xA2, 0x93,
    0xBD, 0x8C, 0xDF, 0xEE, 0x79, 0x48, 0x1B, 0x2A,
    0xC1, 0xF0, 0xA3, 0x92, 0x05, 0x34, 0x67, 0x56,
    0x78, 0x49, 0x1A, 0x2B, 0xBC, 0x8D, 0xDE, 0xEF,
    0x82, 0xB3, 0xE0, 0xD1, 0x46, 0x77, 0x24, 0x15,
    0x3B, 0x0A, 0x59, 0x68, 0xFF, 0xCE, 0x9D, 0xAC
};

static inline uint8_t get_crc8(uint8_t *data, uint8_t length)
{
    uint8_t crc = 0xFF;
    for(uint8_t i = 0; i < length; ++i){
        crc = crc8_table[crc ^ *data++];
    }
    return crc;
}

static inline void get_crc8_by_byte(uint8_t *crc_value, uint8_t byte)
{
    *crc_value = crc8_table[*crc_value ^ byte];
}

static inline void increase_circular_value(uint16_t *value, uint16_t amount, uint16_t max_value)
{
    for (uint16_t i = 0; i < amount; ++i) {
        *value = (*value + 1) % max_value;
    }
}

//TODO: Функция не должна выполнять лишних действий ! Не изменять desc->rx_buff_begin, а возвращать
static inline uint8_t sheller_found_start_byte(sheller_t *desc)
{
    if ((desc->rx_buff_begin == desc->rx_buff_end) && desc->rx_buff_empty_flag == 0) {
        //Пройтись с ограничением в размер буффера
    } else {
        while((desc->rx_buff[desc->rx_buff_begin] != SHELLER_START_BYTE) && (desc->rx_buff_begin != desc->rx_buff_end)) {
            increase_circular_value(&desc->rx_buff_begin, 1, SHELLER_RX_BUFF_LENGTH);
        }
    }

    if (desc->rx_buff[desc->rx_buff_begin] == SHELLER_START_BYTE) {
        desc->start_byte_pos = desc->rx_buff_begin;
        return 1;
    }

    return 0;
}

static inline uint8_t sheller_try_read_data(sheller_t *desc)
{
    uint16_t received_crc_position = desc->rx_buff_begin;
    increase_circular_value(&received_crc_position, (SHELLER_MESSAGE_LENGTH + SHELLER_SERVICE_BYTES_COUNT) - 1, SHELLER_RX_BUFF_LENGTH);
    uint16_t received_crc = desc->rx_buff[received_crc_position];

    uint8_t calculate_crc = 0xFF;
    uint16_t begin = desc->rx_buff_begin;
    increase_circular_value(&begin, 1, SHELLER_RX_BUFF_LENGTH);
    for (uint8_t i = 0; i < 8; ++i) {
        get_crc8_by_byte(&calculate_crc, desc->rx_buff[begin]);
        increase_circular_value(&begin, 1, SHELLER_RX_BUFF_LENGTH);
    }

    if (calculate_crc == received_crc) {
        return 1;
    }

    return 0;
}

static inline void sheller_write_received_package(sheller_t *desc, uint8_t *dest)
{
    for (uint8_t i = 0; i < 8; ++i) {
        increase_circular_value(&desc->rx_buff_begin, 1, SHELLER_RX_BUFF_LENGTH);
        dest[i] = desc->rx_buff[desc->rx_buff_begin];
    }
}

static inline uint16_t sheller_rx_buff_used_length(sheller_t *desc)
{
    uint16_t value = 0;
    if (desc != NULL) {
        if (desc->rx_buff_end > desc->rx_buff_begin) {
            value = desc->rx_buff_end - desc->rx_buff_begin;
        } else if (desc->rx_buff_end < desc->rx_buff_begin) {
            value = (SHELLER_RX_BUFF_LENGTH - desc->rx_buff_begin) + desc->rx_buff_end;
        } else {
            if (desc->rx_buff_empty_flag == 0) {
                value = SHELLER_RX_BUFF_LENGTH;
            }
        }
    }

    return value;
}



/*!
    \brief Set initial values
    \param[in] desc - Address of sheller descriptor
    \return status of adding byte. If return 1 - operation success else NULL-pointer
*/
uint8_t sheller_init(sheller_t *desc)
{
    uint8_t init_result = SHELLER_ERROR;
    if (desc != NULL) {
        desc->rx_buff_begin = 0;
        desc->rx_buff_end   = 0;
        desc->rx_buff_empty_flag = 1;
        desc->start_byte_pos = 0;
        for (uint16_t i = 0; i < SHELLER_RX_BUFF_LENGTH; ++i) {
            desc->rx_buff[i] = 0;
        }
        init_result = SHELLER_OK;
    }

    return init_result;
}

/*!
    \brief Add new received byte to internal buffer for further work
    \param[in] desc - Address of sheller descriptor
    \param[in] byte - Value of current received byte
    \return status of adding byte. If return 1 - operation success else buffer overflow or NULL-pointer

    \details Error <buffer overflow> can occurs if a long time not to call sheller_read function
*/
uint8_t sheller_push(sheller_t *desc, uint8_t byte)
{
    /*qDebug() << "\nBefore\nSheller buff: " << QByteArray((char*)desc->rx_buff, SHELLER_RX_BUFF_LENGTH).toHex('.');
    qDebug() << "Length = " << QString::number(sheller_rx_buff_used_length(desc));
    qDebug() << "Begin = " << QString::number(desc->rx_buff_begin);
    qDebug() << "End = " << QString::number(desc->rx_buff_end);*/
    uint8_t work_result = SHELLER_ERROR;
    if (desc != NULL) {
        if (desc->rx_buff_end != desc->rx_buff_begin || desc->rx_buff_empty_flag == 1) {
            desc->rx_buff_empty_flag = 0;
            desc->rx_buff[desc->rx_buff_end] = byte;
            desc->rx_buff_end = (desc->rx_buff_end + 1) % SHELLER_RX_BUFF_LENGTH;
            work_result = SHELLER_OK;
        } else {
            //Overflow signal
            qDebug() << "Length = " << QString::number(sheller_rx_buff_used_length(desc));
            qDebug() << "Begin = " << QString::number(desc->rx_buff_begin);
            qDebug() << "End = " << QString::number(desc->rx_buff_end);
        }
    }

    /*qDebug() << "\nAfter\nSheller buff: " << QByteArray((char*)desc->rx_buff, SHELLER_RX_BUFF_LENGTH).toHex('.');
    qDebug() << "Length = " << QString::number(sheller_rx_buff_used_length(desc));
    qDebug() << "Begin = " << QString::number(desc->rx_buff_begin);
    qDebug() << "End = " << QString::number(desc->rx_buff_end);*/

    return work_result;
}

/*!
 * \brief Attempts to read the message
 * \param[in] desc - Address of sheller descriptor
 * \param[out] dest - Pointer to the buffer of received message
 * \return 1 - if read message success else - no message or message damaged
 *
 * \details If sheller_read return 1 and your system cannot handle the message immediately you to take
 *          sheller_read another buffer otherwise the data will be erased
 */
uint8_t sheller_read(sheller_t *desc, uint8_t *dest)
{
    uint8_t result = SHELLER_ERROR;
    if (desc != NULL) {
        if (sheller_rx_buff_used_length(desc) >= (SHELLER_MESSAGE_LENGTH + SHELLER_SERVICE_BYTES_COUNT)) {
            qDebug() << "Sheller buff: " << QByteArray((char*)desc->rx_buff, SHELLER_RX_BUFF_LENGTH).toHex('.');
            qDebug() << "End = " << QString::number(desc->rx_buff_end);
            qDebug() << "   y Begin = " << QString::number(desc->rx_buff_begin);
            if (sheller_found_start_byte(desc)) {
                qDebug() << "   x Begin = " << QString::number(desc->rx_buff_begin);
                if (sheller_try_read_data(desc)) {
                    qDebug() << "   0 Begin = " << QString::number(desc->rx_buff_begin);
                    sheller_write_received_package(desc, dest);
                    qDebug() << "   1 Begin = " << QString::number(desc->rx_buff_begin);
                    increase_circular_value(&desc->rx_buff_begin, 2, SHELLER_RX_BUFF_LENGTH);
                    qDebug() << "   2 Begin = " << QString::number(desc->rx_buff_begin);
                    result = SHELLER_OK;
                } else {
                    if (desc->rx_buff_begin != desc->rx_buff_end) {
                        qDebug() << "   3 Begin = " << QString::number(desc->rx_buff_begin);
                        increase_circular_value(&desc->rx_buff_begin, 1, SHELLER_RX_BUFF_LENGTH);
                        qDebug() << "   4 Begin = " << QString::number(desc->rx_buff_begin);
                    }
                }
            }

            if (desc->rx_buff_begin == desc->rx_buff_end) {
                desc->rx_buff_empty_flag = 1;
            }
        }
    }

    return result;
}

/*!
    \brief Wrap the message which we want to send. Add start byte and CRC-8 byte
    \param[in] desc - Address of sheller descriptor
    \param[in] data - Pointer to the message, which would be sent
    \param[in] data_length - Amount of the bytes in the message
    \param[out] dest - Pointer to the buffer of wrapped message
    \return status of wrapping message. If return 1 - operation success else NULL-pointer
*/
uint8_t sheller_wrap(sheller_t *desc, uint8_t *data, uint8_t data_length, uint8_t *dest)
{
    uint8_t result = SHELLER_ERROR;
    if ((desc != NULL) && (dest != NULL)) {
        dest[0] = SHELLER_START_BYTE;
        if ((data_length <= SHELLER_MESSAGE_LENGTH) && (data_length > 0)) {
            memcpy((dest + 1), data, data_length);
            dest[SHELLER_MESSAGE_LENGTH + 1] = get_crc8(data, data_length);
            result = SHELLER_OK;
        }
    }

    return result;
}
