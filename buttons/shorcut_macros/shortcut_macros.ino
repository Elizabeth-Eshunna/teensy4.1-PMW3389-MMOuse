#include <Bounce2.h>
#include "usb_keyboard.h"
#include "usb_mouse.h" // For advanced mouse functions like horizontal scroll

// --- Button Definitions ---
#define LEFT_FAR_EDGE       0
#define LEFT_NEAR_EDGE      1
#define LEFT_TIP            2
#define RIGHT_TIP           3
#define LEFT_PUSH_TRIGGER   4
#define RIGHT_PUSH_TRIGGER  5

// --- Configuration ---
const int NUM_BUTTONS = 6; // Corrected to match your defined buttons
const int LAYER_SWITCH_PIN = 6; // Pin for RIGHT_EDGE (the layer key)

const int buttonPins[NUM_BUTTONS] = {
  0, // LEFT_FAR_EDGE
  1, // LEFT_NEAR_EDGE
  2, // LEFT_TIP
  3, // RIGHT_TIP
  4, // LEFT_PUSH_TRIGGER
  5, // RIGHT_PUSH_TRIGGER
};

// --- Global Variables ---
Bounce layerSwitchButton = Bounce();
Bounce buttons[NUM_BUTTONS];
int currentLayer = 0;
const int NUM_LAYERS = 2;

void setup() {
  pinMode(LAYER_SWITCH_PIN, INPUT_PULLUP);
  layerSwitchButton.attach(LAYER_SWITCH_PIN);
  layerSwitchButton.interval(5);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    buttons[i].attach(buttonPins[i]);
    buttons[i].interval(5);
  }

  Keyboard.begin();
  Mouse.begin();
  Serial.begin(9600);
}

// --- MAIN LOOP with Fallback Logic ---
void loop() {
  layerSwitchButton.update();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].update();
  }

 if (layerSwitchButton.read() == LOW) {
    currentLayer = 1;
    Serial.print("Switched to Layer: "); // Optional debug message
    Serial.println(currentLayer);
  } else {
    currentLayer = 0;
    Serial.print("Switched to Layer: "); // Optional debug message
    Serial.println(currentLayer);
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    switch (i) {

      // --- Stateful "Hold-Down" Buttons ---
      case LEFT_FAR_EDGE: // This button has a Layer 1 override
        if (buttons[i].fell()) { // On PRESS
          // Check for the override layer first
          if (currentLayer == 1) { // <-- CHANGE HERE
            Keyboard.press(MODIFIERKEY_GUI);
            Keyboard.press('v');
          } else { // Fallback to Layer 0 action
            Keyboard.press(MODIFIERKEY_CTRL);
            Keyboard.press('v');
          }
        } else if (buttons[i].rose()) { // On RELEASE
          // Same logic for release
          if (currentLayer == 1) { // <-- CHANGE HERE
            Keyboard.release('v');
            Keyboard.release(MODIFIERKEY_GUI);
          } else { // Fallback to Layer 0 action
            Keyboard.release('v');
            Keyboard.release(MODIFIERKEY_CTRL);
          }
        }
        break;

      case LEFT_NEAR_EDGE: // This button has NO Layer 1 override
        if (buttons[i].fell()) {
          // No layer check needed, this is the default action
          Keyboard.press(MODIFIERKEY_CTRL);
          Keyboard.press('c');
        } else if (buttons[i].rose()) {
          Keyboard.release('c');
          Keyboard.release(MODIFIERKEY_CTRL);
        }
        break;

      // --- Stateless "Tap" Buttons ---
    
  }
}
}
