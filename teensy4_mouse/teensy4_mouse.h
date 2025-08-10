//Mouse Inputs
#define PMW_CS     10
#define PMW_MOSI   11
#define PMW_MISO   12
#define PMW_SCK    13
#define PMW_MOTION 16
#define PMW_RESET  17

#define MS_0       29
#define MS_1       30
#define MC_NO      14
#define MC_NC      15

#define M1_NO      33
#define M1_NC      34
#define M2_NO      35
#define M2_NC      36
#define M3_NO      37
#define M3_NC      38
#define M4_NO      39
#define M4_NC      40
#define M5_NO      31
#define M5_NC      32

#define FAR_EDGE              0
#define NEAR_EDGE             1
#define RIGHT_EDGE            2 // This will be our G-Shift key
#define LEFT_TIP              3
#define RIGHT_TIP             4
#define LEFT_PUSH_TRIGGER     5
#define RIGHT_PUSH_TRIGGER    6
#define LEFT_PULL_TRIGGER     7
#define RIGHT_PULL_TRIGGER    8
#define FAR_WING              9
#define MID_WING              10
#define NEAR_WING             11
#define FAR_DPI               12
#define MID_DPI               13
#define NEAR_DPI              14
#define GRID_TOP_LEFT         15

// MCP2 (I2C Address 0x22)
#define GRID_LEFT             16
#define GRID_BOT_LEFT         17
#define GRID_BOT              18
#define GRID_BOT_RIGHT        19
#define GRID_RIGHT            20
#define GRID_FAR_TOP          21
#define GRID_FAR_BOT          22
// Unassigned buttons below (23-31)

//#define FAR_EDGE 0            //#define BUTTONa0 0
//#define NEAR_EDGE 1            //#define BUTTONa1 1
//#define RIGHT_EDGE 2            //#define BUTTONa2 2
//
//#define LEFT_TIP 3         //#define BUTTONa3 3
//#define RIGHT_TIP 4            //#define BUTTONa4 4
//
//#define LEFT_PUSH_TRIGGER 5            //#define BUTTONa5 5
//#define RIGHT_PUSH_TRIGGER 6            //#define BUTTONa6 6
//
//#define LEFT_PULL_TRIGGER 7            //#define BUTTONa7 7
//#define RIGHT_PULL_TRIGGER 8           //#define BUTTONa8 8
//
//#define FAR_WING 9            //#define BUTTONa9 9
//#define MID_WING 10           //#define BUTTONa10 10
//#define NEAR_WING 11            //#define BUTTONa11 11
//
//#define FAR_DPI 12            //#define BUTTONa12 12
//#define MID_DPI 13            //#define BUTTONa13 13
//#define NEAR_DPI 14            //#define BUTTONa14 14
//
//#define GRID_TOP_LEFT 15            //#define BUTTONa15 15       
//#define GRID_LEFT 16            //#define BUTTONb0 16
//#define GRID_BOT_LEFT 17            //#define BUTTONb1 17
//#define GRID_BOT 18            //#define BUTTONb2 18
//#define GRID_BOT_RIGHT 19            //#define BUTTONb3 19
//#define GRID_RIGHT 20            //#define BUTTONb4 20
//#define GRID_FAR_TOP 21            //#define BUTTONb5 21
//#define GRID_FAR_BOT 22            //#define BUTTONb6 22
            //#define BUTTONb7 23
            //#define BUTTONb8 24
            //#define BUTTONb9 25
            //#define BUTTONb10 26
            //#define BUTTONb11 27
            //#define BUTTONb12 28
            //#define BUTTONb13 29
            //#define BUTTONb14 30
            //#define BUTTONb15 31

//PMW_3360 registers
#define REG_Product_ID                 0x00
#define REG_Revision_ID                0x01
#define REG_Motion                     0x02
#define REG_Delta_X_L                  0x03
#define REG_Delta_X_H                  0x04
#define REG_Delta_Y_L                  0x05
#define REG_Delta_Y_H                  0x06
#define REG_SQUAL                      0x07
#define REG_Raw_Data_Sum               0x08
#define REG_Maximum_Raw_data           0x09
#define REG_Minimum_Raw_data           0x0A
#define REG_Shutter_Lower              0x0B
#define REG_Shutter_Upper              0x0C
#define REG_Control                    0x0D
#define REG_Config1                    0x0F
#define REG_Config2                    0x10
#define REG_Angle_Tune                 0x11
#define REG_Frame_Capture              0x12
#define REG_SROM_Enable                0x13
#define REG_Run_Downshift              0x14
#define REG_Rest1_Rate_Lower           0x15
#define REG_Rest1_Rate_Upper           0x16
#define REG_Rest1_Downshift            0x17
#define REG_Rest2_Rate_Lower           0x18
#define REG_Rest2_Rate_Upper           0x19
#define REG_Rest2_Downshift            0x1A
#define REG_Rest3_Rate_Lower           0x1B
#define REG_Rest3_Rate_Upper           0x1C
#define REG_Observation                0x24
#define REG_Data_Out_Lower             0x25
#define REG_Data_Out_Upper             0x26
#define REG_Raw_Data_Dump              0x29
#define REG_SROM_ID                    0x2A
#define REG_Min_SQ_Run                 0x2B
#define REG_Raw_Data_Threshold         0x2C
#define REG_Config5                    0x2F
#define REG_Power_Up_Reset             0x3A
#define REG_Shutdown                   0x3B
#define REG_Inverse_Product_ID         0x3F
#define REG_LiftCutoff_Tune3           0x41
#define REG_Angle_Snap                 0x42
#define REG_LiftCutoff_Tune1           0x4A
#define REG_Motion_Burst               0x50
#define REG_LiftCutoff_Tune_Timeout    0x58
#define REG_LiftCutoff_Tune_Min_Length 0x5A
#define REG_SROM_Load_Burst            0x62
#define REG_Lift_Config                0x63
#define REG_Raw_Data_Burst             0x64
#define REG_LiftCutoff_Tune2           0x65

// Wait times for SPI transactions according to PMW3360 datasheet
#define TSRAD_MOTION_BURST             35
#define TSWW                           180
#define TSRAD                          160
#define TSRW                           20
#define TSRR                           20
#define TLOAD                          15

void     write_reg_PMW(uint8_t reg, uint8_t value);
uint8_t  read_reg_PMW(uint8_t reg);
void     upload_byte(uint8_t value);
void     upload_firmware();
uint32_t begin_PMW();
uint32_t check_signature();
void     set_CPI(uint16_t cpi);
void     read_burst_start();
void     read_burst_end();
void     update_scroll();
void     update_buttons();
void     update_dpi();
void     update_usb();

extern const char* const buttonNames[32];

