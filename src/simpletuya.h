#ifndef SIMPLETUYA_H
#define SIMPLETUYA_H

#include <stdint.h>
#include <stdbool.h>

#ifndef size_t
typedef unsigned long size_t;
#endif

enum DataType {
  RAW_T     = 0x00, // N bytes
  BOOL_T    = 0x01, // 1 bytes
  INT_T     = 0x02, // 4 bytes
  STR_T     = 0x03, // N bytes
  CHAR_T    = 0x04, // 1 bytes
  BITMAP_T  = 0x05  // 1/2/4 bytes
};

enum Commands {
  // module commands
  HBEARTBEAT_C     = 0x00,
  GET_INFO_C       = 0x01,
  WORKING_MODE_C   = 0x02,
  NETWORK_STATUS_C = 0x03,
  SEND_DATA_C      = 0x06,

  // MCU commands
  RESET_MODULE_C   = 0x04,
  QUERY_DATA_C     = 0x07,
};

typedef struct _DataUnit
{
    uint8_t dpid;
    uint8_t type;
    uint8_t len[2];
    uint8_t *value;
} DataUnit;

typedef struct _DataFrame
{
    uint8_t    header[2];
    uint8_t    version;
    uint8_t    command;
    uint8_t    data_len[2];
    DataUnit  *data_unit;
    uint8_t    checksum;
} DataFrame;


bool is_frame_valid(DataFrame*);
char *frame_to_str(DataFrame*);

void free_data_frame(DataFrame*);
void free_data_unit(DataUnit*);

DataFrame *bytes_to_data_frame(uint8_t*, size_t);
DataUnit *bytes_to_data_unit(uint8_t*, size_t);

uint8_t *data_unit_to_bytes(DataUnit*);
uint8_t *data_frame_to_bytes(DataFrame*);


#define bytes_to_decimal(T, bytes)                      \
({                                                      \
    T value = 0;                                        \
    int j = sizeof(T);                                  \
    for (unsigned long i = 0; i < sizeof(T); i++)       \
    {                                                   \
        value |= (T)bytes[--j] << (8 * i);              \
    }                                                   \
    value;                                              \
})

#endif


