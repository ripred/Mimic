// ====================================================================================================
// 
// PushButton library version 1.0
// written 2011 - trent m. wyatt
// 

#include <Arduino.h>
#include "ButtonLib2.h"

ButtonPressCallback bpcb = nullptr;

// ====================================================================================================
// Set up a specific input pin for use as a push button input.
// Note: The input pin will be configured to be pulled up by an internal
// 20K resistor to Vcc so fewer external parts are needed.  This means
// that a digitalRead(...) of an un-pressed button will read as HIGH
// and the digitalRead(...) of a pressed button (connected to GND on
// one side and the input pin on the Arduino) will read as LOW.
// 
void set_button_input(const char pin) {
  pinMode(pin, INPUT);      // set the button pin as an input
  digitalWrite(pin, HIGH);  // use the internal 20k pullup resistor to pull the pin HIGH by default
}


// ====================================================================================================
// 
// Get the state of a push button input
// Returns: true if the specified button was continuously pressed, otherwise returns false.
// 
// Note: The button must be continuously pressed (no jitter/makes/breaks) for at least as long as KEYDBDELAY
//       (key debounce delay). This smooths out the dozens of button connections/disconnections detected at
//       the speed of the CPU when the button first begins to make contact into a lower frequency responce.
// 
bool button_pressed(const char pin) {
  unsigned long int presstime = millis() + KEYDBDELAY;

  while (!digitalRead(pin)) {     // remember a pressed button returns LOW not HIGH
    if (millis() >= presstime) {
      return true;
    }
  }
  
  return false;
}


// ====================================================================================================
// 
// See if the user presses a button once, twice, or three times.  Also detect whether the user has held
// down the button to indicate a "long" press.  This is an enhanced  button press check
// (as opposed to button_pressed(...)) in that it attempts to detect gestures.
// 
char check_button_gesture(const char pin) {
  if (!button_pressed(pin)) {
    return NOT_PRESSED;
  }

  // The button is pressed.
  // Time how long the user presses the button to get the intent/gesture
  unsigned long int presstime = millis() + KEYLONGDELAY;  // get the current time in milliseconds plus the long-press offset
  while (button_pressed(pin)) {
    if (millis() >= presstime) {
      return SINGLE_PRESS_LONG;
    }
  }

  // check for double-tap/press
  // The user has released the button, but this might be a double-tap.  Check again and decide.
  presstime = millis() + ALLOWED_MULTIPRESS_DELAY;
  while (millis() < presstime) {
    if (button_pressed(pin)) {
      presstime = millis() + KEYLONGDELAY;
      while (button_pressed(pin)) {
        if (millis() >= presstime) {
          return DOUBLE_PRESS_LONG;
        }
      }

      // check for triple-tap/press
      // The user has released the button, but this might be a triple-tap.  Check again and decide.
      presstime = millis() + ALLOWED_MULTIPRESS_DELAY;
      while (millis() < presstime) {
        if (button_pressed(pin)) {
          presstime = millis() + KEYLONGDELAY;
          while (button_pressed(pin)) {
            if (millis() >= presstime) {
              return TRIPLE_PRESS_LONG;
            }
          }
          return TRIPLE_PRESS_SHORT;
        }
      }    
      return DOUBLE_PRESS_SHORT;
    }
  }
  return SINGLE_PRESS_SHORT;
}


// ====================================================================================================
// 
// This wrapper function is used to allow consistent return values for back-to-back calls of
// check_button_internal(...) while the button is continuously pressed.  Without this extra step
// DOUBLE_BUTTON_LONG and TRIPLE_BUTTON_LONG presses would return correctly on their first check but
// would then be reported as SINGLE_BUTTON_LONG after that if the user kept the button pressed.
// 
// Additionally this function prevents the spurious reporting of a *_BUTTON_SHORT state to be reported
// after the user has let go of a button once one or more *_BUTTON_LONG states have been observed.
// 
char check_button(const char pin, char& lastButtonState) {
  char state = check_button_gesture(pin);
  if (state & LONG_PRESS) {
    if (lastButtonState & LONG_PRESS) {
      state = LONG_PRESS | (lastButtonState & 0x0F);
      if (nullptr != bpcb) {
        bpcb(pin, state);
      }
      return state;
    }
  } else {
    if (lastButtonState & LONG_PRESS) {
      if (nullptr != bpcb) {
        bpcb(pin, NOT_PRESSED);
      }
      return lastButtonState = NOT_PRESSED;
    }
  }

  if (nullptr != bpcb) {
    bpcb(pin, state);
  }
  return lastButtonState = state;
}


// ====================================================================================================
// 
// example use:
// 
// ====================================================================================================

//#define EXAMPLE_SETUP_AND_LOOP
#ifdef EXAMPLE_SETUP_AND_LOOP

// NOTE: change/define the following pin(s) based on your project/connections
#define   BUTTON_PIN_1    2
char      Button1State = NOT_PRESSED;

#define   BUTTON_PIN_2    3
char      Button2State = NOT_PRESSED;


void setup(void) {
  Serial.end();
  Serial.begin(250000L);
  while (!Serial)
    ;
  Serial.flush();
  Serial.println(F("\n\nArduino Core Library - Button Library Test"));

  set_button_input(BUTTON_PIN_1);
  set_button_input(BUTTON_PIN_2);
}


void loop(void) {
  report_button(check_button(BUTTON_PIN_1, Button1State), "push button 1");
  report_button(check_button(BUTTON_PIN_2, Button2State), "push button 2");
}


void report_button(const char state, const char* const label = NULL)  {
  switch (state) {
    case SINGLE_PRESS_SHORT: Serial.print(F("Single button short press")); break;
    case SINGLE_PRESS_LONG:  Serial.print(F("Single button long  press")); break;
    case DOUBLE_PRESS_SHORT: Serial.print(F("Double button short press")); break;
    case DOUBLE_PRESS_LONG:  Serial.print(F("Double button long  press")); break;
    case TRIPLE_PRESS_SHORT: Serial.print(F("Triple button short press")); break;
    case TRIPLE_PRESS_LONG:  Serial.print(F("Triple button long  press")); break;
    default:
    case NOT_PRESSED:
      return;
  }
  if (NULL != label) {
    Serial.print(F(" on "));
    Serial.print(label);
  }
  Serial.println();
}
#endif  // #ifdef EXAMPLE_SETUP_AND_LOOP
