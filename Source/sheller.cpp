#include "sheller.h"
#include "crc.h"

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
        } else if (desc->rx_buff_empty_flag == 0) {
            value = SHELLER_RX_BUFF_LENGTH;
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
                if (sheller_get_circular_buff_length(desc) >= SHELLER_PACKAGE_LENGTH) {
                    if (sheller_try_read_data(desc)) {
                        sheller_write_received_package(desc, dest);
                        increase_circular_value(&desc->rx_buff_begin, 3, SHELLER_RX_BUFF_LENGTH);
                        result = SHELLER_OK;
                    } else {
                        increase_circular_value(&desc->rx_buff_begin, 1, SHELLER_RX_BUFF_LENGTH);
                    }

                    if (desc->rx_buff_begin == desc->rx_buff_end) {
                        desc->rx_buff_empty_flag = 1;
                    }
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
