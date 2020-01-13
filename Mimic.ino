/*\
|*| "Mimic" project
|*| 
|*| Created January 2020 by Trent M. Wyatt
|*| 
|*| Box with two arms, each with 4 degrees of freedom: Waist, Elbow, Wrist, and Pincher.
|*| The first arm has a 5K or 10K potentiometer at each joint and the other arm has a servo at each joint.
|*| The first arm is the input arm, reading the 4 analog values from the pots
|*| The second arm is the output arm that can mimic the movements of the input arm
|*| A pushbutton and a bi-color LED are provided as input and output for setup, mode selection etc.
|*| An on/off switch is also included.
|*| 
|*| Current features:
|*|  + allows the output arm to mimic the input arm in real time.
|*|  + allows the mimic to be disabled
|*|  + can "park" the output arm so it lays flat across to box top
|*| 
|*| TODO:
|*|  - Allow movements to be recorded (to EEPROM) and played back.
|*|  - Enhance the output arm so it takes the position of each servo into account
|*|      when moving to a new set of positions so the deltas for each movement
|*|      can be evenly spread across a given amount of time. This will allow
|*|      further enhancement as a "playback speed" setting can then be added.
|*|  - Add a SoftSerial port and implement an API to allow external control
|*|  - (hardware) Add a brace to pressure the wrist servo shaft so it stays
|*|      pressed in (better: replace that servo)
|*|  + Modify the potentiometer reading so it keeps a running average
|*|      of the last N samples to smooth out minor drifting
|*|  + Modify button library to add a setting to tighten or loosen the 
|*|      timing used to determine single, double, and triple clicks
|*| 
\*/

#include "mimic.h"
#include "InputArm.h"
#include "OutputArm.h"
#include "ButtonLib2.h"

void playback(int mS = 0);

// ---------------------------------------------------------------------------------
// Project specific pin connections
// Change to match implementation

// bi-color LED (common anode)
#define LED1    2
#define LED2    4

// pushbutton (active low)
#define BUTTON  7

// 4 potentiometers in input arm (each 5K or 10K, connected across Vcc and Gnd)
#define POT1    A0
#define POT2    A1
#define POT3    A2
#define POT4    A3

// 4 servos in output arm
#define S1_PIN  3
#define S2_PIN  5
#define S3_PIN  6
#define S4_PIN  9

// ---------------------------------------------------------------------------------
// Global variables

//                     pinch open      wrist down    elbow forward    waist left
static Arm iRange1 = {     80,           650,           758,               87     };
//                     pinch closed    wrist up      elbow back       waist right
static Arm iRange2 = {    850,           100,               30,           660     };
//                     pinch open      wrist down    elbow forward    waist left
static Arm oRange1 = {     20,            10,                0,             7     };
//                     pinch closed    wrist up      elbow back       waist right
static Arm oRange2 = {     80,           165,              165,            180    };

static Limits iRange(iRange1, iRange2);
static Limits oRange(oRange1, oRange2);

static InputArm inArm(POT1, POT2, POT3, POT4, iRange);
static OutputArm outArm(S1_PIN, S2_PIN, S3_PIN, S4_PIN, iRange, oRange);
static int mode = 0;
static Tree<Pos> saved;
static int playbackPause = 400;
bool stopPlayback = false;
// ---------------------------------------------------------------------------------


void setup() {
  initSerial();
  initLED();
  set_button_input(BUTTON);

// Uncomment to manually set up the potentiometer limits
//  setup_pot_values();

  outArm.attachServos();

// Uncomment to manually set up the servo limits
//  setup_servo_values();

  setLED(RED);
}

void loop() {
  switch (getButton()) {
    case DOUBLE_PRESS_LONG:
      mode = 1;
      outArm.park();
      outArm.detachServos();
      while (digitalRead(BUTTON) == LOW)
        ;
  
      for (int v=0; v < 5; v++) {
        setLED(RED);
        delay(250);
        setLED(OFF);
        delay(250);
      }
      break;
  
    case SINGLE_PRESS_SHORT:
      mode = (mode + 1) % 2;
      setLED(mode + 1);
  
      if (mode == 1)
        outArm.detachServos();
      else if (mode == 0)
        outArm.attachServos();
      break;
  
    case DOUBLE_PRESS_SHORT:
      playback(1000);
      break;

    case SINGLE_PRESS_LONG:
      record();
      break;
  }

  if (mode == 0) {
    mimic();
  }
}


void record() {
  Serial.println("record");
  int button;
  saved.clear();
  for (int i=0; i < 5; i++) {
    setLED(OFF);
    delay(200);
    setLED(GREEN);
    delay(100);
  }

  setLED(RED);

  do {
    mimic();
    button = getButton();
    switch (button) {
      case SINGLE_PRESS_SHORT:
        saved.addTail(outArm);
        setLED(GREEN);
        delay(250);
        setLED(RED);
        break;
    }
  } while (button != SINGLE_PRESS_LONG);

  setLED(GREEN);

  while (!digitalRead(BUTTON))
    delay(10);

  Serial.println("record done");
  setLED(RED);
}


void play(Pos &p) {
  Serial.println("play");
  outArm = p;
  outArm.write();
  setLED(RED);
  if (0 != playbackPause) {
    unsigned long timer = millis() + playbackPause;
    while (millis() < timer) {
      if (getButton() != NOT_PRESSED) {
        stopPlayback = true;
        break;
      }
    }
  }
  setLED(GREEN);
}


void playback(int mS) {
  Serial.println("playback");
  if (saved.empty()) {
    for (int i=0; i < 5; i++) {
      setLED(OFF);
      delay(100);
      setLED(RED);
      delay(100);
    }
    return;
  }

  setLED(GREEN);
  playbackPause = mS;
  while (1) {
    playbackPause = map(inArm.readPinch(), iRange2.pinch, iRange1.pinch, 400, 3000);
    if (getButton() != NOT_PRESSED || stopPlayback) {
      stopPlayback = false;
      break;
    }
    saved.foreach(play);
  }
  setLED(RED);
  Serial.println("playback done");
}


void setup_pot_values() {
  while (1) {
    char buff[128] = "";
    sprintf(buff, "pinch = %3d    wrist = %3d    elbow = %3d    waist = %3d", 
      analogRead(A0), 
      analogRead(A1), 
      analogRead(A2), 
      analogRead(A3)
      );
    Serial.println(buff);
  }
}


void setup_servo_values() {
  outArm.writePinch(20);   // 80 = closed, 20 = open
  outArm.writeWrist(150);  // back = 165, forward = 10, straight = 150
  outArm.writeElbow(17);   // back = 165, forward = 1, vertical = 95, horizontal = 17
  outArm.writeWaist(101);  // left = 7, right = 180, forward = 101

  while (1);
}


void mimic() {
  outArm = inArm.read();
  outArm.write();
}


// ==============================================================
// Serial functions

void initSerial() {
  Serial.end();
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.flush();
  Serial.println();
}

// ==============================================================
// LED functions

void initLED() {
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
}

// set the red/green LED value
// 0 = off
// 1 = red
// 2 = green
// 3 = orange
// 
void setLED(int value) {
  digitalWrite( LED1, (value & 1) ? LOW : HIGH);
  digitalWrite( LED2, (value & 2) ? LOW : HIGH);
}

// ==============================================================
// Button functions

// Get a button gesture if available
// 
// Armsible return values are:
// 
//    NOT_PRESSED
//    SINGLE_PRESS_SHORT
//    SINGLE_PRESS_LONG
//    DOUBLE_PRESS_SHORT
//    DOUBLE_PRESS_LONG
//    TRIPLE_PRESS_SHORT
//    TRIPLE_PRESS_LONG
// 
int getButton() {
  static char buttonState = NOT_PRESSED;
  int result = check_button(BUTTON,  buttonState);
  return result;
}

void report_button(const char state)  {
  switch (state) {
    case SINGLE_PRESS_SHORT: Serial.println(F("Single button short press")); break;
    case SINGLE_PRESS_LONG:  Serial.println(F("Single button long  press")); break;
    case DOUBLE_PRESS_SHORT: Serial.println(F("Double button short press")); break;
    case DOUBLE_PRESS_LONG:  Serial.println(F("Double button long  press")); break;
    case TRIPLE_PRESS_SHORT: Serial.println(F("Triple button short press")); break;
    case TRIPLE_PRESS_LONG:  Serial.println(F("Triple button long  press")); break;
    default:
    case NOT_PRESSED:
      return;
  }
}


// Move the referenced servo to the specified position
// and then wait for the user to set the potentiometer
// specified in 'inpin' to the correct position and
// then press the pushbutton to set it
// 
uint16_t setPot(Servo& s, uint8_t pos, int inpin) {
  UNUSED(s);
  UNUSED(pos);

//  s.write(pos);
//
  uint16_t value = analogRead(inpin);
//  while(getButton() == 0) {
//    value = analogRead(inpin);
//  }
//
  return value;
}
