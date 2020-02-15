#ifndef OUTPUT_ARM_H_INCL
#define OUTPUT_ARM_H_INCL

#include <Servo.h>
#include "mimic.h"


enum UpdateMode : unsigned { Immediate, Increment1, IncrementHalf, IncrementTime };

class OutputArm : public Arm {
private:
  UpdateMode mode;

public:

  Servo pinchServo, wristServo, elbowServo, waistServo;
  Pos last, target;
  float pinchInc, wristInc, elbowInc, waistInc;
  float pinchPos, wristPos, elbowPos, waistPos;
  uint32_t lastUpdate;

  OutputArm(void) = delete;

  OutputArm(int pinch_pin, int wrist_pin, int elbow_pin, int waist_pin, Limits &limits) :
    Arm(pinch_pin, wrist_pin, elbow_pin, waist_pin, limits) {
    pinMode(pinchPin, OUTPUT);
    pinMode(wristPin, OUTPUT);
    pinMode(elbowPin, OUTPUT);
    pinMode(waistPin, OUTPUT);

    mode = Immediate;

    pinchPos = target.pinch = pinch = range.a.pinch;  // set pincher to wide open, not the midpoint like the others
    wristPos = target.wrist = wrist = ((range.b.wrist - range.a.wrist) / 2) + range.a.wrist;
    elbowPos = target.elbow = elbow = ((range.b.elbow - range.a.elbow) / 2) + range.a.elbow;
    waistPos = target.waist = waist = 1582;

    pinchInc = wristInc = elbowInc = waistInc = 1.0f;
    lastUpdate = micros();
  }

  // Attach the output pins to their servos
  // 
  void attach(void) {
    pinchServo.attach(pinchPin);
    wristServo.attach(wristPin);
    elbowServo.attach(elbowPin);
    waistServo.attach(waistPin);
  }

  // Detach the output pins from their servos 
  // 
  void detach(void) {
    pinchServo.detach();
    wristServo.detach();
    elbowServo.detach();
    waistServo.detach();
  }

  // Map another Arm object's position onto our position
  Arm & operator = (Arm &arm) {
    target.pinch = map(arm.pinch, arm.range.a.pinch, arm.range.b.pinch, range.a.pinch, range.b.pinch);
    target.wrist = map(arm.wrist, arm.range.a.wrist, arm.range.b.wrist, range.a.wrist, range.b.wrist);
    target.elbow = map(arm.elbow, arm.range.a.elbow, arm.range.b.elbow, range.a.elbow, range.b.elbow);
    target.waist = map(arm.waist, arm.range.a.waist, arm.range.b.waist, range.a.waist, range.b.waist);
    calcIncs();
    pinchInc = wristInc = elbowInc = waistInc = 1.0f;
    return *this;
  }

  // update our position to a specific position
  //
  OutputArm & operator = (Pos &pos) {
    target = pos;
    calcIncs();
    pinchInc = wristInc = elbowInc = waistInc = 1.0f;
    return *this;
  }

  // Set the update mode for the servos
  //
  void setMode(UpdateMode m) {
    mode = m;
    calcIncs();
  }

  // Pause for the specified number of milliseconds,
  // continually updating the output position if necessary
  //
  void delay(int ms) {
    uint32_t timer = millis() + ms;
    while (millis() < timer) {
      write();
    }
  }

  // Calculate the increment values for all 4 axis
  // from the current position to the target and
  // set the float Pos values to the current start
  // position. Used for timed movements
  void calcIncs(float ms = 0.0) {
    lastUpdate = millis();
    if (ms == 0.0) ms = 350.0;

    pinchPos = pinch;
    wristPos = wrist;
    elbowPos = elbow;
    waistPos = waist;

    pinchInc = (pinch < target.pinch) ? (float) (target.pinch - pinch) / (float) ms : (pinch > target.pinch) ? (float) (pinch - target.pinch) / (float) ms : 0.0;
    wristInc = (wrist < target.wrist) ? (float) (target.wrist - wrist) / (float) ms : (wrist > target.wrist) ? (float) (wrist - target.wrist) / (float) ms : 0.0;
    elbowInc = (elbow < target.elbow) ? (float) (target.elbow - elbow) / (float) ms : (elbow > target.elbow) ? (float) (elbow - target.elbow) / (float) ms : 0.0;
    waistInc = (waist < target.waist) ? (float) (target.waist - waist) / (float) ms : (waist > target.waist) ? (float) (waist - target.waist) / (float) ms : 0.0;
  }


  void write(Pos &pos, int ms = 0, bool wait = false) {
    target = pos;

    calcIncs();

    if (wait)
      delay(ms);
    else
      write();
  }


  // Update the servos towards the target position
  // using the current update mode
  void write(void) {
    switch (mode) {
      case Immediate:
        *(dynamic_cast<Pos*>(this)) = target;
        break;

      case Increment1:
        pinch = (pinch < target.pinch) ? pinch + 1 : (pinch > target.pinch) ? pinch - 1 : pinch;
        wrist = (wrist < target.wrist) ? wrist + 1 : (wrist > target.wrist) ? wrist - 1 : wrist;
        elbow = (elbow < target.elbow) ? elbow + 1 : (elbow > target.elbow) ? elbow - 1 : elbow;
        waist = (waist < target.waist) ? waist + 1 : (waist > target.waist) ? waist - 1 : waist;
        break;

      case IncrementHalf:
        pinch = (pinch < target.pinch) ? (pinch + ((target.pinch - pinch) / 2)) : (pinch > target.pinch) ? (pinch - ((pinch - target.pinch) / 2)) : pinch;
        wrist = (wrist < target.wrist) ? (wrist + ((target.wrist - wrist) / 2)) : (wrist > target.wrist) ? (wrist - ((wrist - target.wrist) / 2)) : wrist;
        elbow = (elbow < target.elbow) ? (elbow + ((target.elbow - elbow) / 2)) : (elbow > target.elbow) ? (elbow - ((elbow - target.elbow) / 2)) : elbow;
        waist = (waist < target.waist) ? (waist + ((target.waist - waist) / 2)) : (waist > target.waist) ? (waist - ((waist - target.waist) / 2)) : waist;
        break;

      case IncrementTime:
        {
          float elapsed = millis() - lastUpdate;
          float pinchAmt = elapsed * pinchInc;
          float wristAmt = elapsed * wristInc;
          float elbowAmt = elapsed * elbowInc;
          float waistAmt = elapsed * waistInc;
  
          pinch = (unsigned) ((pinch < target.pinch) ? (pinchPos + pinchAmt) : (pinch > target.pinch) ? (pinchPos - pinchAmt) : pinch);
          wrist = (unsigned) ((wrist < target.wrist) ? (wristPos + wristAmt) : (wrist > target.wrist) ? (wristPos - wristAmt) : wrist);
          elbow = (unsigned) ((elbow < target.elbow) ? (elbowPos + elbowAmt) : (elbow > target.elbow) ? (elbowPos - elbowAmt) : elbow);
          waist = (unsigned) ((waist < target.waist) ? (waistPos + waistAmt) : (waist > target.waist) ? (waistPos - waistAmt) : waist);

          pinch = clip(pinch, range.a.pinch, range.b.pinch);
          wrist = clip(wrist, range.a.wrist, range.b.wrist);
          elbow = clip(elbow, range.a.elbow, range.b.elbow);
          waist = clip(waist, range.a.waist, range.b.waist);
        }
        break;
    }

    if (last.pinch != pinch) {
      pinchServo.writeMicroseconds(last.pinch = pinch);
    }
    if (last.wrist != wrist) {
      wristServo.writeMicroseconds(last.wrist = wrist);
    }
    if (last.elbow != elbow) {
      elbowServo.writeMicroseconds(last.elbow = elbow);
    }
    if (last.waist != waist) {
      waistServo.writeMicroseconds(last.waist = waist);
    }
  }


  // "Park" the output arm so it lays
  // down across the top of the box
  // 
  void park() {
    LinkedList<Pos> parkMoves;
    //                    pinch wrist elbow waist
    parkMoves.addTail(Pos(1050, 2100, 1450, 1582));
    parkMoves.addTail(Pos(1050, 2100, 1450,  620));
    parkMoves.addTail(Pos(1050, 2100,  580,  620));
    parkMoves.addTail(Pos(1050, 2300,  450,  620));

    Node<Pos> *ptr;
    mode = Immediate;
    attach();
    for (ptr = parkMoves.head; ptr != nullptr; ptr = ptr->next) {
      *this = ptr->t;
      delay(1000);
    }
  }
  
};

#endif // #ifndef OUTPUT_ARM_H_INCL
