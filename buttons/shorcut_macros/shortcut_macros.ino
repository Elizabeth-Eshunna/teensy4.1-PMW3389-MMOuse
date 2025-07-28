
#include <Bounce2.h>
#include "usb_mouse.h" // For advanced mouse functions like horizontal scroll

// --- Step 1: Name all your buttons for readability ---
// Give each button a unique index from 0 to NUM_BUTTONS-1
#define LEFT_FAR_EDGE       0
#define LEFT_NEAR_EDGE      1
// RIGHT_EDGE is the layer key and handled separately
#define LEFT_TIP            2
#define RIGHT_TIP           3
#define LEFT_PUSH_TRIGGER   4
#define RIGHT_PUSH_TRIGGER  5
// Add more names if you have more buttons

// --- Configuration ---
const int NUM_BUTTONS = 29; // Total number of regular action buttons
const int LAYER_SWITCH_PIN = 6; // Pin for RIGHT_EDGE (the layer key)

// --- Step 2: Assign your physical PINS here ---
// The order MUST match the #define names above.
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
Bounce buttons[NUM_BUTTONS]; // Array of Bounce objects for each button

int currentLayer = 0;
const int NUM_LAYERS = 2;

void setup() {
  // --- Layer Switch Button Setup (RIGHT_EDGE) ---
  pinMode(LAYER_SWITCH_PIN, INPUT_PULLUP);
  layerSwitchButton.attach(LAYER_SWITCH_PIN);
  layerSwitchButton.interval(5); // Debounce interval in milliseconds

  // --- General Button Setup ---
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    buttons[i].attach(buttonPins[i]);
    buttons[i].interval(5);
  }

  Keyboard.begin();
  Mouse.begin();
  Serial.begin(9600); // Optional: for debugging
}

void loop() {
  // --- Update Button States ---
  layerSwitchButton.update();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].update();
  }

  // --- Layer Switching Logic ---
  if (layerSwitchButton.fell()) {
    currentLayer = (currentLayer + 1) % NUM_LAYERS;
    Serial.print("Switched to Layer: "); // Optional debug message
    Serial.println(currentLayer);
  }

  // --- Button Action Logic ---
  for (int i = 0; i < NUM_BUTTONS; i++) {

switch (i) {

      // --- CASE 1: Stateful "Hold-Down" Buttons ---
      // Add any button that needs separate press/release actions here.
      
      case LEFT_FAR_EDGE: // Copy
        if (buttons[i].fell()) { // On PRESS
          if (currentLayer == 0) {
            Keyboard.press(MODIFIERKEY_CTRL);
            Keyboard.press('c');
          } else { // Layer 1
            Keyboard.press(MODIFIERKEY_GUI);
            Keyboard.press('v');
          }
        } else if (buttons[i].rose()) { // On RELEASE
          if (currentLayer == 0) {
            Keyboard.release('c');
            Keyboard.release(MODIFIERKEY_CTRL);
          } else { // Layer 1
            Keyboard.release('v');
            Keyboard.release(MODIFIERKEY_GUI);
          }
        }
        break; // IMPORTANT: End the case

      case LEFT_NEAR_EDGE: // Paste
        if (buttons[i].fell()) { // On PRESS
          if (currentLayer == 0) {
            Keyboard.press(MODIFIERKEY_CTRL);
            Keyboard.press('v');
          }
          // No action on Layer 1 yet for this button
        } else if (buttons[i].rose()) { // On RELEASE
          if (currentLayer == 0) {
            Keyboard.release('v');
            Keyboard.release(MODIFIERKEY_CTRL);
          }
        }
        break; // IMPORTANT: End the case

      // Add more hold-down buttons here with new 'case' statements...


      // --- CASE 2: Stateless "Tap" Buttons ---
      // For all other buttons, we call the simple handler function.
      default:
        if (buttons[i].fell()) {
          handleKeyPress(i);
        }
        break;
    }
  }
}


// --- Function to Handle All Key Presses ---
void handleKeyPress(int buttonIndex) {
  // Use a switch statement for clean and efficient routing
  switch (buttonIndex) {

   

    // --- Tip Buttons ---
    case LEFT_TIP:
      if (currentLayer == 0) Keyboard.write(KEY_ENTER);
      break;

    // --- Trigger Buttons ---
    case LEFT_PUSH_TRIGGER:
      if (currentLayer == 0) Keyboard.write(KEY_BACKSPACE);
      break;
    case RIGHT_PUSH_TRIGGER:
      if (currentLayer == 0) Keyboard.write(KEY_DELETE);
      break;

  }
}

