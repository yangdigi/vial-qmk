#define PRINT_ROWS 5
#define PRINT_COLS 15
void ec_matrix_print(void) {
    static uint8_t key_matrix[PRINT_ROWS][PRINT_COLS][2] = {
        {{0x00, 4},{0x01, 4},{0x02, 4},{0x03, 4},{0x04, 4},{0x05, 4},{0x06, 4},{0x07, 4},{0x08, 4},{0x09, 4}, {0x0A, 4},{0x0B, 4},{0x0C, 4},{0x0D, 4},{0x2D, 4}},
        {{0x10, 6},{0x11, 4},{0x12, 4},{0x13, 4},{0x14, 4},{0x15, 4},{0x16, 4},{0x17, 4},{0x18, 4},{0x19, 4}, {0x1A, 4},{0x1B, 4},{0x1C, 4},{0x1D, 6},{0x00, 0}},
        {{0x20, 7},{0x21, 4},{0x22, 4},{0x23, 4},{0x24, 4},{0x25, 4},{0x26, 4},{0x27, 4},{0x28, 4},{0x29, 4}, {0x2A, 4},{0x2B, 4},{0x2C, 9},{0x00, 0},{0x00, 0}},
        {{0x30, 9},{0x00, 0},{0x32, 4},{0x33, 4},{0x34, 4},{0x35, 4},{0x36, 4},{0x37, 4},{0x38, 4},{0x39, 4}, {0x3A, 4},{0x3B, 4},{0x3C, 7},{0x3D, 4},{0x00, 0}},
        {{0x40, 6},{0x41, 4},{0x42, 6},{0x00, 0},{0x00, 0},{0x00, 0},{0x00, 0},{0x47,28},{0x00, 0},{0x00, 0}, {0x00, 0},{0x4B, 6},{0x4C, 4},{0x4D, 6},{0x00, 0}}
    };

    print("\n  ");
    for (uint8_t row = 0; row < PRINT_ROWS; row++) {
        for (uint8_t col = 0; col < PRINT_COLS; col++) {
            uint8_t krow = key_matrix[row][col][0]>>4;
            uint8_t kcol = key_matrix[row][col][0]&0xf;
            uint8_t klen = key_matrix[row][col][1];
            if (klen > 0 ) {
                print("-");
                if (krow >= 8) klen -= 3;
                for (uint8_t i=0; i<klen-1; i++) {
                    if (ec_key_value[krow][kcol] >= EC_APC_VALUE) print("_");
                    else print("-");
                }
            }
        }
        print("-\n  |");
        for (uint8_t col = 0; col < PRINT_COLS; col++) {
            uint8_t krow = key_matrix[row][col][0]>>4;
            uint8_t kcol = key_matrix[row][col][0]&0xf;
            uint8_t klen = key_matrix[row][col][1];
            if (klen > 0) {
                uint8_t klen_l = (klen-4)/2;
                uint8_t klen_r = klen-klen_l-4;
                for (uint8_t i=0; i<klen_l; i++) {
                    print(" ");
                }
                if (krow < 8) {
                    #if (EC_INIT_CHECK_TIMES)
                    if (ec_actuation_point[krow][kcol] < 5) xprintf("-%2d", ec_key_value[krow][kcol]);
                    else
                    #endif
                    xprintf("%3d", ec_key_value[krow][kcol]);
                }
                for (uint8_t i=0; i<klen_r; i++) {
                    print(" ");
                }
                print("|");
            }
        }
        print("\n  ");
    }
    for (uint8_t i=0; i<(PRINT_COLS*4+1); i++) {
        print("-");
    }
    print("\n");
}
