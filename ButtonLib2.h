// ====================================================================================================
// 
// PushButton library version 1.0
// written 2011 - trent m. wyatt
// 

#ifndef BUTTONLIB2_INCL
#define BUTTONLIB2_INCL

// ====================================================================================================
// 
// manifest constants for button library
// 
#define  NOT_PRESSED         0x00

#define  SINGLE_BUTTON       0x01
#define  DOUBLE_BUTTON       0x02
#define  TRIPLE_BUTTON       0x04

#define  SHORT_PRESS         0x10
#define  LONG_PRESS          0x20

#define  SINGLE_PRESS_SHORT (SINGLE_BUTTON | SHORT_PRESS)
#define  SINGLE_PRESS_LONG  (SINGLE_BUTTON | LONG_PRESS)
#define  DOUBLE_PRESS_SHORT (DOUBLE_BUTTON | SHORT_PRESS)
#define  DOUBLE_PRESS_LONG  (DOUBLE_BUTTON | LONG_PRESS)
#define  TRIPLE_PRESS_SHORT (TRIPLE_BUTTON | SHORT_PRESS)
#define  TRIPLE_PRESS_LONG  (TRIPLE_BUTTON | LONG_PRESS)

#define  KEYDBDELAY               36                // original bell labs standard phone push button debounce delay in mS
#define  KEYLONGDELAY             (KEYDBDELAY * 20) // how long to consider a pressed button a "long press" versus a "short press"
#define  ALLOWED_MULTIPRESS_DELAY (KEYDBDELAY * 7)  // the amount of time allowed between multiple "taps" to be considered part of the last "tap"

typedef void (*ButtonPressCallback)(const char pin, const char state);

// ====================================================================================================
// Set up a specific input pin for use as a push button input.
// Note: The input pin will be configured to be pulled up by an internal
// 20K resistor to Vcc so fewer external parts are needed.  This means
// that a digitalRead(...) of an un-pressed button will read as HIGH
// and the digitalRead(...) of a pressed button (connected to GND on
// one side and the input pin on the Arduino) will read as LOW.
// 
void set_button_input(const char pin);


// ====================================================================================================
// 
// Get the state of a push button input
// Returns: true if the specified button was continuously pressed, otherwise returns false.
// 
// Note: The button must be continuously pressed (no jitter/makes/breaks) for at least as long as KEYDBDELAY
//       (key debounce delay). This smooths out the dozens of button connections/disconnections detected at
//       the speed of the CPU when the button first begins to make contact into a lower frequency responce.
//       This technique is known as "key de-bouncing".
// 
bool button_pressed(const char pin);


// ====================================================================================================
// 
// See if the user presses a button once, twice, or three times.  Also detect whether the user has held
// down the button to indicate a "long" press.  This is an enhanced  button press check
// (as opposed to button_pressed(...)) in that it attempts to detect gestures.
// 
char check_button_gesture(const char pin);


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
char check_button(const char pin, char& lastButtonState);
#endif // #ifndef BUTTONLIB2_INCL
