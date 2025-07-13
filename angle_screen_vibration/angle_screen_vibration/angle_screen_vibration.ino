#include <Wire.h>
#include <usb_mouse.h>
#include <usb_keyboard.h>

#include <MPU6050_light.h> //gyro
#include <Adafruit_DRV2605.h> //haptic driver

#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

const float ROLL_THRESHOLD = 20.0;
const float ROLL_RESET_MARGIN = ROLL_THRESHOLD - 0.5;  // Hysteresis threshold

Adafruit_SSD1306 display(128, 64);
MPU6050 mpu(Wire);
Adafruit_DRV2605 drv;

unsigned long timer = 0;
bool rollTriggered = false; // Track if roll vibration has already been triggered

void setup() {


  Serial.begin(115200);
  Wire.begin();


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
}

void loop() {


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

 


void triggerVibration() {
  drv.setWaveform(0, 1);  // Strong click
  drv.setWaveform(1, 0);  // End sequence
  Serial.println("VIBRATE");

  drv.go();
}
