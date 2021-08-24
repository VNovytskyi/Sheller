#include "sheller.h"

static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static inline uint16_t get_crc(const uint8_t *data, const uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for(uint8_t i = 0; i < length; ++i){
        crc = (crc << 8) ^ crc16_table[(crc >> 8) ^ *data++];
    }

    return crc;
}

static inline void get_crc_by_byte(uint16_t *crc_value, const uint8_t byte)
{
    *crc_value = (*crc_value << 8) ^ crc16_table[(*crc_value >> 8) ^ byte];
}


static inline void increase_circular_value(uint16_t *value, const uint16_t amount, const uint16_t max_value)
{
    for (uint16_t i = 0; i < amount; ++i) {
        *value = (*value + 1) % max_value;
    }
}

static inline uint8_t sheller_found_start_byte(sheller_t *desc)
{
    if ((desc->rx_buff_begin == desc->rx_buff_end) && desc->rx_buff_empty_flag == 0) {
        for (uint16_t i = 0; i < SHELLER_RX_BUFF_LENGTH; ++i) {
            if(desc->rx_buff[desc->rx_buff_begin] == SHELLER_START_BYTE) {
                desc->start_byte_pos = desc->rx_buff_begin;
                return 1;
            }
            increase_circular_value(&desc->rx_buff_begin, 1, SHELLER_RX_BUFF_LENGTH);
        }
    } else {
        while((desc->rx_buff[desc->rx_buff_begin] != SHELLER_START_BYTE) && (desc->rx_buff_begin != desc->rx_buff_end)) {
            increase_circular_value(&desc->rx_buff_begin, 1, SHELLER_RX_BUFF_LENGTH);
        }

        if (desc->rx_buff[desc->rx_buff_begin] == SHELLER_START_BYTE) {
            desc->start_byte_pos = desc->rx_buff_begin;
            return 1;
        }
    }

    return 0;
}

static inline uint8_t sheller_try_read_data(sheller_t *desc)
{
    uint16_t received_crc_position = desc->rx_buff_begin;
    increase_circular_value(&received_crc_position, SHELLER_USEFULL_DATA_LENGTH + 1, SHELLER_RX_BUFF_LENGTH);
    uint8_t received_crc_l = desc->rx_buff[received_crc_position];
    increase_circular_value(&received_crc_position, 1, SHELLER_RX_BUFF_LENGTH);
    uint8_t received_crc_h = desc->rx_buff[received_crc_position];
    uint16_t received_crc = received_crc_l | ((uint16_t)received_crc_h << 8);

    uint16_t calculate_crc = 0xFFFF;
    uint16_t begin = desc->rx_buff_begin;
    increase_circular_value(&begin, 1, SHELLER_RX_BUFF_LENGTH);
    for (uint8_t i = 0; i < 8; ++i) {
        get_crc_by_byte(&calculate_crc, desc->rx_buff[begin]);
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

    if (desc->rx_buff_begin == desc->rx_buff_end) {
        desc->rx_buff_empty_flag = 1;
    }
}

static inline uint16_t sheller_get_circular_buff_length(sheller_t *desc)
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
    \return result of initializing. This operation can be failed only if a pointer to sheller`s descriptor will be NULL
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
    \return status of adding byte. If return 1 - operation success else buffer overflow or NULL-pointer of sheller`s descriptor
    \details Error "BufferOverflow" can occurs if a long time not to call sheller_read function in main loop
*/
uint8_t sheller_push(sheller_t *desc, const uint8_t byte)
{
    uint8_t work_result = SHELLER_ERROR;
    if (desc != NULL) {
        if (desc->rx_buff_end != desc->rx_buff_begin || desc->rx_buff_empty_flag == 1) {
            desc->rx_buff_empty_flag = 0;
            desc->rx_buff[desc->rx_buff_end] = byte;
            desc->rx_buff_end = (desc->rx_buff_end + 1) % SHELLER_RX_BUFF_LENGTH;
            work_result = SHELLER_OK;
        }
    }

    return work_result;
}

/*!
 * \brief Attempts to read the message from internal buffer
 * \param[in] desc - Address of sheller descriptor
 * \param[out] dest - Pointer to the buffer into which the received message will be written
 * \return 1 - if read message success. 0 - no message or message damaged
 *
 * \details If sheller_read returns 1 and your system cannot handle the message immediately you need to take
 *          sheller_read another buffer otherwise the data will be erased
 */
uint8_t sheller_read(sheller_t *desc, uint8_t *dest)
{
    uint8_t result = SHELLER_ERROR;
    if (desc != NULL && dest != NULL) {
        if (sheller_get_circular_buff_length(desc) >= SHELLER_PACKAGE_LENGTH) {
            if (sheller_found_start_byte(desc)) {
                if (sheller_try_read_data(desc)) {
                    sheller_write_received_package(desc, dest);
                    increase_circular_value(&desc->rx_buff_begin, 3, SHELLER_RX_BUFF_LENGTH);
                    result = SHELLER_OK;
                } else {
                    if (desc->rx_buff_begin != desc->rx_buff_end) {
                        increase_circular_value(&desc->rx_buff_begin, 1, SHELLER_RX_BUFF_LENGTH);
                    }
                }

                if (desc->rx_buff_begin == desc->rx_buff_end) {
                    desc->rx_buff_empty_flag = 1;
                }
            }
        }
    }

    return result;
}

/*!
    \brief Wrap the message which you want to send
    \param[in] desc - Address of sheller descriptor
    \param[in] data - Pointer to the message, which would be sent
    \param[in] data_length - Amount of the bytes in the message
    \param[out] dest - Pointer to the buffer of wrapped message
    \return status of wrapping message. If return 1 - operation success else NULL-pointer
    \details Add start byte and CRC-8 byte. The length of data must be less or equal than SHELLER_MESSAGE_LENGTH
*/
uint8_t sheller_wrap(sheller_t *desc, uint8_t *data, const uint8_t data_length, uint8_t *dest)
{
    uint8_t result = SHELLER_ERROR;
    if ((desc != NULL) && (dest != NULL) && (data != NULL)) {
        if ((data_length <= SHELLER_USEFULL_DATA_LENGTH) && (data_length > 0)) {
            memset(dest, 0, SHELLER_PACKAGE_LENGTH);
            dest[0] = SHELLER_START_BYTE;
            memcpy((dest + 1), data, data_length);

            uint16_t crc = get_crc((dest + 1), SHELLER_USEFULL_DATA_LENGTH);
            dest[SHELLER_USEFULL_DATA_LENGTH + 1] = crc & 0xFF;
            dest[SHELLER_USEFULL_DATA_LENGTH + 2] = (crc >> 8) & 0xFF;

            result = SHELLER_OK;
        }
    }

    return result;
}
