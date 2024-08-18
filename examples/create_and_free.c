#include "../src/simpletuya.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// int main(void)
// {
//     uint8_t bytes[] = {
//         0x00, 0x00, 0x01,
//         0x55, 0xAA, 0x03, 0x07, 0x00, 0x05, 0x69, 0x04, 0x00, 0x01, 0x00, 0x7C};
//
//     DataFrame *f = bytes_to_data_frame(bytes, sizeof(bytes) / sizeof(bytes[0]));
//     char *frame_str = frame_to_str(f);
//     printf("%s\n", frame_str);
//     free(frame_str);
//
//     uint8_t *frame_bytes = data_frame_to_bytes(f);
//     free_data_frame(f);
//
//     DataFrame *f2 = bytes_to_data_frame(frame_bytes, 12);
//     char *frame_str2 = frame_to_str(f2);
//     free_data_frame(f2);
//
//     printf("%s\n", frame_str2);
//     free(frame_str2);
//
//     uint8_t data[] = {0x02};
//     BucketConfig config = {0x6A, CHAR_T, data, NULL};
//     BytesBucket *bucket = create_bucket(0x00, SEND_DATA_C, &config);
//     char *bucket_str = bucket_to_str(bucket);
//     printf("%s\n", bucket_str);
//
//     return 0;
// }


int main(void) {
    // DataFrame frame;
    // DataUnit data_unit = {
    //     .dpid=0x69,
    //     .type=TYPE_INT,
    //     .value_len=4,
    //     .value=4
    // };
    // DataFrameDTO frame_dto = {
    //     .ver=0x03,
    //     .cmd=CMD_QUERY_DATA,
    //     .data_type=DT_UNIT,
    //     .du=&data_unit,
    //     .raw_data=NULL
    // };
    // init_data_frame(&frame, &frame_dto);

    uint8_t bytes[] = {0x00, 0x00, 0x01,
         0x55, 0xAA, 0x03, 0x07, 0x00, 0x05, 0x69, 0x04, 0x00, 0x01, 0x00, 0x7C};
    DataFrame *df = bytes2df(bytes, 15);

    // char *str = frame_to_str(df);

    DataUnit du;
    DataUnitDTO du_dto = {.dpid=0x69, .type=TYPE_INT, .int_value=256};
    init_data_unit(&du, &du_dto);

    DataFrame df2;
    DataFrameDTO df_dto = {
        .version=0x03,
        .command=CMD_QUERY_DATA,
        .data_type=DT_UNIT,
        .data_unit=&du
    };
    init_data_frame(&df2, &df_dto);

    BytesArray arr;
    df2bytes(&arr, &df2);

    // char *str = frame_to_str(&df2);
    //
    // printf("%s", str);
    //
    for (int i=0; i<arr.len; i++) {
        printf("%d ", arr.bytes[i]);
    }
    printf("\n %lu \n", arr.len);


    return 0;


}
