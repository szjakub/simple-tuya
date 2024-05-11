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

    if (len < data_len + 7)
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
    uint16_t frame_size = 7 + data_len;

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

