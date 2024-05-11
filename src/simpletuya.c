#include "simpletuya.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *   Example raw frame
 *
 *   data_frame
 *   |  header | version | command | data length |          data           | checksum |
 *   |  55 AA  |    03   |    07   |    00 08    | 6D 02 00 04 00 00 00 3B |    BF    |
 *
 *   data can be parsed into data unit
 *
 *   data_unit
 *   | dpid | type |  len  |   value     |
 *   |  6D  |  02  | 00 04 | 00 00 00 3B |
 *
 *
 */

uint8_t TUYA_FRAME_HEADER[] = {0x55, 0xAA};


uint8_t *u16_to_bytes(uint16_t value)
{
    unsigned long u16_size = sizeof(uint16_t);
    uint8_t *bytes = (uint8_t *)malloc(u16_size);
    for (unsigned long i=0; i<u16_size; i++)
    {
        bytes[i] = (value >> (8 * (u16_size - 1 - i))) & 0xFF;
    }
    return bytes;
}


uint8_t calculate_bytes_checksum(uint8_t *bytes, size_t len)
{
    int sum = 0;
    for (int i=0; i<len; i++)
        sum += bytes[i];
    return sum % 256;
}


bool is_frame_valid(DataFrame *frame)
{
    int sum = 0;
    sum += frame->header[0];
    sum += frame->header[1];
    sum += frame->version;
    sum += frame->command;
    sum += frame->data_len[0];
    sum += frame->data_len[1];

    DataUnit *du = frame->data_unit;

    sum += du->dpid;
    sum += du->type;
    sum += du->len[0];
    sum += du->len[1];

    uint8_t len_bytes[2] = {du->len[0], du->len[1]};
    uint16_t data_len = bytes_to_decimal(uint16_t, len_bytes);

    for (int i=0; i<data_len; i++)
        sum += du->value[i];

    return sum % 256 == frame->checksum;
}


char *bucket_to_str(BytesBucket *bucket)
{
    char *str = (char *)malloc(sizeof(uint8_t) * bucket->data_len * 5);
    char *temp = str;

    for (unsigned long i=0; i<bucket->data_len; i++)
        temp += sprintf(temp, "%02X ", bucket->bytes[i]);
    temp += '\0';

    return str;
}


char *frame_to_str(DataFrame *frame)
{
    size_t frame_size = sizeof(DataFrame) + bytes_to_decimal(uint16_t, frame->data_len);

    char *str = (char *)malloc(sizeof(uint8_t) * frame_size * 5);
    char *temp = str;

    temp += sprintf(temp, "%02X%02X ", frame->header[0], frame->header[1]);
    temp += sprintf(temp, "%02X ", frame->version);
    temp += sprintf(temp, "%02X ", frame->command);
    temp += sprintf(temp, "%02X%02X ", frame->data_len[0], frame->data_len[1]);

    DataUnit *du = frame->data_unit;
    temp += sprintf(temp, "%02X ", du->dpid);
    temp += sprintf(temp, "%02X ", du->type);
    temp += sprintf(temp, "%02X%02X ", du->len[0], du->len[1]);

    uint8_t *len_ptr = &du->len[0];
    uint16_t data_len = bytes_to_decimal(uint16_t, len_ptr);

    for (uint16_t i = 0; i < data_len; i++)
    {
        temp += sprintf(temp, "%02X ", du->value[i]);
    }
    temp += sprintf(temp, "%02X", frame->checksum);
    temp += '\0';

    return str;
}


void free_data_unit(DataUnit *du)
{
    free(du->value);
    free(du);
}


void free_data_frame(DataFrame *frame)
{
    if (frame->data_unit == NULL)
        free_data_unit(frame->data_unit);
    free(frame);
}


void free_bytes_bucket(BytesBucket *bucket)
{
    free(bucket->bytes);
    free(bucket);
}


DataUnit *bytes_to_data_unit(uint8_t *bytes, size_t len)
{
    if (len < 4)
        return NULL;

    DataUnit *du = (DataUnit *)malloc(sizeof(DataUnit));

    du->dpid = bytes[0];
    du->type = bytes[1];
    du->len[0] = bytes[2];
    du->len[1] = bytes[3];

    uint8_t *len_ptr = &bytes[2];
    uint16_t data_len = bytes_to_decimal(uint16_t, len_ptr);

    du->value = (uint8_t *)malloc(sizeof(uint8_t));

    for (size_t i=0; i<data_len; i++)
    {
        du->value[i] = bytes[i+4];
    }
    return du;
}


DataFrame *bytes_to_data_frame(uint8_t *bytes, size_t len)
{
    if (len < 7)
        return NULL;

    uint8_t h1 = bytes[0];
    uint8_t h2 = bytes[1];
    while (h1 != TUYA_FRAME_HEADER[0] || h2 != TUYA_FRAME_HEADER[1])
    {
        len--;
        if (len < 7)
            return NULL;

        bytes++;
        h1 = bytes[0], h2 = bytes[1];
    }

    DataFrame *frame = (DataFrame *)malloc(sizeof(DataFrame));
    frame->data_unit = NULL;

    frame->header[0] = h1, frame->header[1] = h2;
    frame->version = bytes[2];
    frame->command = bytes[3];
    frame->data_len[0] = bytes[4];
    frame->data_len[1] = bytes[5];

    uint8_t *len_ptr = &bytes[4];
    uint16_t data_len = bytes_to_decimal(uint16_t, len_ptr);

    if (len < DF_MIN_SIZE + data_len)
    {
        free_data_frame(frame);
        return NULL;
    }

    if (data_len != 0)
        frame->data_unit = bytes_to_data_unit(&bytes[6], data_len);

    frame->checksum = bytes[6 + data_len];

    return frame;
}


uint8_t *data_unit_to_bytes(DataUnit *du)
{
    uint8_t *len_ptr = &du->len[0];
    uint16_t data_len = bytes_to_decimal(uint16_t, len_ptr);
    size_t du_size = 4 + data_len;

    uint8_t *du_bytes = (uint8_t *)malloc(sizeof(uint8_t) * du_size);

    du_bytes[0] = du->dpid;
    du_bytes[1] = du->type;
    du_bytes[2] = du->len[0];
    du_bytes[3] = du->len[1];

    memcpy(du_bytes + 4, du->value, sizeof(uint8_t) * data_len);

    return du_bytes;
}


uint8_t *data_frame_to_bytes(DataFrame *frame)
{
    uint8_t *len_ptr = &frame->data_len[0];
    uint16_t data_len = bytes_to_decimal(uint16_t, len_ptr);
    uint16_t frame_size = DF_MIN_SIZE + data_len;

    uint8_t *frame_bytes = (uint8_t *)malloc(sizeof(uint8_t) * frame_size);

    frame_bytes[0] = frame->header[0];
    frame_bytes[1] = frame->header[1];
    frame_bytes[2] = frame->version;
    frame_bytes[3] = frame->command;
    frame_bytes[4] = frame->data_len[0];
    frame_bytes[5] = frame->data_len[1];

    uint8_t *du_bytes = data_unit_to_bytes(frame->data_unit);
    memcpy(frame_bytes + 6, du_bytes, data_len);

    free(du_bytes);

    frame_bytes[6 + data_len] = frame->checksum;
    return frame_bytes;
}


BytesBucket *create_bucket(uint8_t ver, Command cmd, uint8_t dpid,
                           DataType type, void *value, uint16_t *value_len)
{
    uint16_t _value_len = 0;
    uint8_t *value_bytes = NULL;

    switch (type)
    {
        case BOOL_T:
        case CHAR_T:
            _value_len = 1;
            value_bytes = decimal_to_bytes(uint8_t, *(uint8_t *)value);
            break;
        case INT_T:
            _value_len = 4;
            value_bytes = decimal_to_bytes(int, *(int *)value);
            break;
        default: // for other cases length has to be specified
            if (value_len == NULL)
                return NULL;
            _value_len = *value_len;
            value_bytes = (uint8_t *)value;
    }

    uint16_t frame_len = DF_MIN_SIZE + DU_MIN_SIZE + _value_len;

    BytesBucket *bucket = (BytesBucket *)malloc(sizeof(BytesBucket));
    uint8_t *bytes = (uint8_t *)malloc(sizeof(uint8_t) * _value_len);

    bytes[0] = 0x55;
    bytes[1] = 0xAA;
    bytes[2] = ver;
    bytes[3] = cmd;

    uint16_t data_len =  DU_MIN_SIZE + _value_len;
    uint8_t *data_len_bytes = u16_to_bytes(data_len);
    bytes[4] = data_len_bytes[0];
    bytes[5] = data_len_bytes[1];
    free(data_len_bytes);

    bytes[6] = dpid;
    bytes[7] = type;

    uint8_t *value_len_bytes = u16_to_bytes(_value_len);
    bytes[8] = value_len_bytes[0];
    bytes[9] = value_len_bytes[1];
    free(value_len_bytes);

    memcpy(bytes + 10, value_bytes, _value_len);
    bytes[10 + _value_len] = calculate_bytes_checksum(bytes, frame_len - 1);

    free(value_bytes);

    bucket->bytes = bytes;
    bucket->data_len = frame_len;

    return bucket;
}




