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

const uint16_t TUYA_FRAME_HEADER_SHORT = 0x55AA;
const uint8_t TUYA_FRAME_HEADER[HEADER_SIZE] = {0x55, 0xAA};

const uint8_t DU_COMMAND_IDS[DU_COMMANDS_COUNT] = {0x06, 0x07, 0x22, 0x26};

bool u8_in_array(uint8_t value, const uint8_t *array, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (array[i] == value)
            return true;
    }
    return false;
}

uint8_t calculate_bytes_checksum(uint8_t *bytes, size_t len) {
    int sum = 0;
    for (size_t i = 0; i < len; i++)
        sum += bytes[i];
    return sum % 256;
}

uint8_t calculate_frame_checksum(DataFrame *frame) {
    int sum = 0;
    uint8_t header_bytes[2];
    decimal_to_bytes(header_bytes, frame->header);

    sum += header_bytes[0];
    sum += header_bytes[1];

    sum += frame->version;
    sum += frame->command;

    uint8_t data_len_bytes[2];
    decimal_to_bytes(data_len_bytes, frame->data_len);

    sum += data_len_bytes[0];
    sum += data_len_bytes[1];

    if (frame->data_type == DT_UNIT) {
        DataUnit *du = frame->data_unit;
        sum += du->dpid;
        sum += du->type;

        uint8_t du_len_bytes[2];
        decimal_to_bytes(du_len_bytes, du->value_len);

        sum += du_len_bytes[0];
        sum += du_len_bytes[1];

        uint8_t buffer[512];

        switch (du->type) {
            case TYPE_INT:
                decimal_to_bytes(buffer, du->int_value);
                du->value_len = 4;
                break;
            case TYPE_CHAR:
            case TYPE_BOOL:
                buffer[0] = du->byte_value;
                du->value_len = 1;
                break;
            case TYPE_RAW:
            case TYPE_STR:
            case TYPE_BITMAP:
                memcpy(buffer, du->array_value, sizeof(uint8_t) * du->value_len);
                break;
        }

        for (int i = 0; i < du->value_len; i++) {
            sum += buffer[i];
        }
    }
    else if (frame->data_type == DT_RAW) {
        for (int i = 0; i < frame->data_len; i++) {
            sum += frame->raw_data[i];
        }
    }

    return sum % 256;
}


bool is_frame_valid(DataFrame *frame) {
    uint8_t checksum = calculate_frame_checksum(frame);
    return checksum == frame->checksum;
}

char *bytes_array_to_str(BytesArray *bytes_array) {
    char *str = (char *) malloc(sizeof(uint8_t) * bytes_array->len * 5);
    char *temp = str;

    for (unsigned long i = 0; i < bytes_array->len; i++)
        temp += sprintf(temp, "%02X ", bytes_array->bytes[i]);
    temp += '\0';

    return str;
}

char *frame_to_str(DataFrame *frame) {
    char *str, *p;

    size_t frame_size = sizeof(DataFrame) + frame->data_len;
    p = str = (char *) malloc(sizeof(uint8_t) * frame_size * 5);

    p += sprintf(p, "%04X ", frame->header);
    p += sprintf(p, "%02X ", frame->version);
    p += sprintf(p, "%02X ", frame->command);
    p += sprintf(p, "%04X ", frame->data_len);

    if (frame->data_type == DT_UNIT) {
        DataUnit *du = frame->data_unit;
        p += sprintf(p, "%02X ", du->dpid);
        p += sprintf(p, "%02X ", du->type);
        p += sprintf(p, "%04X ", du->value_len);

        uint8_t buffer[512];

        switch (du->type) {
            case TYPE_INT:
                printf("%d", du->int_value);
                decimal_to_bytes(buffer, du->int_value);
                du->value_len = 4;
                break;
            case TYPE_CHAR:
            case TYPE_BOOL:
                printf("%d", du->byte_value);
                buffer[0] = du->byte_value;
                du->value_len = 1;
                break;
            case TYPE_RAW:
            case TYPE_STR:
            case TYPE_BITMAP:
                memcpy(buffer, du->array_value, sizeof(uint8_t) * du->value_len);
                break;
        }

        for (uint16_t i = 0; i < du->value_len; i++) {
            p += sprintf(p, "%02X ", buffer[i]);
        }
    } else if (frame->data_type == DT_RAW) {
        for (uint16_t i = 0; i < frame->data_len; i++) {
            p += sprintf(p, "%02X ", frame->raw_data[i]);
        }
    }

    p += sprintf(p, "%02X", frame->checksum);
    p += '\0';

    return str;
}

void free_data_unit(DataUnit *du) {
    if (du->type == TYPE_RAW || du->type == TYPE_STR || du->type == TYPE_BITMAP) {
        free(du->array_value);
    }
    free(du);
}

void free_data_frame(DataFrame *frame) {
    if (frame->data_type == DT_UNIT) {
        free(frame->data_unit);
    }
    else if (frame->data_type == DT_RAW) {
        free(frame->raw_data);
    }
    free(frame);
}

// void free_bytes_bytes_array(BytesArray *bytes_array) {
//     free(bytes_array->bytes);
//     free(bytes_array);
// }

DataUnit *bytes2du(uint8_t *bytes, size_t len) {
    if (len < 4)
        return NULL;

    DataUnit *du = (DataUnit *) malloc(sizeof(DataUnit));

    du->dpid = bytes[0];
    du->type = bytes[1];

    uint8_t *len_ptr = &bytes[2];
    du->value_len = bytes_to_decimal(uint16_t, len_ptr);

    switch (du->type) {
        case TYPE_INT:
            du->int_value = bytes_to_decimal(int, (bytes + 3));
            break;
        case TYPE_CHAR:
        case TYPE_BOOL:
            du->byte_value = bytes[3];
            break;
        case TYPE_RAW:
        case TYPE_STR:
        case TYPE_BITMAP:
            du->array_value = (uint8_t *) malloc(sizeof(uint8_t) * du->value_len);
            memcpy(du->array_value, bytes + 3, du->value_len);
            break;
    }
    return du;
}

DataFrame *bytes2df(uint8_t *src, size_t len) {
    /*
     *  DataFrame can contain either DataUnit or raw data
     *  so having buffer here is not very clean option
     *  TODO: check which to deallocate on memory release
     */
    if (len < 7)
        return NULL;

    uint8_t h1 = src[0];
    uint8_t h2 = src[1];
    while (h1 != TUYA_FRAME_HEADER[0] || h2 != TUYA_FRAME_HEADER[1]) {
        len--;
        if (len < 7)
            return NULL;

        src++;
        h1 = src[0], h2 = src[1];
    }

    DataFrame *frame = (DataFrame *) malloc(sizeof(DataFrame));

    const uint8_t *header_ptr = &src[0];
    frame->header = bytes_to_decimal(uint16_t, header_ptr);

    frame->version = src[2];
    frame->command = src[3];

    if (u8_in_array(frame->command, DU_COMMAND_IDS, DU_COMMANDS_COUNT)) {
        frame->data_type = DT_UNIT;
    } else if (frame->data_len > 0) {
        frame->data_type = DT_RAW;
    } else {
        frame->data_type = DT_EMPTY;
    }

    const uint8_t *data_len_ptr = &src[4];
    frame->data_len = bytes_to_decimal(uint16_t, data_len_ptr);

    if (frame->data_type == DT_UNIT) {
        if (frame->data_len < DU_MIN_SIZE) {
            free_data_frame(frame);
            return NULL;
        }
        frame->data_unit = bytes2du(&src[6], frame->data_len);
    } else if (frame->data_type == DT_RAW) {
        frame->raw_data = (uint8_t *) malloc(sizeof(uint8_t) * frame->data_len);
        for (int i = 0; i < frame->data_len; i++) {
            frame->raw_data[i] = src[6 + i];
        }
    }

    frame->checksum = src[DF_MIN_SIZE - 1 + frame->data_len];
    return frame;
}

uint8_t *du2bytes(const DataUnit *du) {
    const size_t du_size = DU_MIN_SIZE + du->value_len;
    uint8_t *du_bytes = (uint8_t *) malloc(sizeof(uint8_t) * du_size);

    du_bytes[0] = du->dpid;
    du_bytes[1] = du->type;

    uint8_t du_len_bytes[2];
    decimal_to_bytes(du_len_bytes, du->value_len);

    du_bytes[2] = du_len_bytes[0];
    du_bytes[3] = du_len_bytes[1];

    switch (du->type) {
        case TYPE_INT:
            uint8_t buffer[4];
            decimal_to_bytes(buffer, du->int_value);
            memcpy(du_bytes + 4, buffer, sizeof(uint8_t) * 4);
            break;
        case TYPE_CHAR:
        case TYPE_BOOL:
            du_bytes[4] = du->byte_value;
            break;
        case TYPE_RAW:
        case TYPE_STR:
        case TYPE_BITMAP:
            memcpy(du_bytes + 4, du->array_value, sizeof(uint8_t) * du->value_len);
            break;
    }
    return du_bytes;
}

uint8_t *df2bytes(const DataFrame *frame) {
    const uint16_t frame_size = DF_MIN_SIZE + frame->data_len;
    uint8_t *frame_bytes = (uint8_t *) malloc(sizeof(uint8_t) * frame_size);

    uint8_t header_bytes[2];
    decimal_to_bytes(header_bytes, frame->header);
    memcpy(frame_bytes, header_bytes, sizeof(uint8_t) * 2);

    frame_bytes[2] = frame->version;
    frame_bytes[3] = frame->command;

    uint8_t data_len_bytes[2];
    decimal_to_bytes(data_len_bytes, frame->data_len);
    memcpy(frame_bytes + 4, data_len_bytes, sizeof(uint8_t) * 2);

    if (frame->data_type == DT_UNIT) {
        uint8_t *du_bytes = du2bytes(frame->data_unit);
        memcpy(frame_bytes + 6, du_bytes, frame->data_len);
        free(du_bytes);
    } else if (frame->data_type == DT_RAW) {
        for (size_t i = 0; i < frame->data_len; i++) {
            frame_bytes[6 + i] = frame->raw_data[i];
        }
    }
    frame_bytes[DF_MIN_SIZE - 1 + frame->data_len] = frame->checksum;
    return frame_bytes;
}
//
//
// uint8_t *data_frame_to_bytes(const DataFrame *frame, BytesArray *dest) {
//     uint8_t header_bytes[2];
//     decimal_to_bytes(header_bytes, frame->header);
//
//     uint8_t data_len_bytes[2];
//     decimal_to_bytes(data_len_bytes, frame->data_len);
//
//     const uint16_t frame_size = DF_MIN_SIZE + frame->data_len;
//
//     uint8_t *frame_bytes = (uint8_t *) malloc(sizeof(uint8_t) * frame_size);
//
//     frame_bytes[0] = header_bytes[0];
//     frame_bytes[1] = header_bytes[1];
//     frame_bytes[2] = frame->version;
//     frame_bytes[3] = frame->command;
//     frame_bytes[4] = data_len_bytes[0];
//     frame_bytes[5] = data_len_bytes[1];
//
//     if (frame->data_type == DATA_UNIT) {
//         uint8_t *du_bytes = data_unit_to_bytes(frame->data_unit);
//         memcpy(frame_bytes + 6, du_bytes, frame->data_len);
//         free(du_bytes);
//     } else if (frame->data_type == RAW_DATA) {
//         for (size_t i = 0; i < frame->data_len; i++) {
//             frame_bytes[6 + i] = frame->raw_data[i];
//         }
//     }
//     frame_bytes[DF_MIN_SIZE - 1 + frame->data_len] = frame->checksum;
//     return frame_bytes;
// }

void init_data_unit(DataUnit *du, const DataUnitDTO *params) {
    du->dpid = params->dpid;
    du->type = params->type;

    switch (params->type) {
        case TYPE_INT:
            du->int_value = *(uint8_t *) params->value;
            du->value_len = 4;
            break;
        case TYPE_BOOL:
        case TYPE_CHAR:
            du->byte_value = *(uint8_t *) params->value;
            du->value_len = 1;
            break;
        case TYPE_RAW:
        case TYPE_STR:
        case TYPE_BITMAP:
            du->array_value = (uint8_t *) params->value;
            du->value_len = params->value_len;
            break;
    }
}

void init_data_frame(DataFrame *frame, const DataFrameDTO *params) {
    frame->header = bytes_to_decimal(uint16_t, TUYA_FRAME_HEADER);
    frame->version = params->ver;
    frame->command = params->cmd;

    switch (params->data_type) {
        case DT_EMPTY: {
            frame->data_len = 0;
        }
        case DT_UNIT: {
            frame->data_unit = params->du;
            frame->data_len = DU_MIN_SIZE + params->du->value_len;
        }
        case DT_RAW: {
            frame->raw_data = params->raw_data->bytes;
            frame->data_len = params->raw_data->len;
        }
    }
    frame->checksum = calculate_frame_checksum(frame);
}


// BytesArray *create_bytes_array(uint8_t ver, uint8_t cmd, DataUnitParams *du_params)
// {
//
//     if (du_params != NULL)
//     {
//         switch (du_params->type)
//         {
//             case BOOL_T:
//             case CHAR_T:
//         }
//     }
// }
//
//
// BytesArray *create_bytes_array(uint8_t ver, uint8_t cmd, DataUnitParams *du_params)
// {
//     // TODO: Rewrite this shit
//     BytesArray *bytes_array = (BytesArray *)malloc(sizeof(BytesArray));
//
//     size_t data_unit_len = 0;
//     size_t data_unit_value_len = 0;
//     uint8_t *value_bytes = NULL;
//
//     if (du_params->dpid)
//     {
//         switch (du_params->type)
//         {
//         case BOOL_T:
//         case CHAR_T:
//             data_unit_value_len = 1;
//
//             value_bytes = decimal_to_bytes(*(uint8_t *)du_params->value);
//             break;
//         case INT_T:
//             data_unit_value_len = 4;
//
//             value_bytes = decimal_to_bytes(*(int *)du_params->value);
//             break;
//         default: // for other cases length has to be specified
//         {
//             if (du_params->value_len == NULL)
//                 return NULL;
//             data_unit_value_len = *(size_t *)du_params->value_len;
//             value_bytes = (uint8_t *)du_params->value;
//         }
//         }
//         data_unit_len = data_unit_value_len + DU_MIN_SIZE;
//
//     }
//
//     size_t frame_len = DF_MIN_SIZE + data_unit_len;
//     uint8_t *bytes = (uint8_t *)malloc(sizeof(uint8_t) * frame_len);
//
//     bytes[0] = 0x55;
//     bytes[1] = 0xAA;
//     bytes[2] = ver;
//     bytes[3] = cmd;
//
//     if (du_params->dpid)
//     {
//         bytes[4] = du_params->dpid;
//         bytes[5] = du_params->type;
//         memcpy(bytes + 6, du_params->value_len, 2);
//         memcpy(bytes + 8, value_bytes, data_unit_value_len);
//     }
//     bytes[frame_len - 1] = calculate_bytes_checksum(bytes, frame_len - 1);
//
//     bytes_array->bytes = bytes;
//     bytes_array->len = frame_len;
//     return bytes_array;
// }
