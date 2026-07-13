#include "Adafruit_TinyUSB.h"
#include <math.h>

// ============================================================================
// Orbit DIY SpaceMouse Pro Firmware - ESP32-S3 Edition
// Auhor: Manuel Borrajo
// ============================================================================

// ============================================================================
// Based on Hackman3D DIY SpaceMouse Firmware - Arduino Pro Micro Edition
// ============================================================================

// ============================================================================
// SETTINGS (Adjusted for 12-bits: the original values multiplied by 4)
// ============================================================================

// Input deadzone applied directly after reading the joysticks.
const int DEADZONE_INPUT  = 160;  // Original: 40
// Output deadzone applied before sending values to the computer
const int DEADZONE_OUTPUT = 180;  // Original: 45
// Number of samples used at startup to calculate joystick center positions.
const int CENTER_SAMPLES  = 100; 
// Smoothing divisor. Higher value = smoother but slower response.
const int SMOOTH_DIVISOR  = 5; 

// ============================================================================
// SENSITIVITY 
// ============================================================================

// Translation sensitivity.
const float GAIN_TX = 1.3;
const float GAIN_TY = 1.3;
const float GAIN_TZ = 2.3;

// Rotation sensitivity.
const float GAIN_RX = 1.8;
const float GAIN_RY = 1.8;
const float GAIN_RZ = 2.0;

// Global maximum speed scale. 0.70 means 30% slower than the base firmware.
const float MAX_SPEED_SCALE = 0.70;

// Response curve. 1.0 is linear; higher values make small movements slower.
const float RESPONSE_CURVE = 1.6;

// Speed profiles. Default mode 1 keeps the values above.
const int SPEED_MODE_COUNT = 3;
const int DEFAULT_SPEED_MODE = 1;
const float SPEED_MODE_SCALE[SPEED_MODE_COUNT] = { 0.50, MAX_SPEED_SCALE, 1.00 }; 
const float SPEED_MODE_RESPONSE_CURVE[SPEED_MODE_COUNT] = { 1.9, RESPONSE_CURVE, 1.3 };

// Serial debug output. Keep disabled during normal HID use.
const bool DEBUG_SERIAL = false; 
const unsigned long DEBUG_SERIAL_BAUD = 115200;
const unsigned long DEBUG_SERIAL_INTERVAL_MS = 100;

// Lower value gives rotation more priority over translation.
const float ROTATION_PRIORITY = 0.65;

// If false, multiple axes can be sent at the same time.
const bool ENABLE_DOMINANT_AXIS_FILTER = false;

// Experimental mode for slicers without native SpaceMouse support.
// SLICER MODE (Umbrales escalados a 12-bits)
const bool ENABLE_SLICER_MOUSE_MODE = false;
const bool DEFAULT_SLICER_MOUSE_MODE = false;
const bool ENABLE_SLICER_KEYBOARD_SHORTCUTS = true;
const int SLICER_MOUSE_MOVE_DIVISOR = 120;
const int SLICER_MOUSE_WHEEL_THRESHOLD = 360; // Original: 90 
const int SLICER_MOUSE_WHEEL_FULL_SCALE = 2800; // Original: 700 
const int SLICER_MOUSE_MAX_MOVE = 12;
const int SLICER_MOUSE_MAX_WHEEL = 1;
const unsigned long SLICER_MOUSE_WHEEL_MIN_INTERVAL_MS = 45; 
const unsigned long SLICER_MOUSE_WHEEL_MAX_INTERVAL_MS = 125;
const bool SLICER_MOUSE_AUTO_DRAG = true;

const uint8_t SLICER_MOUSE_BUTTON_LEFT = 0x01; 
const uint8_t SLICER_MOUSE_BUTTON_RIGHT = 0x02;
const uint8_t SLICER_MOUSE_BUTTON_MIDDLE = 0x04;
const uint8_t SLICER_MOUSE_DRAG_BUTTON = SLICER_MOUSE_BUTTON_LEFT;

const uint8_t SLICER_SHORTCUT_MODIFIER_PRIMARY = 0x08; // macOS Command
const uint8_t SLICER_SHORTCUT_MODIFIER_SHIFT = 0x02;
const uint8_t SLICER_SHORTCUT_KEY_0 = 0x27;
const uint8_t SLICER_SHORTCUT_KEY_A = 0x04; 
const uint8_t SLICER_SHORTCUT_KEY_G = 0x0A;
const uint8_t SLICER_SHORTCUT_KEY_L = 0x0F;
const uint8_t SLICER_SHORTCUT_KEY_N = 0x11;
const uint8_t SLICER_SHORTCUT_KEY_TAB = 0x2B; 

const int SLICER_BUTTON_ACTION_HOME = 1;
const int SLICER_BUTTON_ACTION_PAINT = 2;
const int SLICER_BUTTON_ACTION_TAB_SEND = 3;
const unsigned long SLICER_BUTTON_LONG_PRESS_MS = 650;
const int SLICER_BUTTON_ACTIONS[3] = { 
  SLICER_BUTTON_ACTION_TAB_SEND, 
  SLICER_BUTTON_ACTION_PAINT, 
  SLICER_BUTTON_ACTION_HOME 
};

// ============================================================================
// AXIS INVERSION / INVERSION DES AXES
// ============================================================================

// Set to true to reverse an axis.
bool invX  = false;
bool invY  = false;
bool invZ  = false;
bool invRX = true;
bool invRY = true;
bool invRZ = true;

// ============================================================================
// PIN CONFIGURATION / CONFIGURATION DES PINS
// ============================================================================

// Analog pins used by the 4 Hall effect joysticks.
const int pins[8] = { 4, 5, 6, 7, 8, 9, 10, 11 }; 

// Button pins. Buttons must be wired between pin and GND.
const int BUTTON_COUNT = 32;

#define NC 99 // NOT CONNECTED/ASSIGNED
const int buttonPins[BUTTON_COUNT] = { 
  1,  2,  3,  NC, 12, 13, NC, NC,   // 0-7 bit   = | MENU   | FIT  |  T   | - | R | F |  -  |  -  |
  14, NC, NC, NC, 15, 16, 17, 18,   // 8-15 bit  = | ROTATE |  -   |  -   | - | 1 | 2 |  3  |  4  |
  NC, NC, NC, NC, NC, NC, 21, 34,   // 16-23 bit = |   -    |  -   |  -   | - | - | - | ESC | ALT |
  39, 40, 41, NC, NC, NC, NC, NC    // 24-31 bit = | SHIFT  | CTRL | LOCK | - | - | - |  -  |  -  |
}; // GPIO seguros para botones

/* BOTONES del SPACEMOUSE PRO */
/* 
  BIT 0: MENU
  BIT 1: FIT
  BIT 2: T
  BIT 4: R
  BIT 5: F
  BIT 8: ROTATE
  BIT 12: 1
  BIT 13: 2
  BIT 14: 3
  BIT 15: 4
  BIT 22: ESC
  BIT 23: ALT
  BIT 24: SHIFT
  BIT 25: CTRL
  BIT 26: LOCK
*/

// Button indexes used to switch speed mode.
const int MODE_SWITCH_BUTTONS[BUTTON_COUNT] = { 
  0,  1,  2,  3,  4,  5,  6,  7, 
  8,  9,  10, 11, 12, 13, 14, 15, 
  16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, 28, 29, 30, 31 
};
const int MODE_SWITCH_BUTTON_COUNT = 32;

// If true, the mode-switch button combo is not sent as normal HID buttons.
const bool MODE_SWITCH_SUPPRESS_BUTTONS = true;

// Time to wait for the full combo before sending individual combo buttons.
const unsigned long MODE_SWITCH_CHORD_WINDOW_MS = 250;

// Minimum time between two speed mode changes.
const unsigned long MODE_SWITCH_DEBOUNCE_MS = 500;

// Button indexes used to toggle CAD / slicer mouse mode. Default = buttons 2 + 3.
const int SLICER_MODE_BUTTONS[BUTTON_COUNT] = { 1, 2, 0 };
const int SLICER_MODE_BUTTON_COUNT = 2;
const unsigned long SLICER_MODE_HOLD_MS = 250;
const unsigned long SLICER_MODE_DEBOUNCE_MS = 500;


// ============================================================================
// GLOBAL VARIABLES / VARIABLES GLOBALES
// ============================================================================

//Stores the neutral center value for each analog input.
int center[8];
// Smoothed values sent to the computer.
int16_t smoothTX = 0, smoothTY = 0, smoothTZ = 0;
int16_t smoothRX = 0, smoothRY = 0, smoothRZ = 0;

// Current speed profile index.
int currentSpeedMode = DEFAULT_SPEED_MODE;
bool slicerMouseModeEnabled = ENABLE_SLICER_MOUSE_MODE && DEFAULT_SLICER_MOUSE_MODE;

bool modeSwitchComboWasPressed = false;
bool modeSwitchChordActive = false;
bool modeSwitchChordComboTriggered = false;
bool modeSwitchButtonsForwarded = false;
uint32_t modeSwitchPendingButtons = 0;
unsigned long modeSwitchChordStartedAt = 0;
unsigned long lastModeSwitchAt = 0;

bool slicerModeComboWasPressed = false;
bool slicerModeToggleHandled = false;
uint8_t slicerMouseButtonMask = 0;
bool slicerButtonWasPressed[3] = { false, false, false };
bool slicerButtonLongHandled[3] = { false, false, false };
unsigned long slicerButtonPressedAt[3] = { 0, 0, 0 };
unsigned long slicerModeComboStartedAt = 0;
unsigned long lastSlicerModeSwitchAt = 0;
unsigned long lastSlicerMouseWheelAt = 0;
int8_t lastSlicerMouseWheelDirection = 0;

// ============================================================================
// HID DESCRIPTOR (TinyUSB Unified)
// ============================================================================
#define REPORT_ID_TRANS 1
#define REPORT_ID_ROT   2
#define REPORT_ID_BTN   3
#define REPORT_ID_MOUSE 4
#define REPORT_ID_KBD   5

uint8_t const desc_hid_report[] = {
  // --- 3Dconnexion Translation --- 
  0x05, 0x01, 0x09, 0x08, 0xA1, 0x01,
  0xA1, 0x00, 0x85, REPORT_ID_TRANS,
  0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F, 0x36, 0x00, 0x80, 0x46, 0xFF, 0x7F,
  0x09, 0x30, 0x09, 0x31, 0x09, 0x32, 0x75, 0x10, 0x95, 0x03, 0x81, 0x02, 0xC0,
  
  // --- 3Dconnexion Rotation ---
  0xA1, 0x00, 0x85, REPORT_ID_ROT,
  0x16, 0x00, 0x80, 0x26, 0xFF, 0x7F, 0x36, 0x00, 0x80, 0x46, 0xFF, 0x7F,
  0x09, 0x33, 0x09, 0x34, 0x09, 0x35, 0x75, 0x10, 0x95, 0x03, 0x81, 0x02, 0xC0,
  
  // --- 3Dconnexion Buttons --- 
  0xA1, 0x00, 0x85, REPORT_ID_BTN,
  0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 32,
  0x05, 0x09, 0x19, 1, 0x29, 32, 0x81, 0x02, 0xC0, 0xC0,
  
  // --- Slicer Mouse --- 
  0x05, 0x01, 0x09, 0x02, 0xA1, 0x01,
  0x85, REPORT_ID_MOUSE, 0x09, 0x01, 0xA1, 0x00,
  0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02,
  0x95, 0x01, 0x75, 0x05, 0x81, 0x03,
  0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x03, 0x81, 0x06,
  0xC0, 0xC0,

  // --- Slicer Keyboard --- 
  0x05, 0x01, 0x09, 0x06, 0xA1, 0x01,
  0x85, REPORT_ID_KBD,
  0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
  0x95, 0x01, 0x75, 0x08, 0x81, 0x03,
  0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x73, 0x05, 0x07, 0x19, 0x00, 0x29, 0x73, 0x81, 0x00, 0xC0
};

Adafruit_USBD_HID usb_hid;

// ============================================================================
// Core Functions
// ============================================================================

// Reads all analog inputs.
void readAxes(int* values) {
  for (int i = 0; i < 8; i++) {
    values[i] = analogRead(pins[i]);
  }
}

// Calculates the neutral center value of every joystick at startup.
// Important:
// Do not touch the SpaceMouse during startup calibration.
void calibrateCenter() {
  long sum[8] = {0};
  for (int n = 0; n < CENTER_SAMPLES; n++) {
    int temp[8];
    readAxes(temp);
    for (int i = 0; i < 8; i++) sum[i] += temp[i];
    delay(5);
  }
  for (int i = 0; i < 8; i++) center[i] = sum[i] / CENTER_SAMPLES;
}

// Removes small joystick noise before calculations.
void applyInputDeadzone(int* values) {
  for (int i = 0; i < 8; i++) {
    if (abs(values[i]) < DEADZONE_INPUT) values[i] = 0;
  }
}

// Removes tiny final output values to avoid drift.
void applyOutputDeadzone(int16_t &x, int16_t &y, int16_t &z, int16_t &rx, int16_t &ry, int16_t &rz) {
  if (abs(x)  < DEADZONE_OUTPUT) x  = 0;
  if (abs(y)  < DEADZONE_OUTPUT) y  = 0;
  if (abs(z)  < DEADZONE_OUTPUT) z  = 0;
  if (abs(rx) < DEADZONE_OUTPUT) rx = 0;
  if (abs(ry) < DEADZONE_OUTPUT) ry = 0;
  if (abs(rz) < DEADZONE_OUTPUT) rz = 0;
}

// Counts how many of 4 values are above a threshold.
int countPositive4(int a, int b, int c, int d, int threshold) {
  int n = 0;
  if (a > threshold) n++; 
  if (b > threshold) n++;
  if (c > threshold) n++; 
  if (d > threshold) n++;
  return n;
}

// Counts how many of 4 values are below a negative threshold.
int countNegative4(int a, int b, int c, int d, int threshold) {
  int n = 0;
  if (a < -threshold) n++; 
  if (b < -threshold) n++;
  if (c < -threshold) n++; 
  if (d < -threshold) n++;
  return n;
}

// Smooths movement to avoid harsh jumps.
int16_t smoothValue(int16_t current, int16_t target) {
  int16_t delta = target - current;
  if (delta == 0) return current;

  int16_t step = delta / SMOOTH_DIVISOR;
  if (step == 0) step = (delta > 0) ? 1 : -1;

  return current + step;
}

// Applies sensitivity gain to an axis value.
int16_t applyGain(int16_t value, float gain) {
  return (int16_t)(value * gain);
}

// Scales maximum speed and makes small movements easier to control.
int16_t applyResponseCurve(int16_t value, float inputMax, float speedScale, float responseCurve) {
  if (value == 0) return 0;

  float magnitude = abs(value);
  float maxMagnitude = inputMax;

  if (maxMagnitude < DEADZONE_OUTPUT + 1.0) maxMagnitude = DEADZONE_OUTPUT + 1.0;
  if (magnitude < DEADZONE_OUTPUT) return 0;
  if (magnitude > maxMagnitude) magnitude = maxMagnitude;

  float normalized = (magnitude - DEADZONE_OUTPUT) / (maxMagnitude - DEADZONE_OUTPUT);
  float maxOutputMagnitude = maxMagnitude * speedScale;

  if (maxOutputMagnitude < DEADZONE_OUTPUT) maxOutputMagnitude = DEADZONE_OUTPUT;

  float curved = DEADZONE_OUTPUT + pow(normalized, responseCurve) * (maxOutputMagnitude - DEADZONE_OUTPUT);
  if (value < 0) curved = -curved;

  return (int16_t)curved;
}

// Clears smoothed axis memory after mode changes.
void resetSmoothing() {
  smoothTX = 0; 
  smoothTY = 0; 
  smoothTZ = 0;

  smoothRX = 0; 
  smoothRY = 0; 
  smoothRZ = 0;
}

// Clears temporary state used while detecting a speed-mode button chord.
void resetModeSwitchChord() {
  modeSwitchChordActive = false;
  modeSwitchChordComboTriggered = false;
  modeSwitchButtonsForwarded = false;
  modeSwitchPendingButtons = 0;
}

// Keeps only the strongest axis and cancels the others.
// This makes the SpaceMouse easier to control and reduces unwanted diagonal
// movements.
void keepOnlyDominantAxis(int16_t &tx, int16_t &ty, int16_t &tz, int16_t &rx, int16_t &ry, int16_t &rz) {
  int16_t values[6] = { tx, ty, tz, rx, ry, rz };
  int maxIndex = 0;
  int maxValue = abs(values[0]);

  for (int i = 1; i < 6; i++) {
    if (abs(values[i]) > maxValue) {
      maxValue = abs(values[i]);
      maxIndex = i;
    }
  }

  tx = (maxIndex == 0) ? tx : 0; 
  ty = (maxIndex == 1) ? ty : 0; 
  tz = (maxIndex == 2) ? tz : 0;
  rx = (maxIndex == 3) ? rx : 0; 
  ry = (maxIndex == 4) ? ry : 0; 
  rz = (maxIndex == 5) ? rz : 0;
}

// ============================================================================
// TinyUSB HID Send Functions
// ============================================================================

// Sends translation and rotation reports to the computer.
void sendCommand(int16_t rx, int16_t ry, int16_t rz, int16_t x, int16_t y, int16_t z) {
  if (!usb_hid.ready()) return;

  uint8_t trans[6] = { 
    (uint8_t)(x & 0xFF), (uint8_t)(x >> 8), 
    (uint8_t)(y & 0xFF), (uint8_t)(y >> 8), 
    (uint8_t)(z & 0xFF), (uint8_t)(z >> 8) 
  };

  uint8_t rot[6] = { 
    (uint8_t)(rx & 0xFF), (uint8_t)(rx >> 8),
    (uint8_t)(ry & 0xFF), (uint8_t)(ry >> 8), 
    (uint8_t)(rz & 0xFF), (uint8_t)(rz >> 8) 
  };
  usb_hid.sendReport(REPORT_ID_TRANS, trans, sizeof(trans));

  // Waiting to PC complete to read the previous report
  // The send report in arduino is sync but in ESP32 is async
  while (!usb_hid.ready()) {
    delay(1); 
  }

  usb_hid.sendReport(REPORT_ID_ROT, rot, sizeof(rot));
}

// Sends buttons reports to the computer
void sendButtons(uint32_t buttonMask) {
  // Save the last button state in a static variable
  static uint32_t lastButtonMask = 0; 

  // if the state doesn't change
  if (buttonMask == lastButtonMask) return;

  // if the state was changed then wait to send the report
  while (!usb_hid.ready()) {
    delay(1);
  }
  
  uint8_t buttons[4] = { 
    (uint8_t)(buttonMask & 0xFF), 
    (uint8_t)((buttonMask >> 8) & 0xFF), 
    (uint8_t)((buttonMask >> 16) & 0xFF), 
    (uint8_t)((buttonMask >> 24) & 0xFF) 
  }; 
  
  usb_hid.sendReport(REPORT_ID_BTN, buttons, sizeof(buttons));

  // Update the button state
  lastButtonMask = buttonMask;
}

// Sends a relative USB mouse report for slicer mouse emulation.
void sendSlicerMouseReport(int8_t x, int8_t y, int8_t wheel) {
  if (!usb_hid.ready()) return;
  uint8_t report[4] = { slicerMouseButtonMask, (uint8_t)x, (uint8_t)y, (uint8_t)wheel };
  usb_hid.sendReport(REPORT_ID_MOUSE, report, sizeof(report));
}

// Sends one keyboard shortcut through the slicer keyboard interface.
void sendSlicerKeyboardShortcut(uint8_t modifiers, uint8_t key) {
  if (!ENABLE_SLICER_KEYBOARD_SHORTCUTS || !usb_hid.ready()) return;

  uint8_t report[8] = { modifiers, 0, key, 0, 0, 0, 0, 0 };
  usb_hid.sendReport(REPORT_ID_KBD, report, sizeof(report));
  delay(20);
  uint8_t clear_report[8] = { 0 };
  usb_hid.sendReport(REPORT_ID_KBD, clear_report, sizeof(clear_report));
}

// ============================================================================
// Controls Logic
// ============================================================================

// Reads physical buttons into a bit mask.
uint32_t readButtonMask() {
  uint32_t buttons = 0;

  for (int i = 0; i < BUTTON_COUNT; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      buttons |= (1UL << i);
    }
  }

  return buttons;
}

// Builds a bit mask from the configured mode-switch buttons.
uint32_t getModeSwitchButtonMask() {
  if (MODE_SWITCH_BUTTON_COUNT <= 0 || MODE_SWITCH_BUTTON_COUNT > BUTTON_COUNT) return 0;

  uint32_t comboMask = 0;

  for (int i = 0; i < MODE_SWITCH_BUTTON_COUNT; i++) comboMask |= (1UL << MODE_SWITCH_BUTTONS[i]);

  return comboMask;
}

// Builds a bit mask from the configured slicer-mode buttons.
uint32_t getSlicerModeButtonMask() {
  if (SLICER_MODE_BUTTON_COUNT <= 0 || SLICER_MODE_BUTTON_COUNT > BUTTON_COUNT) return 0;

  uint32_t comboMask = 0;

  for (int i = 0; i < SLICER_MODE_BUTTON_COUNT; i++) comboMask |= (1UL << SLICER_MODE_BUTTONS[i]);

  return comboMask;
}

// Checks if the configured speed mode button combo is pressed.
bool isModeSwitchComboPressed(uint32_t buttonMask) {
  uint32_t comboMask = getModeSwitchButtonMask();
  if (comboMask == 0) return false;
  return (buttonMask & comboMask) == comboMask;
}

// Checks if only the configured slicer-mode combo is pressed.
bool isSlicerModeComboPressed(uint32_t buttonMask) {
  uint32_t comboMask = getSlicerModeButtonMask();
  if (!ENABLE_SLICER_MOUSE_MODE || comboMask == 0) return false;
  return buttonMask == comboMask;
}

// Cycles through slow / normal / fast speed profiles.
void updateSpeedMode(bool comboPressed) {
  unsigned long now = millis();
  if (comboPressed && !modeSwitchComboWasPressed && now - lastModeSwitchAt >= MODE_SWITCH_DEBOUNCE_MS) {
    currentSpeedMode++;
    if (currentSpeedMode >= SPEED_MODE_COUNT) currentSpeedMode = 0;
    lastModeSwitchAt = now;
    resetSmoothing();
  }
  modeSwitchComboWasPressed = comboPressed;
}

// Updates one mouse button used by slicer mouse emulation.
void setSlicerMouseButton(uint8_t button, bool pressed) {
  uint8_t previousMask = slicerMouseButtonMask;
  if (pressed) slicerMouseButtonMask |= button;
  else slicerMouseButtonMask &= ~button;
  if (slicerMouseButtonMask != previousMask) sendSlicerMouseReport(0, 0, 0);
}

// Releases mouse buttons used by slicer mouse emulation
void releaseSlicerMouseButtons() {
  if (slicerMouseButtonMask == 0) return;
  slicerMouseButtonMask = 0;
  sendSlicerMouseReport(0, 0, 0);
}

// Toggles CAD / slicer mouse emulation mode.
void updateSlicerMouseMode(bool comboPressed) {
  unsigned long now = millis();

  if (comboPressed && !slicerModeComboWasPressed) {
    slicerModeComboStartedAt = now;
    slicerModeToggleHandled = false;
  }
  if (comboPressed && 
      !slicerModeToggleHandled && 
      now - slicerModeComboStartedAt >= SLICER_MODE_HOLD_MS && 
      now - lastSlicerModeSwitchAt >= SLICER_MODE_DEBOUNCE_MS) {
    slicerMouseModeEnabled = !slicerMouseModeEnabled;
    lastSlicerModeSwitchAt = now;
    slicerModeToggleHandled = true;
    releaseSlicerMouseButtons();
    resetSmoothing();
  }
  if (!comboPressed) slicerModeToggleHandled = false;
  slicerModeComboWasPressed = comboPressed;
}

// Suppresses shortcut buttons while a speed-mode chord is being detected.
uint32_t filterModeSwitchButtons(uint32_t buttonMask, bool comboPressed, bool &comboAccepted) {
  comboAccepted = false;

  if (!MODE_SWITCH_SUPPRESS_BUTTONS) { 
    comboAccepted = comboPressed; 
    return buttonMask; 
  }

  uint32_t switchMask = getModeSwitchButtonMask();
  if (switchMask == 0) return buttonMask;

  unsigned long now = millis();
  uint32_t nonSwitchButtons = buttonMask & ~switchMask;
  uint32_t activeSwitchButtons = buttonMask & switchMask;

  if (activeSwitchButtons == 0) {
    modeSwitchChordActive = false;
    if (!modeSwitchChordComboTriggered && !modeSwitchButtonsForwarded && modeSwitchPendingButtons != 0) {
      uint32_t releasedButtons = modeSwitchPendingButtons;
      resetModeSwitchChord();
      return nonSwitchButtons | releasedButtons;
    }
    resetModeSwitchChord();
    return nonSwitchButtons;
  }

  if (!modeSwitchChordActive) {
    modeSwitchChordActive = true; 
    modeSwitchChordComboTriggered = false;
    modeSwitchButtonsForwarded = false; 
    modeSwitchPendingButtons = activeSwitchButtons;
    modeSwitchChordStartedAt = now;
  } else {
    modeSwitchPendingButtons |= activeSwitchButtons;
  }

  if (comboPressed || modeSwitchChordComboTriggered) {
    modeSwitchChordComboTriggered = true; 
    comboAccepted = true;
    return nonSwitchButtons;
  }

  if (now - modeSwitchChordStartedAt < MODE_SWITCH_CHORD_WINDOW_MS) return nonSwitchButtons;

  modeSwitchButtonsForwarded = true;
  return buttonMask;
}

// Suppresses slicer-mode combo buttons while toggling mode.
uint32_t filterSlicerModeButtons(uint32_t buttonMask, bool comboPressed) {
  if (!ENABLE_SLICER_MOUSE_MODE || !comboPressed) return buttonMask;

  uint32_t comboMask = getSlicerModeButtonMask();
  if (comboMask == 0) return buttonMask;

  return buttonMask & ~comboMask;
}

// Converts HID axis values to small relative mouse movements.
int8_t scaleMouseAxis(int16_t value, int divisor, int maxValue) {
  if (abs(value) < DEADZONE_OUTPUT) return 0;
  int scaled = value / divisor;
  if (scaled == 0) scaled = (value > 0) ? 1 : -1;
  if (scaled > maxValue) scaled = maxValue;
  if (scaled < -maxValue) scaled = -maxValue;
  return (int8_t)scaled;
}

// Sends wheel impulses slowly enough for slicer zoom control.
int8_t scaleMouseWheel(int16_t value) {
  int magnitude = abs(value);

  if (magnitude < SLICER_MOUSE_WHEEL_THRESHOLD) { 
    lastSlicerMouseWheelDirection = 0; 
    return 0; 
  }

  unsigned long now = millis();
  int cappedMagnitude = magnitude > SLICER_MOUSE_WHEEL_FULL_SCALE ? SLICER_MOUSE_WHEEL_FULL_SCALE : magnitude;
  int wheelRange = SLICER_MOUSE_WHEEL_FULL_SCALE - SLICER_MOUSE_WHEEL_THRESHOLD;
  if (wheelRange < 1) wheelRange = 1;

  unsigned long intervalRange = SLICER_MOUSE_WHEEL_MAX_INTERVAL_MS - SLICER_MOUSE_WHEEL_MIN_INTERVAL_MS;
  unsigned long interval = SLICER_MOUSE_WHEEL_MAX_INTERVAL_MS - ((unsigned long)(cappedMagnitude - SLICER_MOUSE_WHEEL_THRESHOLD) * intervalRange / wheelRange);
  int8_t direction = (value > 0) ? -SLICER_MOUSE_MAX_WHEEL : SLICER_MOUSE_MAX_WHEEL;

  if (direction == lastSlicerMouseWheelDirection && now - lastSlicerMouseWheelAt < interval) return 0;
  
  lastSlicerMouseWheelAt = now;
  lastSlicerMouseWheelDirection = direction;
  return direction;
}

// Clears slicer button shortcut state without sending actions.
void resetSlicerButtonActions() {
  for (int i = 0; i < BUTTON_COUNT; i++) {
    slicerButtonWasPressed[i] = false;
    slicerButtonLongHandled[i] = false;
    slicerButtonPressedAt[i] = 0;
  }
  if (ENABLE_SLICER_KEYBOARD_SHORTCUTS && usb_hid.ready()) {
    uint8_t clear_report[8] = { 0 };
    usb_hid.sendReport(REPORT_ID_KBD, clear_report, sizeof(clear_report));
  }
}

// Runs one configured slicer button shortcut.
void runSlicerButtonAction(int action, bool longPress) {
  releaseSlicerMouseButtons();

  if (action == SLICER_BUTTON_ACTION_HOME) {
    if (longPress) sendSlicerKeyboardShortcut(0, SLICER_SHORTCUT_KEY_A);
    else sendSlicerKeyboardShortcut(SLICER_SHORTCUT_MODIFIER_PRIMARY, SLICER_SHORTCUT_KEY_0);
  } else if (action == SLICER_BUTTON_ACTION_PAINT) {
    if (longPress) sendSlicerKeyboardShortcut(0, SLICER_SHORTCUT_KEY_L);
    else sendSlicerKeyboardShortcut(0, SLICER_SHORTCUT_KEY_N);
  } else if (action == SLICER_BUTTON_ACTION_TAB_SEND) {
    if (longPress) sendSlicerKeyboardShortcut(SLICER_SHORTCUT_MODIFIER_PRIMARY | SLICER_SHORTCUT_MODIFIER_SHIFT, SLICER_SHORTCUT_KEY_G);
    else sendSlicerKeyboardShortcut(0, SLICER_SHORTCUT_KEY_TAB);
  }
}

// Maps physical buttons to slicer shortcuts while slicer mode is active.
void updateSlicerMouseButtons(uint32_t buttonMask, bool suppressButtons) {
  if (suppressButtons) {
    releaseSlicerMouseButtons(); 
    resetSlicerButtonActions();
    return;
  }
  if (!ENABLE_SLICER_KEYBOARD_SHORTCUTS) return;

  unsigned long now = millis();

  for (int i = 0; i < BUTTON_COUNT; i++) { 
    bool pressed = (buttonMask & (1UL << i)) != 0;
    int action = SLICER_BUTTON_ACTIONS[i]; 

    if (pressed && !slicerButtonWasPressed[i]) {
      slicerButtonWasPressed[i] = true; 
      slicerButtonLongHandled[i] = false; 
      slicerButtonPressedAt[i] = now;
    }

    if (pressed && 
        (action == SLICER_BUTTON_ACTION_PAINT || 
         action == SLICER_BUTTON_ACTION_TAB_SEND || 
         action == SLICER_BUTTON_ACTION_HOME) && 
         !slicerButtonLongHandled[i] && 
         now - slicerButtonPressedAt[i] >= SLICER_BUTTON_LONG_PRESS_MS) {
      runSlicerButtonAction(action, true);
      slicerButtonLongHandled[i] = true;
    }

    if (!pressed && slicerButtonWasPressed[i]) {
      if (!slicerButtonLongHandled[i]) {
        runSlicerButtonAction(action, false);
      }
      slicerButtonWasPressed[i] = false; 
      slicerButtonLongHandled[i] = false; 
      slicerButtonPressedAt[i] = 0;
    }
  }
}

// Sends mouse drag / wheel events for slicers without SpaceMouse support.
void sendSlicerMouse(int16_t tx, int16_t ty, int16_t tz, int16_t rx, int16_t ry, int16_t rz) {
  int translationPower = abs(tx) + abs(ty);
  int rotationPower = abs(rx) + abs(ry) + abs(rz);
  bool zoomActive = abs(tz) >= SLICER_MOUSE_WHEEL_THRESHOLD;

  int8_t wheel = scaleMouseWheel(tz);
  int8_t mouseX = 0;
  int8_t mouseY = 0;

  if (zoomActive) {
    releaseSlicerMouseButtons();
    if (wheel != 0) sendSlicerMouseReport(0, 0, wheel);
    return;
  }

  if (!SLICER_MOUSE_AUTO_DRAG) {
    if (rotationPower > translationPower && rotationPower > DEADZONE_OUTPUT) {
      mouseX = scaleMouseAxis(ry + rz, SLICER_MOUSE_MOVE_DIVISOR, SLICER_MOUSE_MAX_MOVE);
      mouseY = scaleMouseAxis(rx, SLICER_MOUSE_MOVE_DIVISOR, SLICER_MOUSE_MAX_MOVE);
    } else if (translationPower > DEADZONE_OUTPUT) {
      mouseX = scaleMouseAxis(tx, SLICER_MOUSE_MOVE_DIVISOR, SLICER_MOUSE_MAX_MOVE);
      mouseY = scaleMouseAxis(ty, SLICER_MOUSE_MOVE_DIVISOR, SLICER_MOUSE_MAX_MOVE);
    }
    if (mouseX != 0 || mouseY != 0 || wheel != 0) sendSlicerMouseReport(mouseX, mouseY, wheel);
    return;
  }

  if (rotationPower > translationPower && rotationPower > DEADZONE_OUTPUT) {
    setSlicerMouseButton(SLICER_MOUSE_DRAG_BUTTON, true);
    mouseX = scaleMouseAxis(ry + rz, SLICER_MOUSE_MOVE_DIVISOR, SLICER_MOUSE_MAX_MOVE);
    mouseY = scaleMouseAxis(rx, SLICER_MOUSE_MOVE_DIVISOR, SLICER_MOUSE_MAX_MOVE);
  } else if (translationPower > DEADZONE_OUTPUT) {
    setSlicerMouseButton(SLICER_MOUSE_DRAG_BUTTON, true);
    mouseX = scaleMouseAxis(tx, SLICER_MOUSE_MOVE_DIVISOR, SLICER_MOUSE_MAX_MOVE);
    mouseY = scaleMouseAxis(ty, SLICER_MOUSE_MOVE_DIVISOR, SLICER_MOUSE_MAX_MOVE);
  } else {
    releaseSlicerMouseButtons();
  }

  if (mouseX != 0 || mouseY != 0 || wheel != 0) sendSlicerMouseReport(mouseX, mouseY, wheel);
}

// Optional Serial Plotter friendly debug output.
void debugPrintState(const int* values,int16_t tx, int16_t ty, int16_t tz, int16_t rx, int16_t ry, int16_t rz, uint32_t buttonMask) {
  if (!DEBUG_SERIAL) {
    return;
  }

  static unsigned long lastDebugAt = 0;
  unsigned long now = millis();

  if (now - lastDebugAt < DEBUG_SERIAL_INTERVAL_MS) {
    return;
  }

  lastDebugAt = now;

  Serial.print("mode:");
  Serial.print(currentSpeedMode);

  Serial.print(" buttons:");
  Serial.print(buttonMask);

  for (int i = 0; i < 8; i++) {
    Serial.print(" v");
    Serial.print(i);
    Serial.print(":");
    Serial.print(values[i]);
  }

  Serial.print(" tx:");
  Serial.print(tx);
  Serial.print(" ty:");
  Serial.print(ty);
  Serial.print(" tz:");
  Serial.print(tz);

  Serial.print(" rx:");
  Serial.print(rx);
  Serial.print(" ry:");
  Serial.print(ry);
  Serial.print(" rz:");
  Serial.println(rz);
}

// ============================================================================
// Main Setup & Loop
// ============================================================================

// Initializes HID, buttons, then calibrates the joystick center positions.
void setup() {
  // VID: 0x046D (Logitech), PID: 0xC62B (SpaceMouse Pro)
  // VID: 0x256F (3Dconnexion), PID: 0xC635 (SpaceMouse Compact)
  TinyUSBDevice.setID(0x046D, 0xC62B);
  TinyUSBDevice.setManufacturerDescriptor("3Dconnexion");
  TinyUSBDevice.setProductDescriptor("Orbit Pro");
  // ADC config 12 bits for ESP32-S3
  analogReadResolution(12);

  // TinyUSB Initialization 
  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();

  if (DEBUG_SERIAL) Serial.begin(DEBUG_SERIAL_BAUD);
  for (int i = 0; i < BUTTON_COUNT; i++) pinMode(buttonPins[i], INPUT_PULLUP);

  // Wait to device was connected
  while( !TinyUSBDevice.mounted() ) delay(1);

  // Short delay to let the board and sensors stabilize.
  delay(800);
  calibrateCenter();
}

// ============================================================================
// Main processing loop:
// 1. Read joystick values.
// 2. Subtract calibrated centers.
// 3. Apply input deadzone.
// 4. Calculate translation and rotation.
// 5. Detect Z push/pull and Z rotation.
// 6. Apply anti-drift rotation priority.
// 7. Apply gains, dominant axis filtering, deadzone, inversion and smoothing.
// 8. Send HID reports.
// ============================================================================
void loop() {
  int raw[8];
  int v[8];
  uint32_t buttonMask = readButtonMask();
  bool modeSwitchComboPressed = isModeSwitchComboPressed(buttonMask);
  bool slicerModeComboPressed = isSlicerModeComboPressed(buttonMask);
  bool modeSwitchComboAccepted = false;
  
  uint32_t hidButtonMask = filterModeSwitchButtons(buttonMask, modeSwitchComboPressed, modeSwitchComboAccepted);
  hidButtonMask = filterSlicerModeButtons(hidButtonMask, slicerModeComboPressed);

  updateSpeedMode(modeSwitchComboAccepted);
  updateSlicerMouseMode(slicerModeComboPressed);

  // 1. Read raw joystick values
  readAxes(raw);

  // 2. Convert raw values into movement values around zero.
  for (int i = 0; i < 8; i++) v[i] = raw[i] - center[i];

  // 3. Apply input deadzone
  applyInputDeadzone(v);

  // 4. Basic translation & rotation calculation
  int16_t transX = (v[5] - v[1]);
  int16_t transY = (v[7] - v[3]);
  int16_t transZ = 0;
  int16_t rotX = (v[4] - v[0]);
  int16_t rotY = (v[2] - v[6]);
  int16_t rotZ = 0;

  // 5. Detect Z push/pull and Z rotation.

  // Z PUSH / PULL DETECTION
  // If at least 3 sensors move in the same direction, the firmware considers
  // it a vertical push/pull movement.
  int zPushPull = v[0] + v[2] + v[4] + v[6];
  int zPosCount = countPositive4(v[0], v[2], v[4], v[6], DEADZONE_INPUT);
  int zNegCount = countNegative4(v[0], v[2], v[4], v[6], DEADZONE_INPUT);
  bool zDetected = ((zPosCount >= 3) || (zNegCount >= 3)) && (abs(zPushPull) > DEADZONE_INPUT * 2);
  
  if (zDetected) { 
    transZ = -zPushPull; 
    transX = 0; 
    transY = 0; 
  }
  
  // Z ROTATION DETECTION
  // If at least 3 side sensors move in the same direction, the firmware
  // interprets it as a twist around the Z axis.
  int zTwist = v[1] + v[3] + v[5] + v[7];
  int rzPosCount = countPositive4(v[1], v[3], v[5], v[7], DEADZONE_INPUT);
  int rzNegCount = countNegative4(v[1], v[3], v[5], v[7], DEADZONE_INPUT);
  bool rzDetected = ((rzPosCount >= 3) || (rzNegCount >= 3)) && (abs(zTwist) > DEADZONE_INPUT * 3);
  
  if (rzDetected) { 
    rotZ = zTwist / 2; 
    rotX = 0; 
    rotY = 0; 
  }

  // 6. Apply anti-drift rotation priority.

  // ROTATION PRIORITY
  // This reduces the issue where a rotation is accidentally interpreted as a
  // left/right or forward/back translation.

  int rotationPower = abs(rotX) + abs(rotY) + abs(rotZ);
  int translationPower = abs(transX) + abs(transY) + abs(transZ);
  
  // Adjusted for 12-bit (80 * 4 = 320)
  bool rotationMode = rotationPower > (320) && rotationPower > translationPower * ROTATION_PRIORITY;
  
  if (rotationMode) { 
    transX = 0; 
    transY = 0; 
    transZ = 0; 
    // Reset translation smoothing to avoid ghost movement.
    smoothTX = 0; 
    smoothTY = 0; 
    smoothTZ = 0; 
  }

  // 7. Apply gains, dominant axis filtering, deadzone, inversion and smoothing.

  // GAINS
  transX = applyGain(transX, GAIN_TX);
  transY = applyGain(transY, GAIN_TY);
  transZ = applyGain(transZ, GAIN_TZ);

  rotX = applyGain(rotX, GAIN_RX);
  rotY = applyGain(rotY, GAIN_RY);
  rotZ = applyGain(rotZ, GAIN_RZ);

  // DOMINANT AXIS FILTER
  if (ENABLE_DOMINANT_AXIS_FILTER) keepOnlyDominantAxis(transX, transY, transZ, rotX, rotY, rotZ);

  // RESPONSE CURVE
  float speedScale = SPEED_MODE_SCALE[currentSpeedMode];
  float responseCurve = SPEED_MODE_RESPONSE_CURVE[currentSpeedMode];

  // Adjusted for 12-bit: 1024 * 4 = 4096.0, 2048 * 4 = 8192.0
  transX = applyResponseCurve(transX, 4096.0 * GAIN_TX, speedScale, responseCurve);
  transY = applyResponseCurve(transY, 4096.0 * GAIN_TY, speedScale, responseCurve);
  transZ = applyResponseCurve(transZ, 8192.0 * GAIN_TZ, speedScale, responseCurve);

  rotX = applyResponseCurve(rotX, 4096.0 * GAIN_RX, speedScale, responseCurve);
  rotY = applyResponseCurve(rotY, 4096.0 * GAIN_RY, speedScale, responseCurve);
  rotZ = applyResponseCurve(rotZ, 4096.0 * GAIN_RZ, speedScale, responseCurve);

  // OUTPUT DEADZONE
  applyOutputDeadzone(transX, transY, transZ, rotX, rotY, rotZ);

  // AXIS INVERSION
  if (invX)  transX = -transX; 
  if (invY)  transY = -transY; 
  if (invZ)  transZ = -transZ;

  if (invRX) rotX   = -rotX;   
  if (invRY) rotY   = -rotY;   
  if (invRZ) rotZ   = -rotZ;

  // SMOOTHING 
  smoothTX = smoothValue(smoothTX, transX); 
  smoothTY = smoothValue(smoothTY, transY); 
  smoothTZ = smoothValue(smoothTZ, transZ);

  smoothRX = smoothValue(smoothRX, rotX);   
  smoothRY = smoothValue(smoothRY, rotY);   
  smoothRZ = smoothValue(smoothRZ, rotZ);

  // FINAL DEADZONE 
  int16_t outputTX = smoothTX; 
  int16_t outputTY = smoothTY; 
  int16_t outputTZ = smoothTZ;

  int16_t outputRX = smoothRX; 
  int16_t outputRY = smoothRY; 
  int16_t outputRZ = smoothRZ;
  applyOutputDeadzone(outputTX, outputTY, outputTZ, outputRX, outputRY, outputRZ);

  // 8. Send HID reports.
  if (slicerMouseModeEnabled) {
    sendCommand(0, 0, 0, 0, 0, 0);
    sendButtons(0);
    updateSlicerMouseButtons(buttonMask, modeSwitchComboPressed || slicerModeComboPressed);
    sendSlicerMouse(outputTX, outputTY, outputTZ, outputRX, outputRY, outputRZ);
  } else {
    releaseSlicerMouseButtons();
    sendCommand(outputRX, outputRZ, outputRY, outputTX, outputTZ, outputTY);
    sendButtons(hidButtonMask);
  }
}