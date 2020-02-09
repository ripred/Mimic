#ifndef OUTPUT_ARM_H_INCL
#define OUTPUT_ARM_H_INCL

#include <Servo.h>
#include "mimic.h"


enum UpdateMode { Immediate, Increment1, IncrementHalf, IncrementTime };

class OutputArm : public Arm {
private:
  UpdateMode mode;

public:

  Servo pinchServo, wristServo, elbowServo, waistServo;
  Pos last, target;
  double pinchInc, wristInc, elbowInc, waistInc;
  double pinchPos, wristPos, elbowPos, waistPos;
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
//  waistPos = target.waist = waist = ((range.b.waist - range.a.waist) / 2) + range.a.waist;
    waistPos = target.waist = waist = 1582;


    pinchInc = wristInc = elbowInc = waistInc = 1;
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

  Arm & operator = (Arm &arm) {
    target.pinch = map(arm.pinch, arm.range.a.pinch, arm.range.b.pinch, range.a.pinch, range.b.pinch);
    target.wrist = map(arm.wrist, arm.range.a.wrist, arm.range.b.wrist, range.a.wrist, range.b.wrist);
    target.elbow = map(arm.elbow, arm.range.a.elbow, arm.range.b.elbow, range.a.elbow, range.b.elbow);
    target.waist = map(arm.waist, arm.range.a.waist, arm.range.b.waist, range.a.waist, range.b.waist);

    pinchInc = wristInc = elbowInc = waistInc = 1.0;

    return *this;
  }

  // update our position to a specific position
  //
  OutputArm & operator = (Pos &pos) {
    target = pos;

    pinchInc = wristInc = elbowInc = waistInc = 1.0;

    return *this;
  }

  // Set the update mode for the servos
  //
  void setMode(UpdateMode m) {
    mode = m;
    if (mode == IncrementTime) {
      pinchPos = pinch;
      wristPos = wrist;
      elbowPos = elbow;
      waistPos = waist;
      pinchInc = wristInc = elbowInc = waistInc = 1;
    }
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
          double s = (double) (micros() - lastUpdate) / 1000.0f;
          pinch = (int) (pinchPos += (pinch < target.pinch ? s*pinchInc : pinch > target.pinch ? -(s*pinchInc) : 0.0f));
          wrist = (int) (wristPos += (wrist < target.wrist ? s*wristInc : wrist > target.wrist ? -(s*wristInc) : 0.0f));
          elbow = (int) (elbowPos += (elbow < target.elbow ? s*elbowInc : elbow > target.elbow ? -(s*elbowInc) : 0.0f));
          waist = (int) (waistPos += (waist < target.waist ? s*waistInc : waist > target.waist ? -(s*waistInc) : 0.0f));

          pinch = clip(pinch, range.a.pinch, range.b.pinch);
          wrist = clip(wrist, range.a.wrist, range.b.wrist);
          elbow = clip(elbow, range.a.elbow, range.b.elbow);
          waist = clip(waist, range.a.waist, range.b.waist);
        }
        break;
    }

    if ((mode != IncrementTime) || ((micros() - lastUpdate) >= 1000)) {
      lastUpdate = micros();
  
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
  }

  void write(Pos &pos, int ms = 0, bool wait = false) {
    target = pos;
    lastUpdate = micros();

    if (ms == 0)
      ms = 1;
      
    if (pinch < target.pinch)
      pinchInc = (double) (target.pinch - pinch) / (double) ms;
    else if (pinch > target.pinch)
      pinchInc = (double) (pinch - target.pinch) / (double) ms;
    else
      pinchInc = 0.0;

    if (wrist < target.wrist)
      wristInc = (double) (target.wrist - wrist) / (double) ms;
    else if (wrist > target.wrist)
      wristInc = (double) (wrist - target.wrist) / (double) ms;
    else
      wristInc = 0.0;

    if (elbow < target.elbow)
      elbowInc = (double) (target.elbow - elbow) / (double) ms;
    else if (elbow > target.elbow)
      elbowInc = (double) (elbow - target.elbow) / (double) ms;
    else
      elbowInc = 0.0;

    if (waist < target.waist)
      waistInc = (double) (target.waist - waist) / (double) ms;
    else if (waist > target.waist)
      waistInc = (double) (waist - target.waist) / (double) ms;
    else
      waistInc = 0.0;

    if (wait)
      delay(ms);
    else
      write();
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
    attach();
    for (ptr = parkMoves.head; ptr != nullptr; ptr = ptr->next) {
      *this = ptr->t;
      delay(1000);
    }
  }
  
};

#endif // #ifndef OUTPUT_ARM_H_INCL
