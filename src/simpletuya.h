#ifndef SIMPLETUYA_H
#define SIMPLETUYA_H

#include <stdint.h>
#include <stdbool.h>

#ifndef size_t
typedef unsigned long size_t;
#endif


#define DF_MIN_SIZE 7
#define DU_MIN_SIZE 4


typedef enum {
  RAW_T     = 0x00, // N bytes
  BOOL_T    = 0x01, // 1 bytes
  INT_T     = 0x02, // 4 bytes
  STR_T     = 0x03, // N bytes
  CHAR_T    = 0x04, // 1 bytes
  BITMAP_T  = 0x05  // 1/2/4 bytes
} DataType;


typedef enum  {
  // module commands
  HBEARTBEAT_C     = 0x00,
  GET_INFO_C       = 0x01,
  WORKING_MODE_C   = 0x02,
  NETWORK_STATUS_C = 0x03,
  SEND_DATA_C      = 0x06,

  // MCU commands
  RESET_MODULE_C   = 0x04,
  QUERY_DATA_C     = 0x07,
} Command;


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


typedef struct _BytesBucket
{
    uint8_t *bytes;
    size_t data_len;

} BytesBucket;


uint8_t *u16_to_bytes(uint16_t);
uint8_t calculate_bytes_checksum(uint8_t*, size_t);
bool is_frame_valid(DataFrame*);

char *bucket_to_str(BytesBucket*);
char *frame_to_str(DataFrame*);

void free_data_frame(DataFrame*);
void free_data_unit(DataUnit*);
void free_bytes_bucket(BytesBucket*);

DataFrame *bytes_to_data_frame(uint8_t*, size_t);
DataUnit *bytes_to_data_unit(uint8_t*, size_t);

uint8_t *data_unit_to_bytes(DataUnit*);
uint8_t *data_frame_to_bytes(DataFrame*);
BytesBucket *create_bucket(uint8_t, Command, uint8_t, DataType, void*, uint16_t*);


#define bytes_to_decimal(T, bytes)                                                                 \
({                                                                                                 \
    T value = 0;                                                                                   \
    size_t j = sizeof(T);                                                                          \
    for (unsigned long i = 0; i < sizeof(T); i++)                                                  \
    {                                                                                              \
        value |= (T)bytes[--j] << (8 * i);                                                         \
    }                                                                                              \
    value;                                                                                         \
})


#define decimal_to_bytes(T, value)                                                                 \
({                                                                                                 \
    uint8_t *bytes = (uint8_t *)malloc(sizeof(T));                                                 \
    for (unsigned long i=0; i<sizeof(T); i++)                                                      \
    {                                                                                              \
        bytes[i] = (value >> ((sizeof(T)-1) * 8)) & 0xFF;                                          \
    }                                                                                              \
    bytes;                                                                                         \
})


#endif


