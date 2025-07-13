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
#include <Wire.h>
#include <usb_mouse.h>
#include <usb_keyboard.h>

#include <MPU6050_light.h> //gyro
#include <Adafruit_DRV2605.h> //haptic driver

#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

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
const float ROLL_THRESHOLD = 20.0;
const float ROLL_RESET_MARGIN = ROLL_THRESHOLD - 0.5;  // Hysteresis threshold
// Use the formula 1000000 / set_rate = polling rate to get the polling rate
// Note: this timer is seperate from the USB timer but it effectivly limits the polling rate
// Note: at lower polling rates (1000Hz and lower) it is recommended to use 16 bit x and y values to not hit the 8 bit limit
// USB_mouse_move is by default limited to 8 bit values see my github for a fix
// Note: polling rate can be "properly" limited by setting a divider (8000 / divider = polling rate or 1000 / divider = polling rate depending on interface) 
// This divider can be set in the teensy core files but not sure yet if you can adjust it somewhere during runtime (usb_desc.h -> MOUSE_INTERVAL)
Adafruit_SSD1306 display(128, 64);
MPU6050 mpu(Wire);
Adafruit_DRV2605 drv;

unsigned long timer = 0;
bool rollTriggered = false;





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

void triggerVibration() {
  drv.setWaveform(0, 1);  // Strong click
  drv.setWaveform(1, 0);  // End sequence
  Serial.println("VIBRATE");

  drv.go();
}
// Sets up all the buttons and the sensor
void setup()
{

  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  byte status = mpu.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  while (status != 0) {}

  Serial.println(F("Calculating offsets..."));
  delay(1000);
  mpu.calcOffsets();
  Serial.println("Done!\n");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setRotation(0);
  display.setTextWrap(false);
  display.dim(0);

  drv.begin();
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

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



  //screen  

  mpu.update();
  float angleX = mpu.getAngleX();
  float angleY = mpu.getAngleY();
  float angleZ = mpu.getAngleZ();

  if ((millis() - timer) > 50) {
 
    display.clearDisplay();
    display.setFont(&FreeMono9pt7b);
    display.setTextSize(0);

    
  char buf[16];  // Buffer for formatted output

  // Angle X
  sprintf(buf, "P|X: %6.1f", angleX);  // width = 6 chars, 1 decimal
  display.setCursor(0, 20);
  display.println(buf);
  
  // Angle Y
  sprintf(buf, "R|Y: %6.1f", angleY);
  display.setCursor(0, 40);
  display.println(buf);


  // Angle Z
  sprintf(buf, "Y|Z: %6.1f", angleZ);
  display.setCursor(0, 60);
  display.println(buf);
  
  Serial.print("X : ");
	Serial.print(angleX);
	Serial.print("\tY : ");
	Serial.print(angleY);
	Serial.print("\tZ : ");
	Serial.println(angleZ);

  display.display();
  timer = millis();
  }

  // --- Roll tilt trigger ---
  if (!rollTriggered && abs(angleY) >= ROLL_THRESHOLD) {
    triggerVibration();
    rollTriggered = true;
  } else if (rollTriggered && abs(angleY) <= ROLL_RESET_MARGIN) {
    rollTriggered = false;  // Reset only when back within safe range
  }              




}//end loop
