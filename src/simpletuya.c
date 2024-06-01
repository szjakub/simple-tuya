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

uint8_t *u16_to_bytes(uint16_t value)
{
    unsigned long u16_size = sizeof(uint16_t);
    uint8_t *bytes = (uint8_t *)malloc(u16_size);
    for (unsigned long i = 0; i < u16_size; i++)
    {
        bytes[i] = (value >> (8 * (u16_size - 1 - i))) & 0xFF;
    }
    return bytes;
}

bool u8_in_array(uint8_t value, uint8_t *array, size_t size)
{
    for (int i = 0; i < size; i++)
    {
        if (array[i] == value)
            return true;
    }
    return false;
}

uint8_t calculate_bytes_checksum(uint8_t *bytes, size_t len)
{
    int sum = 0;
    for (size_t i = 0; i < len; i++)
        sum += bytes[i];
    return sum % 256;
}

bool is_frame_valid(DataFrame *frame)
{
    int sum = 0;
    uint8_t *header = decimal_to_bytes2(frame->header);
    sum += header[0];
    sum += header[1];
    free(header);

    sum += frame->version;
    sum += frame->command;

    uint8_t *data_len = decimal_to_bytes2(frame->data_len);

    sum += data_len[0];
    sum += data_len[1];

    DataUnit *du = frame->data_unit;
    if (frame->data_type == DATA_UNIT)
    {
        sum += du->dpid;
        sum += du->type;
        uint8_t *len = decimal_to_bytes2(du->value_len);

        sum += len[0];
        sum += len[1];

        for (int i = 0; i < du->value_len; i++)
        {
            sum += du->value[i];
        }
    }

    return sum % 256 == frame->checksum;
}

char *bucket_to_str(BytesBucket *bucket)
{
    char *str = (char *)malloc(sizeof(uint8_t) * bucket->data_len * 5);
    char *temp = str;

    for (unsigned long i = 0; i < bucket->data_len; i++)
        temp += sprintf(temp, "%02X ", bucket->bytes[i]);
    temp += '\0';

    return str;
}

char *frame_to_str(DataFrame *frame)
{
    char *str, *p;

    size_t frame_size = sizeof(DataFrame) + bytes_to_decimal2(frame->data_len);
    char p = str = (char *)malloc(sizeof(uint8_t) * frame_size * 5);

    p += sprintf(p, "%04X ", frame->header);
    p += sprintf(p, "%02X ", frame->version);
    p += sprintf(p, "%02X ", frame->command);
    p += sprintf(p, "%04X ", frame->data_len);

    if (frame->data_type == DATA_UNIT)
    {
        DataUnit *du = frame->data_unit;
        p += sprintf(p, "%02X ", du->dpid);
        p += sprintf(p, "%02X ", du->type);
        p += sprintf(p, "%04X ", du->value_len);

        for (uint16_t i = 0; i < du->value_len; i++)
        {
            p += sprintf(p, "%02X ", du->value[i]);
        }
    }
    else if (frame->data_type == RAW)
    {
        for (uint16_t i = 0; i < frame->data_len; i++)
        {
            p += sprintf(p, "%02X ", frame->raw_data[i]);
        }
    }

    p += sprintf(p, "%02X", frame->checksum);
    p += '\0';

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

    uint8_t *len_ptr = &bytes[2];
    du->value_len = bytes_to_decimal(uint16_t, len_ptr);
    du->value = (uint8_t *)malloc(sizeof(uint8_t) * du->value_len);

    for (size_t i = 0; i < du->value_len; i++)
    {
        du->value[i] = bytes[i + 4];
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

    uint8_t *header_ptr = &bytes[0];
    frame->header = bytes_to_decimal(uint16_t, header_ptr);

    frame->version = bytes[2];
    frame->command = bytes[3];

    if (u8_in_array(frame->command, DU_COMMAND_IDS, DU_COMMANDS_COUNT))
    {
        frame->data_type = DATA_UNIT;
    }
    else if (frame->data_len > 0)
    {
        frame->data_type = RAW;
    }
    else
    {
        frame->data_type = EMPTY;
    }

    uint8_t *data_len_ptr = &bytes[4];
    frame->data_len = bytes_to_decimal(uint16_t, data_len_ptr);

    if (frame->data_type == DATA_UNIT)
    {
        if (frame->data_len < DF_MIN_SIZE)
        {
            free_data_frame(frame);
            return NULL;
        }
        frame->data_unit = bytes_to_data_unit(&bytes[6], frame->data_len);
    }
    else if (frame->data_type == RAW)
    {
        frame->raw_data = (uint8_t *)malloc(sizeof(uint8_t) * frame->data_len);
        for (int i = 0; i < frame->data_len; i++)
        {
            frame->raw_data[i] = bytes[6 + i];
        }
    }

    frame->checksum = bytes[DF_MIN_SIZE - 1 + frame->data_len];
    return frame;
}

uint8_t *data_unit_to_bytes(DataUnit *data_unit)
{
    size_t du_size = DU_MIN_SIZE + data_unit->value_len;
    uint8_t *du_bytes = (uint8_t *)malloc(sizeof(uint8_t) * du_size);

    du_bytes[0] = data_unit->dpid;
    du_bytes[1] = data_unit->type;

    uint8_t *du_len_bytes = decimal_to_bytes2(data_unit->value_len);
    du_bytes[2] = du_len_bytes[0];
    du_bytes[3] = du_len_bytes[1];
    free(du_len_bytes);

    memcpy(du_bytes + 4, data_unit->value, sizeof(uint8_t) * data_unit->value_len);
    return du_bytes;
}

uint8_t *data_frame_to_bytes(DataFrame *frame)
{
    uint8_t *header_bytes = decimal_to_bytes2(frame->header);
    uint8_t *data_len_bytes = decimal_to_bytes2(frame->data_len);
    uint16_t frame_size = DF_MIN_SIZE + frame->data_len;

    uint8_t *frame_bytes = (uint8_t *)malloc(sizeof(uint8_t) * frame_size);

    frame_bytes[0] = header_bytes[0];
    frame_bytes[1] = header_bytes[1];
    frame_bytes[2] = frame->version;
    frame_bytes[3] = frame->command;
    frame_bytes[4] = data_len_bytes[0];
    frame_bytes[5] = data_len_bytes[1];

    free(data_len_bytes);
    free(header_bytes);

    if (frame->data_type == DATA_UNIT)
    {
        uint8_t *du_bytes = data_unit_to_bytes(frame->data_unit);
        memcpy(frame_bytes + 6, du_bytes, frame->data_len);
        free(du_bytes);
    }
    else if (frame->data_type == RAW)
    {
        for (size_t i = 0; i < frame->data_len; i++)
        {
            frame_bytes[6 + i] = frame->raw_data[i];
        }
    }
    frame_bytes[DF_MIN_SIZE - 1 + frame->data_len] = frame->checksum;
    return frame_bytes;
}

BytesBucket *create_bucket(uint8_t ver, uint8_t cmd, BucketConfig *config)
{
    BytesBucket *bucket = (BytesBucket *)malloc(sizeof(BytesBucket));

    size_t data_unit_len = 0;
    size_t data_unit_value_len = 0;
    uint8_t *value_bytes = NULL;

    if (config->dpid)
    {
        switch (config->type)
        {
        case BOOL_T:
        case CHAR_T:
            data_unit_value_len = 1;
            value_bytes = decimal_to_bytes(uint8_t, *(uint8_t *)config->value);
            break;
        case INT_T:
            data_unit_value_len = 4;
            value_bytes = decimal_to_bytes(int, *(int *)config->value);
            break;
        default: // for other cases length has to be specified
        {
            if (config->value_len == NULL)
                return NULL;
            data_unit_value_len = *(size_t *)config->value_len;
            value_bytes = (uint8_t *)config->value;
        }
        }
        data_unit_len = data_unit_value_len + DU_MIN_SIZE;
    }

    size_t frame_len = DF_MIN_SIZE + data_unit_len;
    uint8_t *bytes = (uint8_t *)malloc(sizeof(uint8_t) * frame_len);

    bytes[0] = 0x55;
    bytes[1] = 0xAA;
    bytes[2] = ver;
    bytes[3] = cmd;

    if (config->dpid)
    {
        bytes[4] = config->dpid;
        bytes[5] = config->type;
        memcpy(bytes + 6, config->value_len, 2);
        memcpy(bytes + 8, value_bytes, data_unit_value_len);
    }
    bytes[frame_len - 1] = calculate_bytes_checksum(bytes, frame_len - 1);

    bucket->bytes = bytes;
    bucket->data_len = frame_len;
    return bucket;
}
