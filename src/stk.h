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

#include <cstdint>

typedef struct {
    uint8_t dpid;
    uint8_t type;
    uint16_t len;
    uint8_t *value;
} data_unit;

typedef struct
{
    uint16_t    header;
    uint8_t     version;
    uint8_t     command;
    uint16_t    data_len;
    data_unit   *data;
    uint8_t     checksum;
} data_frame;


short u8_to_short(uint8_t *bytes);
int u8_to_int(uint8_t *bytes);
long u8_to_long(uint8_t *bytes);

bool is_frame_valid(data_frame *frame);
void print_frame(data_frame *frame);


