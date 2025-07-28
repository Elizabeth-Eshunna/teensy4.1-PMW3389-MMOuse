#include "usb_keyboard.h"
#include "usb_mouse.h"
//24-32
#define MS_0       20
#define MS_1       21
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
#define M5_NO      41
#define M5_NC      42
#define SCROLL_LEFT 25
#define SCROLL_RIGHT 24
#define FAR_EDGE 0
#define NEAR_EDGE 2
#define RIGHT_EDGE 37
#define LEFT_TIP 4
#define RIGHT_TIP 5 
#define LEFT_PUSH_TRIGGER 6
#define RIGHT_PUSH_TRIGGER 7
#define LEFT_PULL_TRIGGER 8
#define RIGHT_PULL_TRIGGER 9
#define FAR_WING 10
#define MID_WING 11
#define NEAR_WING 12
#define FAR_DPI 14 
#define MID_DPI 15
#define NEAR_DPI 16
#define GRID_TOP_LEFT 17
#define GRID_LEFT 18
#define GRID_BOT_LEFT 19
#define GRID_BOT 20
#define GRID_BOT_RIGHT 21
#define GRID_RIGHT 22
#define GRID_FAR_TOP 23
#define GRID_FAR_BOT 24

static int8_t   buttons                 = 0;
static int8_t   buttons_old             = 0;
static elapsedMillis button_timer = 0;

void setup() {
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
  pinMode(SCROLL_LEFT, INPUT_PULLUP);
  pinMode(SCROLL_RIGHT, INPUT_PULLUP);
  pinMode(FAR_EDGE, INPUT_PULLUP);
  pinMode(NEAR_EDGE, INPUT_PULLUP);
  pinMode(RIGHT_EDGE, INPUT_PULLUP);
  pinMode(LEFT_TIP, INPUT_PULLUP);
  pinMode(RIGHT_TIP, INPUT_PULLUP);
  pinMode(LEFT_PUSH_TRIGGER, INPUT_PULLUP);
  pinMode(RIGHT_PUSH_TRIGGER, INPUT_PULLUP);
  pinMode(LEFT_PULL_TRIGGER, INPUT_PULLUP);
  pinMode(RIGHT_PULL_TRIGGER, INPUT_PULLUP);
  pinMode(FAR_WING, INPUT_PULLUP);
  pinMode(MID_WING, INPUT_PULLUP);
  pinMode(NEAR_WING, INPUT_PULLUP);
  pinMode(FAR_DPI, INPUT_PULLUP);
  pinMode(MID_DPI, INPUT_PULLUP);
  pinMode(NEAR_DPI, INPUT_PULLUP);
  pinMode(GRID_TOP_LEFT, INPUT_PULLUP);
  pinMode(GRID_LEFT, INPUT_PULLUP);
  pinMode(GRID_BOT_LEFT, INPUT_PULLUP);
  pinMode(GRID_BOT, INPUT_PULLUP);
  pinMode(GRID_BOT_RIGHT, INPUT_PULLUP);
  pinMode(GRID_RIGHT, INPUT_PULLUP);
  pinMode(GRID_FAR_TOP, INPUT_PULLUP);
  pinMode(GRID_FAR_BOT, INPUT_PULLUP);

}

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

void update_usb()
{
  int8_t scroll           = (buttons & 0b10000000) ? scroll =  1 : (buttons & 0b01000000) ? scroll = -1 : scroll = 0;
  usb_mouse_buttons_state =  buttons;
  buttons_old             =  buttons;
  // This function is blocking if its buffers fill up and blocks until the next USB poll (125uS at USB 2.0 HIGH speed and 1mS below that speed)
  // usb_mouse_move only accepts 8 bit data by default it still works with 8 bit values but at lower polling speeds (1000Hz and lower)
  // maximum control speed is limited due to the integers overflowing. Recommended to make this function 16 bit for lower polling speeds (see my github)
  
}

void loop() {
  // SCROLL_LEFT → horizontal scroll left
  if (!digitalReadFast(SCROLL_LEFT)) {
    usb_mouse_move(0, 0, 0, -1); delay(100);// horizontal scroll left
  } else {
    usb_mouse_move(0, 0, 0, 0); 
  }

  // SCROLL_RIGHT → horizontal scroll right
  if (!digitalReadFast(SCROLL_RIGHT)) {
    usb_mouse_move(0, 0, 0, 1);  delay(100);// horizontal scroll right
  } else {
    usb_mouse_move(0, 0, 0, 0);
  }

  if (!digitalReadFast(FAR_EDGE)) { 
    Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('c'); 
  } else { 
    Keyboard.release('c'); Keyboard.release(KEY_LEFT_CTRL);
  }
  
   
    update_buttons();
    update_usb();
      
    
}
