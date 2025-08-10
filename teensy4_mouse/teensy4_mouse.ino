/*
Work In Progress

    Teensy4_mouse.ino - minimal firmware for an 8khz mouse written in arduino.
    Requires a teensy4.1 MCU and a mouse with the PMW3389 sensor.
    Use the Serial + Keyboard + Mouse + Joystick USB type.
    (part of the code inspired by https://github.com/SunjunKim/PMW3360, https://github.com/mrjohnk/PMW3360DM-T2QU and https://github.com/Trip93/teensy4_mouse)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "teensy4_mouse.h"
#include "srom_3389.h"
#include <SPI.h>
// MERGED: Include the Adafruit MCP23X17 library
#include <Adafruit_MCP23X17.h>

// PMW3360 datasheet mentions a max of 2MHz for the SPI clock however up to 36Mhz seems to work fine for me
// Very small benefit increasing this speed beyond 2MHz to decrease transaction times sensor FPS is still limited to 12000 FPS
#define SPI_BUS_SPEED     2000000
#define SPI_SETTINGS_PMW  SPISettings      (SPI_BUS_SPEED, MSBFIRST, SPI_MODE3)
#define ACTIVATE_CS_PMW   digitalWriteFast (PMW_CS,        LOW); delayMicroseconds(1)
#define DEACTIVATE_CS_PMW delayMicroseconds(1);                  digitalWriteFast (PMW_CS, HIGH)
#define RESET_PMW         digitalWriteFast (PMW_RESET,     LOW); delayMicroseconds(1); digitalWriteFast(PMW_RESET, HIGH); delayMicroseconds(1)
#define RESET_SPI         digitalWriteFast (PMW_CS,        LOW); delayMicroseconds(1); digitalWriteFast(PMW_CS,    HIGH); delayMicroseconds(1)

// 1 write to burst register -> 35 uS wait time -> 12 reads from burst register + 5 uS margin (at 2Mhz this is 92 uS at 36Mhz this is 43 uS)
#define BURST_READ_TIME (TSRAD_MOTION_BURST + 5) + 8 * (1000000.0 / (SPI_BUS_SPEED)) * (12 + 1)


const char* const buttonNames[32] PROGMEM = {
"FAR_EDGE"            ,
"NEAR_EDGE"           ,
"RIGHT_EDGE"          ,
"LEFT_TIP"            ,
"RIGHT_TIP"           ,
"LEFT_PUSH_TRIGGER"   ,
"RIGHT_PUSH_TRIGGER"  ,
"LEFT_PULL_TRIGGER"   ,
"RIGHT_PULL_TRIGGER"  ,
"FAR_WING"            ,
"MID_WING"            ,
"NEAR_WING"           ,
"FAR_DPI"             ,
"MID_DPI"             ,
"NEAR_DPI"            ,
"GRID_TOP_LEFT"       ,
"GRID_LEFT"           ,
"GRID_BOT_LEFT"       ,
"GRID_BOT"            ,
"GRID_BOT_RIGHT"      ,
"GRID_RIGHT"          ,
"GRID_FAR_TOP"        ,
"GRID_FAR_BOT"        ,
"UNUSED_23"           ,
"UNUSED_24"           ,
"UNUSED_25"           ,
"UNUSED_26"           ,
"UNUSED_27"           ,
"UNUSED_28"           ,
"UNUSED_29"           ,
"UNUSED_30"           , 
"UNUSED_31"
};



static elapsedMicros burst_timer  = 0;
static elapsedMicros polling_rate = 0;
static elapsedMillis scroll_timer = 0;
static elapsedMillis button_timer = 0;

static int16_t  x                       = 0;
static int16_t  y                       = 0;
static uint8_t  scroll                  = 0;
static int8_t   buttons                 = 0;
static int8_t   buttons_old             = 0;
static uint16_t cpi[6]                  = {400, 800, 1600, 2000, 3200, 6400};
static uint8_t  cpi_index               = 5;
static bool     cpi_pressed             = false;
static uint16_t set_rate                = 125;
// Use the formula 1000000 / set_rate = polling rate to get the polling rate
// Note: this timer is seperate from the USB timer but it effectivly limits the polling rate
// Note: at lower polling rates (1000Hz and lower) it is recommended to use 16 bit x and y values to not hit the 8 bit limit
// USB_mouse_move is by default limited to 8 bit values see my github for a fix
// Note: polling rate can be "properly" limited by setting a divider (8000 / divider = polling rate or 1000 / divider = polling rate depending on interface)
// This divider can be set in the teensy core files but not sure yet if you can adjust it somewhere during runtime (usb_desc.h -> MOUSE_INTERVAL)

// MERGED: Create two Adafruit_MCP23X17 objects
Adafruit_MCP23X17 mcp1;
Adafruit_MCP23X17 mcp2;
static bool g_shift_active = false; // Tracks the state of our G-Shift modifier key

// Writes to a register on the PMW3360
void write_reg_PMW(uint8_t reg, uint8_t value)
{
  reg               |= 0b10000000;
  uint16_t transfer  = (reg << 8) | value;

  ACTIVATE_CS_PMW;
  SPI.transfer16(transfer);
  DEACTIVATE_CS_PMW;

  delayMicroseconds(TSWW);
}

// Reads out a register on the PMW3360
uint8_t read_reg_PMW(uint8_t reg)
{
  reg &= 0b01111111;

  ACTIVATE_CS_PMW;
  SPI.transfer     (reg);
  delayMicroseconds(TSRAD);
  uint8_t value = SPI.transfer(0);
  DEACTIVATE_CS_PMW;

  delayMicroseconds(TSRR);

  return value;
}

// Helper function for burst load uploads 1 byte to a preset address
void upload_byte(uint8_t value)
{
  SPI.transfer     (value);
  delayMicroseconds(TLOAD);
}

// Uploads a binary firmware file over SPI to the PMW3360
void upload_firmware()
{
  // Prepare to upload the binary file
  write_reg_PMW(REG_Config2, 0x00);
  write_reg_PMW(REG_SROM_Enable, 0x1d);
  delay        (10);                    // From datasheet
  write_reg_PMW(REG_SROM_Enable, 0x18);

  // Upload the binary file here
  ACTIVATE_CS_PMW;
  upload_byte(REG_SROM_Load_Burst | 0x80);
  for        (uint32_t index = 0; index < 4094; index++) upload_byte(firmware_data[index]);
  DEACTIVATE_CS_PMW;

  read_reg_PMW (REG_SROM_ID);
  write_reg_PMW(REG_Config2, 0x00);
}

// Initializes the PMW3360
uint32_t begin_PMW()
{
  pinMode(PMW_CS   , OUTPUT);
  pinMode(PMW_RESET, OUTPUT);

  SPI.begin();
  RESET_SPI;
  RESET_PMW;
  delay    (50);

  SPI.beginTransaction(SPI_SETTINGS_PMW);
  Serial.println      ("Starting firmware upload");
  upload_firmware     ();
  SPI.endTransaction  ();
  delay               (10);

  return check_signature();
}

// Checks if the firmware was correctly uploaded
uint32_t check_signature()
{
  SPI.beginTransaction(SPI_SETTINGS_PMW);

  uint8_t pid      = read_reg_PMW(REG_Product_ID);
  uint8_t iv_pid   = read_reg_PMW(REG_Inverse_Product_ID);
  uint8_t SROM_ver = read_reg_PMW(REG_SROM_ID);

  SPI.endTransaction();

  return (pid == 0x47 && iv_pid == 0xB8 && SROM_ver == 0xE8); // signature for SROM 0x04
}

// Sets the CPI/DPI value of the PMW3360
void set_CPI(uint16_t cpi)
{
  // Limits cpi to 100 - 12000 effectivly with steps of a 100
  cpi = constrain((cpi / 100) - 1, 0, 119);
  write_reg_PMW  (REG_Config1,     cpi);
}

/*
  Burst report according to PMW3360 datasheet
  BYTE[00] = Motion
  BYTE[01] = Observation
  BYTE[02] = Delta_X_L
  BYTE[03] = Delta_X_H
  BYTE[04] = Delta_Y_L
  BYTE[05] = Delta_Y_H
  BYTE[06] = SQUAL
  BYTE[07] = Raw_Data_Sum
  BYTE[08] = Maximum_Raw_Data
  BYTE[09] = Minimum_Raw_Data
  BYTE[10] = Shutter_Upper
  BYTE[11] = Shutter_Lower
*/

// Activates motion burst from the PMW3360
void read_burst_start()
{
  ACTIVATE_CS_PMW;
  SPI.transfer(REG_Motion_Burst);
  burst_timer = 0;
}

// Updates mouse x and y axis
void read_burst_end()
{
  int8_t burst[12] = {0};
  //read out the data send by the PMW3360 only the first 6 are nessecary so last 6 can be skipped
  SPI.transfer(burst, 12);
  DEACTIVATE_CS_PMW;

  x += ((burst[3] << 8) | burst[2]);
  y += ((burst[5] << 8) | burst[4]);
}

// Updates the scroll movement
void update_scroll()
{
  // Important to put some time between samples
  if (scroll_timer > 0)
  {
    //get new sample to compare with the old sample 0b0000NNOO
    (!digitalReadFast(MS_1)) ? scroll |= 0b00000100 : scroll &= 0b11111011;
    (!digitalReadFast(MS_0)) ? scroll |= 0b00001000 : scroll &= 0b11110111;

    // Grey code coming from the encoder
    if      ((scroll >> 2         ) == (scroll  & 0b00000011))                                                     buttons &= 0b00111111;              // No scrolling if old sample is equal to new sample
    else if ((scroll == 0b00000100) || (scroll == 0b00001101) || (scroll == 0b00001011) || (scroll == 0b00000010)) buttons |= 0b10000000 & 0b10111111; // Scroll up   00 -> 01 -> 11 -> 10 -> 00
    else if ((scroll == 0b00001000) || (scroll == 0b00001110) || (scroll == 0b00000111) || (scroll == 0b00000001)) buttons |= 0b01000000 & 0b01111111; // Scroll down 00 -> 10 -> 11 -> 01 -> 00

    //save new sample so it can be used in the next loop
    scroll       >>= 2;
    scroll_timer   = 0;
  }
}

// Updates the clickable buttons on the mouse
void update_buttons()
{
  // Update main mouse buttons at highest possible speed
  if (!digitalReadFast(M1_NO) && digitalReadFast(M1_NC)) buttons |= 0b00000001;
  if (!digitalReadFast(M1_NC) && digitalReadFast(M1_NO)) buttons &= 0b11111110;
  if (!digitalReadFast(M2_NO) && digitalReadFast(M2_NC)) buttons |= 0b00000010;
  if (!digitalReadFast(M2_NC) && digitalReadFast(M2_NO)) buttons &= 0b11111101;
  if (!digitalReadFast(M4_NO) && digitalReadFast(M4_NC)) buttons |= 0b00001000;
  if (!digitalReadFast(M4_NC) && digitalReadFast(M4_NO)) buttons &= 0b11110111;
  if (!digitalReadFast(M5_NO) && digitalReadFast(M5_NC)) buttons |= 0b00010000;
  if (!digitalReadFast(M5_NC) && digitalReadFast(M5_NO)) buttons &= 0b11101111;

  // Some browser functionallity bugs out when update rate of middle mouse button exceeds a 1000hz (page drag with middle mouse button)
  // No point updating the cpi button beyond a 1000hz
  if (button_timer > 0)
  {
    button_timer = 0;
    if (!digitalReadFast(M3_NO) && digitalReadFast(M3_NC)) buttons |= 0b00000100;
    if (!digitalReadFast(M3_NC) && digitalReadFast(M3_NO)) buttons &= 0b11111011;
    if (!digitalReadFast(MC_NO) && digitalReadFast(MC_NC)) buttons |= 0b00100000;
    if (!digitalReadFast(MC_NC) && digitalReadFast(MC_NO)) buttons &= 0b11011111;
  }
}

// updates cpi avoid using this function within SPI transactions like motion burst
void update_cpi()
{
  // Update cpi on release
  if (  buttons & 0b00100000) cpi_pressed = true;
  if (!(buttons & 0b00100000) && cpi_pressed)
  {
    cpi_pressed = false;
    cpi_index < 5 ? cpi_index++ : cpi_index = 0;
    SPI.beginTransaction(SPI_SETTINGS_PMW);
    set_CPI             (cpi[cpi_index]);
    //start motion burst mode again
    write_reg_PMW       (REG_Motion_Burst, 0x00);
    SPI.endTransaction  ();
  }
}

// Passes the button, scroll and mouse movment information to the USB bus for the next poll
void update_usb()
{
  int8_t scroll           = (buttons & 0b10000000) ? scroll =  1 : (buttons & 0b01000000) ? scroll = -1 : scroll = 0;
  usb_mouse_buttons_state =  buttons;
  buttons_old             =  buttons;
  // This function is blocking if its buffers fill up and blocks until the next USB poll (125uS at USB 2.0 HIGH speed and 1mS below that speed)
  // usb_mouse_move only accepts 8 bit data by default it still works with 8 bit values but at lower polling speeds (1000Hz and lower)
  // maximum control speed is limited due to the integers overflowing. Recommended to make this function 16 bit for lower polling speeds (see my github)
  usb_mouse_move(x, y, scroll, 0);
  x = 0;
  y = 0;
}


// MERGED: Function to read buttons from both MCP23017 expanders
// This function now detects both press and release events and handles G-Shift state changes correctly.
// MERGED: Function to read buttons from both MCP23017 expanders
// This version implements true modifier behavior for the G-Shift key.
void update_mcp_buttons() {
    static uint32_t previous_states = 0xFFFFFFFF;
    static uint32_t g_shift_press_mask = 0; // Remembers the G-Shift state for each key at the moment it was pressed.
    uint32_t current_states = 0;

    // Read the current state of all 32 buttons efficiently
    uint16_t states_mcp1 = mcp1.readGPIOAB();
    uint16_t states_mcp2 = mcp2.readGPIOAB();
    current_states = states_mcp1 | (uint32_t)states_mcp2 << 16;

    // --- First, handle the G-Shift key itself to update the global state ---
    // This ensures its state is known before processing other keys in the same loop.
    bool g_shift_current_state = !((current_states >> RIGHT_EDGE) & 1); // is G-Shift currently pressed?
    if (g_shift_current_state) {
        if (!g_shift_active) {
            g_shift_active = true;
            Serial.println("G-SHIFT ON");
        }
    } else {
        if (g_shift_active) {
            g_shift_active = false;
            Serial.println("G-SHIFT OFF");
        }
    }

    // --- Now, iterate through all buttons to handle their events ---
    for (uint8_t i = 0; i < 32; i++) {
        // Skip the G-Shift button itself, as we've already handled its state change.
        if (i == RIGHT_EDGE) continue;

        bool is_pressed = !((current_states >> i) & 1);
        bool was_pressed = !((previous_states >> i) & 1);

        // --- Key Press Event (was released, is now pressed) ---
        if (is_pressed && !was_pressed) {
            Serial.print(buttonNames[i]);
            Serial.println(" pressed");

            // Store the G-Shift state FOR THIS KEY at the moment of pressing.
            if (g_shift_active) {
                g_shift_press_mask |= (1UL << i);
            } else {
                g_shift_press_mask &= ~(1UL << i);
            }

            // A single switch handles all press actions.
            // The G-Shift logic is handled by the ternary operator: (condition ? value_if_true : value_if_false)
            switch (i) {
                // --- MACROS ---
                case FAR_EDGE: Keyboard.press(MODIFIERKEY_CTRL); Keyboard.press(KEY_C); break;
                case NEAR_EDGE: Keyboard.press(MODIFIERKEY_CTRL); Keyboard.press(KEY_V); break;

                // --- MODIFIER KEYS (excluding G-Shift) ---
                case RIGHT_TIP: Keyboard.press(MODIFIERKEY_CTRL); break;
                case LEFT_PUSH_TRIGGER: Keyboard.press(MODIFIERKEY_ALT); break;
                case RIGHT_PUSH_TRIGGER: Keyboard.press(MODIFIERKEY_SHIFT); break;

                // --- SINGLE KEY PRESSES ---
                case LEFT_TIP: Keyboard.press(KEY_ENTER); break;
                case LEFT_PULL_TRIGGER: Keyboard.press(KEY_BACKSPACE); break;
                case RIGHT_PULL_TRIGGER: Keyboard.press(KEY_DELETE); break;
                case NEAR_DPI: Keyboard.press(KEY_PRINTSCREEN); break;
                case GRID_TOP_LEFT: Keyboard.press(KEY_ENTER); break;

                // --- Layered Keys ---
                case FAR_WING:      Keyboard.press(g_shift_active ? KEY_1 : KEY_A); break;
                case MID_WING:      Keyboard.press(g_shift_active ? KEY_2 : KEY_B); break;
                case NEAR_WING:     Keyboard.press(g_shift_active ? KEY_3 : KEY_C); break;
                case FAR_DPI:       Keyboard.press(g_shift_active ? KEY_4 : KEY_D); break;
                case MID_DPI:       Keyboard.press(g_shift_active ? KEY_5 : KEY_E); break;
                case GRID_LEFT:     Keyboard.press(g_shift_active ? KEY_6 : KEY_F); break;
                case GRID_BOT_LEFT: Keyboard.press(g_shift_active ? KEY_7 : KEY_G); break;
                case GRID_BOT:      Keyboard.press(g_shift_active ? KEY_8 : KEY_H); break;
                case GRID_BOT_RIGHT:Keyboard.press(g_shift_active ? KEY_9 : KEY_I); break;
                case GRID_RIGHT:    Keyboard.press(g_shift_active ? KEY_0 : KEY_J); break;
                case GRID_FAR_TOP:  Keyboard.press(g_shift_active ? KEY_MINUS : KEY_K); break;
                case GRID_FAR_BOT:  Keyboard.press(g_shift_active ? KEY_EQUAL : KEY_L); break;
            }
        }
        // --- Key Release Event (was pressed, is now released) ---
        else if (!is_pressed && was_pressed) {
            Serial.print(buttonNames[i]);
            Serial.println(" released");

            // Check which layer the key was on WHEN IT WAS PRESSED by consulting our mask.
            bool was_g_shifted_on_press = (g_shift_press_mask >> i) & 1;

            // A single switch handles all release actions.
            // The release action is based on the saved state, not the current one.
            switch (i) {
                // --- MACROS (release in reverse order) ---
                case FAR_EDGE: Keyboard.release(KEY_C); Keyboard.release(MODIFIERKEY_CTRL); break;
                case NEAR_EDGE: Keyboard.release(KEY_V); Keyboard.release(MODIFIERKEY_CTRL); break;

                // --- MODIFIER KEYS (excluding G-Shift) ---
                case RIGHT_TIP: Keyboard.release(MODIFIERKEY_CTRL); break;
                case LEFT_PUSH_TRIGGER: Keyboard.release(MODIFIERKEY_ALT); break;
                case RIGHT_PUSH_TRIGGER: Keyboard.release(MODIFIERKEY_SHIFT); break;

                // --- SINGLE KEY PRESSES ---
                case LEFT_TIP: Keyboard.release(KEY_ENTER); break;
                case LEFT_PULL_TRIGGER: Keyboard.release(KEY_BACKSPACE); break;
                case RIGHT_PULL_TRIGGER: Keyboard.release(KEY_DELETE); break;
                case NEAR_DPI: Keyboard.release(KEY_PRINTSCREEN); break;
                case GRID_TOP_LEFT: Keyboard.release(KEY_ENTER); break;
                
                // --- Layered Keys ---
                case FAR_WING:      Keyboard.release(was_g_shifted_on_press ? KEY_1 : KEY_A); break;
                case MID_WING:      Keyboard.release(was_g_shifted_on_press ? KEY_2 : KEY_B); break;
                case NEAR_WING:     Keyboard.release(was_g_shifted_on_press ? KEY_3 : KEY_C); break;
                case FAR_DPI:       Keyboard.release(was_g_shifted_on_press ? KEY_4 : KEY_D); break;
                case MID_DPI:       Keyboard.release(was_g_shifted_on_press ? KEY_5 : KEY_E); break;
                case GRID_LEFT:     Keyboard.release(was_g_shifted_on_press ? KEY_6 : KEY_F); break;
                case GRID_BOT_LEFT: Keyboard.release(was_g_shifted_on_press ? KEY_7 : KEY_G); break;
                case GRID_BOT:      Keyboard.release(was_g_shifted_on_press ? KEY_8 : KEY_H); break;
                case GRID_BOT_RIGHT:Keyboard.release(was_g_shifted_on_press ? KEY_9 : KEY_I); break;
                case GRID_RIGHT:    Keyboard.release(was_g_shifted_on_press ? KEY_0 : KEY_J); break;
                case GRID_FAR_TOP:  Keyboard.release(was_g_shifted_on_press ? KEY_MINUS : KEY_K); break;
                case GRID_FAR_BOT:  Keyboard.release(was_g_shifted_on_press ? KEY_EQUAL : KEY_L); break;
            }
        }
    }

    // Save the current states for the next loop iteration
    previous_states = current_states;
}


// Sets up all the buttons and the sensor
void setup()
{
  // Mouse buttons
  pinMode(M1_NO, INPUT_PULLUP);
  pinMode(M1_NC, INPUT_PULLUP);
  pinMode(M2_NO, INPUT_PULLUP);
  pinMode(M2_NC, INPUT_PULLUP);
  pinMode(M3_NO, INPUT_PULLUP);
  pinMode(M3_NC, INPUT_PULLUP);
  pinMode(M4_NO, INPUT_PULLUP);
  pinMode(M4_NC, INPUT_PULLUP);
  pinMode(M4_NO, INPUT_PULLUP);
  pinMode(M4_NC, INPUT_PULLUP);
  pinMode(M5_NO, INPUT_PULLUP);
  pinMode(M5_NC, INPUT_PULLUP);
  pinMode(MC_NO, INPUT_PULLUP);
  pinMode(MC_NC, INPUT_PULLUP);

  // Scroll wheel
  pinMode(MS_0, INPUT_PULLUP);
  pinMode(MS_1, INPUT_PULLUP);

  Serial.begin(115200);
  //while(!Serial); // Optional: wait for serial monitor to open

  // MERGED: Initialize the MCP23017s
  Serial.println("MCP23xxx Button Test!");
  if (!mcp1.begin_I2C(0x20)) {
    Serial.println("Error with MCP1.");
    while (1);
  }
   if (!mcp2.begin_I2C(0x22)) {
    Serial.println("Error with MCP2.");
    while (1);
  }

  // MERGED: Configure all pins on both MCPs as inputs with pull-up resistors
  for (uint8_t i = 0; i < 16; i++) {
    mcp1.pinMode(i, INPUT_PULLUP);
  }
  for (uint8_t i = 0; i < 16; i++) {
    mcp2.pinMode(i, INPUT_PULLUP);
  }
  Serial.println("MCP INIT DONE");


  Serial.println("Starting PMW");
  while (!begin_PMW()) Serial.println("Problem starting PMW");
  Serial.println("PMW succesfully started");

  Serial.println(BURST_READ_TIME);

  SPI.beginTransaction(SPI_SETTINGS_PMW);

  //set cpi then start burst mode after
  write_reg_PMW(REG_Lift_Config , 0b00000011);
  set_CPI      (6400);
  write_reg_PMW(REG_Motion_Burst, 0x00);
  SPI.endTransaction();
}

// Gathers mouse data and sends it over the USB
void loop()
{
  // MERGED: Call the function to read buttons from the MCP expanders
  update_mcp_buttons();

  //check if there is new motion data from interrupt pin
  if (!digitalReadFast(PMW_MOTION))
  {
    // If there is enough time left or if no motion data is read yet read the motion register
    while (polling_rate < (set_rate - BURST_READ_TIME) || (!x && !y))
    {
      SPI.beginTransaction(SPI_SETTINGS_PMW);
      read_burst_start();
      // Burst data only available after 35 microseconds update other things in the mean time
      while (burst_timer < TSRAD_MOTION_BURST)
      {
        update_buttons();
        update_scroll();
      }
      read_burst_end();
      SPI.endTransaction();
    }
    update_usb();
    polling_rate = 0;
  }
  // If there is no motion data only update the buttons and scroll wheel
  else
  {
    update_cpi();
    update_buttons();
    update_scroll();
    if (buttons != buttons_old && (polling_rate >= set_rate))
    {
      update_usb();
      polling_rate = 0;
    }
  }
}
