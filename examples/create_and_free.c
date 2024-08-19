#include "../src/simpletuya.h"
#include <stdint.h>
#include <stdio.h>


int main(void) {

    uint8_t bytes[] = {0x00, 0x00, 0x01,
         0x55, 0xAA, 0x03, 0x07, 0x00, 0x05, 0x69, 0x04, 0x00, 0x01, 0x00, 0x7C};
    DataFrame *df = bytes2df(bytes, 15);

    // char *str = frame_to_str(df);

    DataUnit du;
    DataUnitDTO du_dto = {.dpid=0x69, .type=TYPE_INT, .int_value=256};
    init_data_unit(&du, &du_dto);

    DataFrame df2;
    BytesArray raw = {
        .len=5,
        .bytes={0x69, 0x04, 0x00, 0x01, 0x00}
    };
    DataFrameDTO df_dto = {
        .version=0x03,
        .command=CMD_SEND_DATA,
        .data_type=DT_RAW,
        .raw_data=&raw
    };
    init_data_frame(&df2, &df_dto);

    BytesArray arr;
    df2bytes(&arr, &df2);

    char *str = frame_to_str(&df2);


    // char *str = frame_to_str(&df2);
    //
    // printf("%s", str);
    //
    for (int i=0; i<arr.len; i++) {
        printf("%d ", arr.bytes[i]);
    }

    printf("\n%s\n", str);


    return 0;


}
