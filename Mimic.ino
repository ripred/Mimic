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
|*|  + Servos are controlled using the Servo::writeMicroseconds(uS) method for more accuracy per servo
|*|  + Allows the output arm to mimic the input arm in real time.
|*|  + The mimic can be disabled
|*|  + The output arm can be "parked" so it lays flat across to box top
|*|  + Movements can be recorded and played back
|*|  + Recorded movements can be stored to/from EEPROM
|*|  + Uses Button "gestures" to multiplex the functionality of the single control button
|*|  + SoftwareSerial port gives API to allow external read and write of input and output arms
|*|  + SoftwareSerial API includes support for controlling playback, recording, and EEPROM storage
|*|  + During playback the "pinch" potentiometer controls the playback speed
|*|  + Uses a lightweight template class for storage of recording, playback, and parking sequences
|*|  + (hardware) Added a brace to pressure the wrist servo shaft so it stays
|*|      pressed in (better: replace that servo)
|*|  + (hardware) Changed on/off switch to DPDT to control both Vcc for logic and Vdd for servos
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
|*|  + Added readVcc() function to determine the Vcc being used.  This affects the potential
|*|    across the potentiometers and gives different ranges of values depending on if Vcc
|*|    is 5V (when powered by USB cable) vs when using a battery.
|*|  - Use voltageRead() function to calibrate values read from the input arm so they are 
|*|    the same whether using 5V USB power or battery
|*| 
|*|  - Add inverse-kinetics functions to control pincher endpoint position using 3D cartesian coordinates
|*|  - Enhance the output arm so it takes the position of each servo into account
|*|      when moving to a new set of positions so the deltas for each movement
|*|      can be evenly spread across a given amount of time. This will allow
|*|      further enhancement as a "playback speed" setting can then be added.
|*| 
|*|  -!See if having a keywords.txt file in current folder can be used
|*|  - Add googly eyes to servo arm :-)
|*|  - Add ability to play "Scissors/Rock/Paper" against the arm! :-)
|*|  - Add mic and op-amp to have dance-party mode!
|*|  - Add HM-10 BlueTooth module to SoftSerial port for wireless control
|*|  - Use the serial API to drive the arm from my JavaChess repo
|*|  - Use the serial API to be able generate arm animations whe someone scores from my nhl/baseball feeds repo
|*| 
\*/

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <string.h>
#include "mimic.h"
#include "InputArm.h"
#include "OutputArm.h"
#include "ButtonLib2.h"

#define DEBUG_API

// ---------------------------------------------------------------------------------
// Project specific pin connections
// Change to match your implementation

// bi-color LED (common anode)
#define LED1          2
#define LED2          4

// pushbutton (active low)
#define BUTTON        7

// There are 4 potentiometers in the input arm (each 5K or 10K, connected across logic Vcc and Gnd)
#define POT1          A0
#define POT2          A1
#define POT3          A2
#define POT4          A3

// There are 4 servos in the output arm
#define S1_PIN        3
#define S2_PIN        5
#define S3_PIN        6
#define S4_PIN        9

// SoftSerial port for serial control
#define SSERIAL_RX    10
#define SSERIAL_TX    8

// ---------------------------------------------------------------------------------
// Global variables

// Potentiometer Ranges
//                     pinch open      wrist down    elbow forward    waist left
static Pos iRange1 = {     80,           650,            758,             87    };
//                     pinch closed    wrist up      elbow back       waist right
static Pos iRange2 = {    850,           100,             30,            660    };


// Servo Ranges - **BY MICROSECONDS**
//                     pinch open      wrist down    elbow forward    waist left
static Pos oRange1 = {    800,           650,            550,            550    };
//                     pinch closed    wrist up      elbow back       waist right
static Pos oRange2 = {   1600,          2300,           2280,           2365    };

static Limits iRange(iRange1, iRange2);
static Limits oRange(oRange1, oRange2);

static SoftwareSerial sserial(SSERIAL_RX, SSERIAL_TX);
static InputArm inArm(POT1, POT2, POT3, POT4, iRange);
static OutputArm outArm(S1_PIN, S2_PIN, S3_PIN, S4_PIN, oRange);
static LinkedList<Pos> saved;
static AppState appState;

// ---------------------------------------------------------------------------------

void setup() {
  initSerial();
  initSSerial(9600);
  initLED();
  setLED(OFF);
  set_button_input(BUTTON);

// Uncomment to manually set up the potentiometer limits
//  setup_pot_values();

// Uncomment to manually set up the servo limits
//  setup_servo_values();

  // load last saved movements from EEPROM
  loadFromEeprom();

  // test timed moves
//  outArm.setMode(IncrementTime);
//  outArm.attach();
//  outArm.delay(500);
//
//  Pos pos(outArm.pinch,    650, 2000, 1582);
//  outArm.write(pos, 1000, true);
//
//  pos = Pos(outArm.pinch, 2100, 1450, 1582);
//  outArm.write(pos, 750, true);
//
//  pos = Pos(outArm.pinch,  650, 1650,  650);
//  outArm.write(pos, 750, true);
//
//  pos = Pos(outArm.pinch,  650, 2000, 1582);
//  outArm.write(pos, 750, true);
//  outArm.detach();

  outArm.setMode(IncrementHalf);

  setMode(IDLE);
}


void loop() {
  processSSerial();

  switch (getButton()) {
    // gesture to toggle the appState.mode
    case SINGLE_PRESS_SHORT:
      toggleMode();
      break;

    // gesture to start recording
    case SINGLE_PRESS_LONG:
      startRecord();
      break;
  
    // gesture to start playback
    case DOUBLE_PRESS_SHORT:
      startPlayback();
      break;

    // gesture for parking the arm
    case DOUBLE_PRESS_LONG:
      parkArm();
      break;
  }

  if (appState.mode == MIMIC) {
    mimic();
  }
}


void emulateSApi() {
  if (Serial.available() == 0) return;

  if (outArm.pinch + outArm.wrist + outArm.elbow + outArm.waist == 0) {
    // Start out in known valid position
    Pos startPos(1050, 2100, 1450, 1582);
    outArm.attach();
    outArm.write(startPos);
  }
  outArm.attach();

  // delay until number of serial bytes available settles:
  int lastAvailable;
  do {
    lastAvailable = Serial.available();
    delayMicroseconds(40000);
  } while (Serial.available() != lastAvailable);

  SerialPacket pkt;
  char buff[16];

  memset(buff, 0, sizeof(buff));
  for (unsigned i=0; Serial.available() > 0 && i < sizeof(buff); i++) {
    char c = Serial.read();
    buff[i] = c >= ' ' ? c : 0;
  }
  buff[sizeof(buff) - 1] = 0;

  switch(buff[0]) {
    case 'A':   // write waist
    case 'B':   // write elbow
    case 'C':   // write wrist
    case 'D':   // wrist pinch
    case 'a':   // read waist
    case 'b':   // read elbow
    case 'c':   // read wrist
    case 'd':   // read pinch
    case 'R':   // Read saved from EEPROM 
    case 'W':   // Write saved to EEPROM
    case 'X':   // Clear saved
    case 'Y':   // Add position to saved
    case 'P':   // Playback
    case 'Z':   // Stop playback
    case 'p':   // Park arm
    case 'M':   // Set Mode

      pkt.fields.cmd = buff[0];
      pkt.fields.value = atoi(buff + 1);
      processPacket(pkt);
      break;
  }
}


// ==============================================================
// main app functions

void toggleMode() {
  if (appState.mode == IDLE) {
    setMode(MIMIC);
  } else 
  if (appState.mode == MIMIC) {
    setMode(IDLE);
  }
}

void startPlayback() {
  outArm.attach();
  playback(1000);
  setMode(appState.mode);
}

void startRecord() {
  outArm.attach();
  record();
  setMode(appState.mode);
}

void parkArm() {
  outArm.park();
  setMode(IDLE);
  waitForButtonRelease();
  flashLED(GREEN);

  // NOTE: we do this so if the user hits the button without turning the arm off
  // then we will naturally toggle the mode, in this case to the safe IDLE state
  appState.mode = MIMIC;
}

// ==============================================================
// mimic function

void mimic(void) {
  outArm = inArm.read();
  outArm.write();
}

// ==============================================================
// Utility functions

void setMode(int m) {
  appState.mode = m;
  switch (appState.mode) {
    case IDLE:
    setLED(GREEN);
    outArm.detach();
    break;

    case MIMIC:
    setLED(RED);
    outArm.attach();
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
    processSSerial();
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

  flashLED(RED);
  saveToEeprom();

  setMode(IDLE);
}


void playback(int mS) {
  if (saved.empty()) {
    flashLED(RED);
    return;
  }

  setLED(GREEN);
  appState.stopPlayback = 0;
  appState.playbackPause = mS;

  Node<Pos> *ptr = saved.head;
  while ((ptr != nullptr) && (appState.stopPlayback == 0)) {
    outArm = ptr->t;
    outArm.write();
    setLED(RED);
    unsigned long now = millis();
    while ((millis() < now + appState.playbackPause) && (appState.stopPlayback == 0)) {
      processSSerial();
      if (getButton() != NOT_PRESSED) {
        appState.stopPlayback = 1;
        break;
      }
      appState.playbackPause = map(inArm.readPinch(), iRange2.pinch, iRange1.pinch, 400, 1500);
    }
    ptr = ptr->next;
  }

  setLED(OFF);
  waitForButtonRelease();
  setMode(IDLE);
}

// ==============================================================
// EEPROM functions



//    Sketch uses 12708 bytes (41%) of program storage space. Maximum is 30720 bytes.
//    Global variables use 508 bytes (24%) of dynamic memory, leaving 1540 bytes for local variables. Maximum is 2048 bytes.
// 
// 
//    Sketch uses 12604 bytes (41%) of program storage space. Maximum is 30720 bytes.
//    Global variables use 496 bytes (24%) of dynamic memory, leaving 1552 bytes for local variables. Maximum is 2048 bytes.
// 
// 
//    Sketch uses 12656 bytes (41%) of program storage space. Maximum is 30720 bytes.
//    Global variables use 496 bytes (24%) of dynamic memory, leaving 1552 bytes for local variables. Maximum is 2048 bytes.
// 
// 
//    Sketch uses 12660 bytes (41%) of program storage space. Maximum is 30720 bytes.
//    Global variables use 496 bytes (24%) of dynamic memory, leaving 1552 bytes for local variables. Maximum is 2048 bytes.
// 
// 
//    Sketch uses 14364 bytes (46%) of program storage space. Maximum is 30720 bytes.
//    Global variables use 530 bytes (25%) of dynamic memory, leaving 1518 bytes for local variables. Maximum is 2048 bytes.


void saveToEeprom() {
  int count = 0;
  Node<Pos> *ptr = saved.head;
  while (ptr != nullptr) {
    EEPROM.put(sizeof(count) + count * sizeof(Pos), ptr->t);
    ptr = ptr->next;
    count++;
  }
  EEPROM.put(0, count);
}

void loadFromEeprom() {
  int eepromCount = 0;
  Pos p;

  saved.clear();
  EEPROM.get(0, eepromCount);

  for (int i=0; i < eepromCount; i++) {
    EEPROM.get(sizeof(eepromCount) + i * sizeof(Pos), p);
    saved.addTail(p);
  }
}

// ==============================================================
// Hardware Serial functions

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

void flashLED(LedColor color, LedColor color2, int count, int timing, bool restore) {
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
  return check_button(BUTTON,  buttonState);
}

void waitForButtonRelease() {
  while (!digitalRead(BUTTON))
    delay(5);
}

// ==============================================================
// Software Serial control port functions

void initSSerial(int baud) {
  sserial.end();
  sserial.begin(baud);
  while (!sserial)
    ;
  sserial.flush();
}


void processPacket(SerialPacket &pkt) {
  switch (pkt.fields.cmd) {

    // set output waist                   // Write Output Arm API
    case 'A':
      outArm.waist = pkt.fields.value;
      outArm.write();
      break;

    // set output elbow
    case 'B':
      outArm.elbow = pkt.fields.value;
      outArm.write();
      break;

    // set output wrist
    case 'C':
      outArm.wrist = pkt.fields.value;
      outArm.write();
      break;

    // set output pinch
    case 'D':
      outArm.pinch = pkt.fields.value;
      outArm.write();
      break;

    // get input waist                    // Read Input Arm API
    case 'a':
      inArm.read();
      sserial.write('a');
      sserial.write(inArm.waist & 0xFF);
      sserial.write((inArm.waist >> 8) & 0xFF);
      break;

    // get input elbow
    case 'b':
      inArm.read();
      sserial.write('b');
      sserial.write(inArm.elbow & 0xFF);
      sserial.write((inArm.elbow >> 8) & 0xFF);
      break;

    // get input wrist
    case 'c':
      inArm.read();
      sserial.write('c');
      sserial.write(inArm.wrist & 0xFF);
      sserial.write((inArm.wrist >> 8) & 0xFF);
      break;

    // get input pinch
    case 'd':
      inArm.read();
      sserial.write('d');
      sserial.write(inArm.pinch & 0xFF);
      sserial.write((inArm.pinch >> 8) & 0xFF);
      break;

    // Clear all recorded positions     // Playback / Record API
    case 'X':
      saved.clear();
      break;

    // Add current output arm position to recorded positions
    case 'Y':
      saved.addTail(outArm);
      break;

    // Write recorded positions to EEPROM
    case 'W':
      saveToEeprom();
      break;

    // Read recorded positions from EEPROM
    case 'R':
      loadFromEeprom();
      break;

    // Start playback of recorded positions
    case 'P':
      startPlayback();
      break;

    // Stop playback
    case 'Z':
      appState.stopPlayback = 1;
      break;

    // Park arm
    case 'p':
      parkArm();
      break;

    // Set mode
    case 'M':
      setMode(pkt.fields.value);
      break;
  }
}

void processSSerial() {
#ifdef DEBUG_API
  emulateSApi();
#endif  

  if (sserial.available() < (int) sizeof(SerialPacket))
    return;

  SerialPacket pkt;

  for (unsigned i=0; i < sizeof(SerialPacket); i++)
    pkt.data[i] = sserial.read();

  // pkt.data[0] := command byte
  // pkt.data[1] := command value LSB (if 'set' command, otherwise ignored for 'get' commands)
  // pkt.data[2] := command value MSB (if 'set' command, otherwise ignored for 'get' commands)

  processPacket(pkt);
}


// ==============================================================
// Voltage reading function

//
// Function to use the internal registers in the ATMega cpu to calculate
// the voltage on Vin:
//
long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif 

  delay(2);                         // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);              // Start conversion
  while (bit_is_set(ADCSRA,ADSC));  // measuring

  uint8_t low  = ADCL;              // must read ADCL first - it then locks ADCH 
  uint8_t high = ADCH;              // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result;       // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result;                    // Vcc in millivolts
}

// ==============================================================
// Manual calibration functions

void setup_pot_values() {
  while (1) {
    char buff[64] = "";
    sprintf(buff, "pinch = %3d  wrist = %3d  elbow = %3d  waist = %3d",
      analogRead(A0), 
      analogRead(A1), 
      analogRead(A2), 
      analogRead(A3)
      );
    Serial.println(buff);
  }
}


/**
 * Manually set each servo position one at a time.
 * Allows for setting min and max ranges for each servo in the output arm
 * 
 */
void setup_servo_values() {
  setLED(RED);

  // Do one servo at a time so we can keep all of them detached (hopefully lower power use)
  outArm.detach();

  Serial.println(F("Manual Servo Calibration\n"));
  Serial.println(F("Adjust by sending < or D for down, > or U for up.\n"));

  struct CurServo {
    int uS;
    int usLast;
    char *name = nullptr;
    Servo &servo = outArm.elbowServo;
    int pin = outArm.elbowPin;

    CurServo()  = delete;

    CurServo(Servo &servo, int pin, const char *name, int value = 1500, bool detach = false) {
      this->pin = pin;
      this->name = const_cast<char *>(name);
      this->servo = servo;

      this->uS = value;
      this->usLast = 0;

      this->servo.detach();
      this->servo.attach(pin);
      this->uS = value;
      this->usLast = 0;
      this->servo.writeMicroseconds(uS);
      if (detach) {
        unsigned long timer = millis() + 350UL;
        while (millis() < timer)
          ;
        this->servo.detach();
      }
    }
  };

  CurServo servo0(outArm.waistServo, outArm.waistPin, "waist", 1582, true);
  CurServo servo1(outArm.elbowServo, outArm.elbowPin, "elbow", 1450, true);
  CurServo servo2(outArm.pinchServo, outArm.pinchPin, "pinch", 1050, true);
  CurServo servo(outArm.wristServo, outArm.wristPin, "wrist", 1170);

  while (1) {
    if (servo.uS != servo.usLast) {
      setLED(GREEN);
      servo.servo.writeMicroseconds(servo.usLast = servo.uS);
      Serial.print(F("Setting "));
      Serial.print(servo.name);
      Serial.print(F(" to "));
      Serial.print(servo.uS, DEC);
      Serial.println(F(" micro seconds"));
      unsigned long timer = millis() + 500;
      while (millis() < timer) 
        ;
      setLED(RED);
    }

    char buff[32] = "";
    if (Serial.available() >= 2) {
      unsigned long timer = millis() + 500;
      while (millis() < timer) 
        ;
      int index = 0;
      while (Serial.available() > 0) {
        buff[index++] = Serial.read();
      }

      char cmd = buff[0];
      switch (cmd) {
        case ',':
        case 'd':
          servo.uS -= 1;
          break;

        case '<':
        case 'D':
          servo.uS -= 10;
          break;

        case '.':
        case 'u':
          servo.uS += 1;
          break;

        case '>':
        case 'U':
          servo.uS += 10;
          break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          servo.uS = atoi(buff);
          break;
      }
    }
  }
}
