
#include <Adafruit_MCP23X17.h>


#define BUTTONa0 0
#define BUTTONa1 1
#define BUTTONa2 2
#define BUTTONa3 3
#define BUTTONa4 4
#define BUTTONa5 5
#define BUTTONa6 6
#define BUTTONa7 7
#define BUTTONa8 8
#define BUTTONa9 9
#define BUTTONa10 10
#define BUTTONa11 11
#define BUTTONa12 12
#define BUTTONa13 13
#define BUTTONa14 14
#define BUTTONa15 15

#define BUTTONb0 16
#define BUTTONb1 17
#define BUTTONb2 18
#define BUTTONb3 19
#define BUTTONb4 20
#define BUTTONb5 21
#define BUTTONb6 22
#define BUTTONb7 23
#define BUTTONb8 24
#define BUTTONb9 25
#define BUTTONb10 26
#define BUTTONb11 27
#define BUTTONb12 28
#define BUTTONb13 29
#define BUTTONb14 30
#define BUTTONb15 31



Adafruit_MCP23X17 mcp1;
Adafruit_MCP23X17 mcp2;

void setup() {
  Serial.begin(9600);
  //while (!Serial);
  Serial.println("MCP23xxx Button Test!");

  // uncomment appropriate mcp.begin
  if (!mcp1.begin_I2C(0x20)) {
  //if (!mcp.begin_SPI(CS_PIN)) {
    Serial.println("Error1.");
    while (1);
  }
   if (!mcp2.begin_I2C(0x22)) {
  //if (!mcp.begin_SPI(CS_PIN)) {
    Serial.println("Error2.");
    while (1);
  }

  // Set all BUTTONa1 to BUTTONa15 as INPUT_PULLUP on mcp1 (Port A)
for (uint8_t i = 0; i <= 15; i++) {
  mcp1.pinMode(i, INPUT_PULLUP);
}

// Set all BUTTONb1 to BUTTONb15 as INPUT_PULLUP on mcp2 (Port B)
for (uint8_t i = 0; i <= 15; i++) {
  mcp2.pinMode(i, INPUT_PULLUP);
}

  Serial.println("INIT DONE");
}

void loop() {
  // Check Port A (BUTTONa1 to BUTTONa15 → pins 1 to 15)
  for (uint8_t i = 0; i <= 15; i++) {
    if (!mcp1.digitalRead(i)) {
      Serial.print("Button a-");
      Serial.println(i);
     
      delay(250);  // debounce or rate limit
    }
  }

for (uint8_t i = 0; i <= 15; i++) {
    if (!mcp2.digitalRead(i)) {
      Serial.print("Button b-");
      Serial.println(i);
      delay(250);  // debounce or rate limit
    }
  }

  
}
