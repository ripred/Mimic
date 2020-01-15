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
|*| A jumper is included to allow single or dual voltage sources for logic and servos
|*| Two 18650's power the logic and servos
|*| Built in USB battery charger
|*| 
|*| Current features:
|*|  + Allows the output arm to mimic the input arm in real time.
|*|  + The mimic can be disabled
|*|  + It can "park" the output arm so it lays flat across to box top
|*|  + Movements can be recorded and played back
|*|  + Recorded movements can be stored to/from EEPROM
|*|  + Uses Button "Gestures" to multiplex the functionality of the single control button
|*|  + During playback the "pinch" potentiometer controls the playback speed
|*|  + Uses lightweight dynamic template based storage for recording, playback, and parking sequences
|*|  + (hardware) Added a brace to pressure the wrist servo shaft so it stays
|*|      pressed in (better: replace that servo)
|*| 
|*| Control by using the button with "gestures":
|*|   Single Click: ...............Toggle idle or mimic mode
|*|   Single Click and Hold: ......Enter Recording Mode:
|*|     Single Click: .............Add position
|*|     Single Click and hold: ....Exit recording mode
|*|   Double Click: ...............Enter Playback Mode:
|*|     Any button press: .........Exit playback mode
|*|   Double Click and Hold: ......Park the servo arm and save any recording to the EEPROM
|*| 
|*| TODO:
|*|  - Change on/off switch to DPDT to control both Vcc for logic and Vdd for servos
|*|  - Add googly eyes to servo arm :-)
|*|  - Add ability to play "Scissors/Rock/Paper" against the arm! :-)
|*|  - Add mic and op-amp to have dance-party mode!
|*|  - Enhance the output arm so it takes the position of each servo into account
|*|      when moving to a new set of positions so the deltas for each movement
|*|      can be evenly spread across a given amount of time. This will allow
|*|      further enhancement as a "playback speed" setting can then be added.
|*|  - Add a SoftSerial port and implement an API to allow external control
|*|  - Add HM-10 BlueTooth module to SoftSerial port for wireless control
|*|  - Use the serial API to drive the arm from my JavaChess repo
|*|  - Use the serial API to be able generate arm animations whe someone scores from my nhl/baseball feeds repo
|*| 
\*/

#include <EEPROM.h>
#include "mimic.h"
#include "InputArm.h"
#include "OutputArm.h"
#include "ButtonLib2.h"

// ---------------------------------------------------------------------------------
// Project specific pin connections
// Change to match your implementation

// bi-color LED (common anode)
#define LED1    2
#define LED2    4

// pushbutton (active low)
#define BUTTON  7

// There are 4 potentiometers in the input arm (each 5K or 10K, connected across logic Vcc and Gnd)
#define POT1    A0
#define POT2    A1
#define POT3    A2
#define POT4    A3

// There are 4 servos in the output arm
#define S1_PIN  3
#define S2_PIN  5
#define S3_PIN  6
#define S4_PIN  9

// ---------------------------------------------------------------------------------
// Global variables

//                     pinch open      wrist down    elbow forward    waist left
static Pos iRange1 = {     80,           650,           758,               87     };
//                     pinch closed    wrist up      elbow back       waist right
static Pos iRange2 = {    850,           100,               30,           660     };
//                     pinch open      wrist down    elbow forward    waist left
static Pos oRange1 = {     20,            10,                0,             7     };
//                     pinch closed    wrist up      elbow back       waist right
static Pos oRange2 = {     80,           165,              165,            180    };

static Limits iRange(iRange1, iRange2);
static Limits oRange(oRange1, oRange2);

static InputArm inArm(POT1, POT2, POT3, POT4, iRange);
static OutputArm outArm(S1_PIN, S2_PIN, S3_PIN, S4_PIN, iRange, oRange);
static LinkedList<Pos> saved;
static AppState appState;

// ---------------------------------------------------------------------------------

void setup() {
  initSerial();
  initLED();
  set_button_input(BUTTON);

// Uncomment to manually set up the potentiometer limits
//  setup_pot_values();

// Uncomment to manually set up the servo limits
//  setup_servo_values();

  // load last saved movements from EEPROM
  loadFromEeprom();

  setMode(IDLE);
}

void loop() {
  switch (getButton()) {
  
    // gesture to toggle the appState.mode
    case SINGLE_PRESS_SHORT:
      setMode((appState.mode + 1) % 2);
      break;

    // gesture to start recording
    case SINGLE_PRESS_LONG:
      outArm.attachServos();
      record();
      setMode(appState.mode);
      break;
  
    // gesture to start playback
    case DOUBLE_PRESS_SHORT:
      outArm.attachServos();
      playback(1000);
      setMode(appState.mode);
      break;

    // gesture for parking the arm
    case DOUBLE_PRESS_LONG:
      outArm.park();
      setMode(IDLE);
      waitForButtonRelease();
      flashLED(RED);
      saveToEeprom();
      break;
  }

  if (appState.mode == MIMIC) {
    mimic();
  }
}

// ==============================================================
// Utility functions

void setMode(int m) {
  appState.mode = m;
  setLED(appState.mode + 1);
  switch (appState.mode) {
    case IDLE:
    outArm.detachServos();
    break;

    case MIMIC:
    outArm.attachServos();
    break;
  }
}

// ==============================================================
// Record and playback functions

void record() {
  int button;
  saved.clear();
  flashLED(GREEN);
  waitForButtonRelease();
  setLED(ORANGE);

  do {
    mimic();
    button = getButton();
    switch (button) {
      case SINGLE_PRESS_SHORT:
        saved.addTail(outArm);
        setLED(GREEN);
        delay(50);
        setLED(ORANGE);
        break;
    }
  } while (button != SINGLE_PRESS_LONG);

  setLED(OFF);
  waitForButtonRelease();
}


void playback(int mS) {
  if (saved.empty()) {
    flashLED(RED);
    return;
  }

  setLED(GREEN);
  appState.stopPlayback = false;
  appState.playbackPause = mS;
  while (!appState.stopPlayback) {
    saved.foreach([](Pos &p) -> int {
      if (!appState.stopPlayback) {
        outArm = p;
        outArm.write();
        setLED(RED);
        if (0 != appState.playbackPause) {
          unsigned long now = millis();
          while (millis() < now + appState.playbackPause) {
            if (getButton() != NOT_PRESSED) {
              appState.stopPlayback = true;
              return 1;
            }
            appState.playbackPause = map(inArm.readPinch(), iRange2.pinch, iRange1.pinch, 400, 3000);
          }
        }
        setLED(GREEN);
      }
      return 0;
    });
  }

  setLED(OFF);
  waitForButtonRelease();
}

// ==============================================================
// EEPROM functions

void saveToEeprom() {
  static int eepromCount;
  eepromCount = 0;
  // use C++11 lambda function to save each position
  saved.foreach(
    // Lambda expression begins
    [](Pos &r) -> int {
      EEPROM.put(sizeof(eepromCount) + eepromCount * sizeof(Pos), r);
      eepromCount++;
      return 0;
    } // end of lambda expression
  );
  EEPROM.put(0, eepromCount);
}


void loadFromEeprom() {
  saved.clear();
  int eepromCount = 0;
  EEPROM.get(0, eepromCount);
  for (int i=0; i < eepromCount; i++) {
    Pos p;
    EEPROM.get(sizeof(eepromCount) + i * sizeof(Pos), p);
    saved.addTail(p);
  }
}

// ==============================================================
// mimic function

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
void setLED(int color) {
  appState.ledColor = color;
  digitalWrite(LED1, (color & 1) ? LOW : HIGH);
  digitalWrite(LED2, (color & 2) ? LOW : HIGH);
}

void flashLED(int color, int color2, int count, int timing, bool restore) {
  int origColor = appState.ledColor;
  while (count-- != 0) {
    setLED(color);
    delay(timing);
    setLED(color2);
    delay(timing);
  }
  if (restore)
    setLED(origColor);
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

void waitForButtonRelease() {
  while (!digitalRead(BUTTON))
    delay(5);
}

// ==============================================================
// Manual calibration functions

void setup_pot_values() {
  while (1) {
    char buff[128] = "";
    sprintf(buff, "pinch = %3d  wrist = %3d  elbow = %3d  waist = %3d",
      analogRead(A0), 
      analogRead(A1), 
      analogRead(A2), 
      analogRead(A3)
      );
    Serial.println(buff);
  }
}

void setup_servo_values() {
  outArm.attachServos();
  outArm.writePinch(20);   // 80 = closed, 20 = open
  outArm.writeWrist(150);  // back = 165, forward = 10, straight = 150
  outArm.writeElbow(17);   // back = 165, forward = 1, vertical = 95, horizontal = 17
  outArm.writeWaist(101);  // left = 7, right = 180, forward = 101

  while (1);
}
