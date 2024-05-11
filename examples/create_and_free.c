#include "../src/simpletuya.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


int main(void)
{
    uint8_t bytes[] = {
        0x00, 0x00, 0x01,
        0x55, 0xAA, 0x03, 0x07, 0x00, 0x05, 0x69, 0x04, 0x00, 0x01, 0x00, 0x7C
    };

    DataFrame *f = bytes_to_data_frame(bytes, sizeof(bytes) / sizeof(bytes[0]));
    char *frame_str = frame_to_str(f);
    printf("%s\n", frame_str);
    free(frame_str);

    uint8_t *frame_bytes = data_frame_to_bytes(f);
    free_data_frame(f);

    DataFrame *f2 = bytes_to_data_frame(frame_bytes, 12);
    char *frame_str2 = frame_to_str(f2);
    free_data_frame(f2);

    printf("%s\n", frame_str2);
    free(frame_str2);

    uint8_t data[] = {0x02};
    BytesBucket *bucket = create_bucket(0x00, SEND_DATA_C, 0x6A, CHAR_T, data, NULL);
    char *bucket_str = bucket_to_str(bucket);
    printf("%s\n", bucket_str);

    return 0;
}
