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
  float pinchInc, wristInc, elbowInc, waistInc;
  float pinchPos, wristPos, elbowPos, waistPos;

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
    waistPos = target.waist = waist = ((range.b.waist - range.a.waist) / 2) + range.a.waist;

    pinchInc = wristInc = elbowInc = waistInc = 1;
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

    return *this;
  }

  // update our position to a specific position
  //
  OutputArm & operator = (Pos &pos) {
    target = pos;

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
    unsigned long int timer = millis() + ms;
    while (millis() < timer)
      write();
  }

  // Update the servos towards the target position
  // using the current update mode
  void write(void) {
    switch (mode) {
      case Immediate:
        pinch = target.pinch;
        wrist = target.wrist;
        elbow = target.elbow;
        waist = target.waist;
        break;

      case Increment1:
        if (pinch < target.pinch)
          pinch++;
        else if (pinch > target.pinch)
          pinch--;
        if (wrist < target.wrist)
          wrist++;
        else if (wrist > target.wrist)
          wrist--;
        if (elbow < target.elbow)
          elbow++;
        else if (elbow > target.elbow)
          elbow--;
        if (waist < target.waist)
          waist++;
        else if (waist > target.waist)
          waist--;
        break;

      case IncrementHalf:
        if (pinch < target.pinch)
          pinch += (target.pinch - pinch) / 2;
        else if (pinch > target.pinch)
          pinch -= (pinch - target.pinch) / 2;
        if (wrist < target.wrist)
          wrist += (target.wrist - wrist) / 2;
        else if (wrist > target.wrist)
          wrist -= (wrist - target.wrist) / 2;
        if (elbow < target.elbow)
          elbow += (target.elbow - elbow) / 2;
        else if (elbow > target.elbow)
          elbow -= (elbow - target.elbow) / 2;
        if (waist < target.waist)
          waist += (target.waist - waist) / 2;
        else if (waist > target.waist)
          waist -= (waist - target.waist) / 2;
        break;

      case IncrementTime:
        if (pinch != target.pinch)
          pinch = (int) (pinchPos += (pinch < target.pinch ? pinchInc : 0.0 - pinchInc));
        if (wrist != target.wrist)
          wrist = (int) (wristPos += (wrist < target.wrist ? wristInc : 0.0 - wristInc));
        if (elbow != target.elbow)
          elbow = (int) (elbowPos += (elbow < target.elbow ? elbowInc : 0.0 - elbowInc));
        if (waist != target.waist)
          waist = (int) (waistPos += (waist < target.waist ? waistInc : 0.0 - waistInc));
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

  void write(Pos &pos, int ms = 0) {
    target = pos;
    if (mode == IncrementTime) {
      if (ms == 0)
        ms = 1;
      pinchInc = ((float) pinch - (float) pos.pinch) / (float) ms;
      wristInc = ((float) wrist - (float) pos.wrist) / (float) ms;
      elbowInc = ((float) elbow - (float) pos.elbow) / (float) ms;
      waistInc = ((float) waist - (float) pos.waist) / (float) ms;
      if (pinchInc < 0)
        pinchInc *= -1.0;
      if (wristInc < 0)
        wristInc *= -1.0;
      if (elbowInc < 0)
        elbowInc *= -1.0;
      if (waistInc < 0)
        waistInc *= -1.0;
    }
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

    Node <Pos> *ptr = parkMoves.head;
    attach();
    while (ptr != nullptr) {
      *this = ptr->t;
      write();
      ptr = ptr->next;
      delay(1000);
    }
  }
  
};

#endif // #ifndef OUTPUT_ARM_H_INCL
