#ifndef OUTPUT_ARM_H_INCL
#define OUTPUT_ARM_H_INCL

#include <Servo.h>
#include "mimic.h"


class OutputArm : public Arm {
public:

  Servo pinchServo, wristServo, elbowServo, waistServo;
  Pos last, target;

  OutputArm() = delete;

  OutputArm(int pinch_pin, int wrist_pin, int elbow_pin, int waist_pin, Limits &limits) :
    Arm(pinch_pin, wrist_pin, elbow_pin, waist_pin, limits) {
    pinMode(pinchPin, OUTPUT);
    pinMode(wristPin, OUTPUT);
    pinMode(elbowPin, OUTPUT);
    pinMode(waistPin, OUTPUT);
  }

  // Attach the output pins to their servos
  // 
  void attach() {
    pinchServo.attach(pinchPin);
    wristServo.attach(wristPin);
    elbowServo.attach(elbowPin);
    waistServo.attach(waistPin);
  }

  // Detach the output pins from their servos 
  // 
  void detach() {
    pinchServo.detach();
    wristServo.detach();
    elbowServo.detach();
    waistServo.detach();
  }

  Arm & operator = (Arm &arm) {
    pinch = map(arm.pinch, arm.range.a.pinch, arm.range.b.pinch, range.a.pinch, range.b.pinch);
    wrist = map(arm.wrist, arm.range.a.wrist, arm.range.b.wrist, range.a.wrist, range.b.wrist);
    elbow = map(arm.elbow, arm.range.a.elbow, arm.range.b.elbow, range.a.elbow, range.b.elbow);
    waist = map(arm.waist, arm.range.a.waist, arm.range.b.waist, range.a.waist, range.b.waist);
    target = *(dynamic_cast<Pos*>(this));

    return *this;
  }

  // update our position to a specific position
  // 
  OutputArm & operator = (Pos &pos) {
    target = *(dynamic_cast<Pos*>(this)) = pos;

    return *this;
  }

  void writePinch(int value) {
    pinch = clip(value, range.a.pinch, range.b.pinch);
    if (last.pinch != pinch) {
      pinchServo.writeMicroseconds(last.pinch = pinch);
    }
  }

  void writeWrist(int value) {
    wrist = clip(value, range.a.wrist, range.b.wrist);
    if (last.wrist != wrist) {
      wristServo.writeMicroseconds(last.wrist = wrist);
    }
  }

  void writeElbow(int value) {
    elbow = clip(value, range.a.elbow, range.b.elbow);
    if (last.elbow != elbow) {
      elbowServo.writeMicroseconds(last.elbow = elbow);
    }
  }

  void writeWaist(int value) {
    waist = clip(value, range.a.waist, range.b.waist);
    if (last.waist != waist) {
      waistServo.writeMicroseconds(last.waist = waist);
    }
  }

  void write() {
    writePinch(pinch);
    writeWrist(wrist);
    writeElbow(elbow);
    writeWaist(waist);
  }

  void write(Pos &pos) {
    operator = (pos);
    write();
  }

  void increment() {
    if (pinch < target.pinch) pinch++;
    else if (pinch > target.pinch) pinch--;

    if (wrist < target.wrist) wrist++;
    else if (wrist > target.wrist) wrist--;

    if (elbow < target.elbow) elbow++;
    else if (elbow > target.elbow) elbow--;

    if (waist < target.waist) waist++;
    else if (waist > target.waist) waist--;

    write();
  }

  void increment(Pos &pos) {
    target = pos;
    increment();
  }

  // "Park" the output arm so it lays
  // down across the top of the box
  // 
  void park() {
    static OutputArm *me = this;
    me = this;
    LinkedList<Pos> parkMoves;
    //                    pinch wrist elbow waist
    parkMoves.addTail(Pos(1050, 2100, 1450, 1582));
    parkMoves.addTail(Pos(1050, 2100, 1450,  620));
    parkMoves.addTail(Pos(1050, 2100,  580,  620));
    parkMoves.addTail(Pos(1050, 2300,  450,  620));

    attach();
    parkMoves.foreach([](Pos &pos) -> int {
      *me = pos;
      (*me).write();
      delay(1000);
      return 0;
    });
  }
  
};

#endif // #ifndef OUTPUT_ARM_H_INCL
