#ifndef OUTPUT_ARM_H_INCL
#define OUTPUT_ARM_H_INCL

#include <Servo.h>
#include "mimic.h"


class OutputArm : public Arm {
private:

  OutputArm() = delete;

  void writePinch(int value) {
    pinch = clip(value, oRange.a.pinch, oRange.b.pinch);
    if (last.pinch != pinch) {
      pinchServo.writeMicroseconds(last.pinch = pinch);
    }
  }

  void writeWrist(int value) {
    wrist = clip(value, oRange.a.wrist, oRange.b.wrist);
    if (last.wrist != wrist) {
      wristServo.writeMicroseconds(last.wrist = wrist);
    }
  }

  void writeElbow(int value) {
    elbow = clip(value, oRange.a.elbow, oRange.b.elbow);
    if (last.elbow != elbow) {
      elbowServo.writeMicroseconds(last.elbow = elbow);
    }
  }

  void writeWaist(int value) {
    waist = clip(value, oRange.a.waist, oRange.b.waist);
    if (last.waist != waist) {
      waistServo.writeMicroseconds(last.waist = waist);
    }
  }

public:

  uint8_t pinchPin, wristPin, elbowPin, waistPin;
  Servo pinchServo, wristServo, elbowServo, waistServo;
  Limits iRange;
  Limits oRange;
  Pos last, target;

public:

  OutputArm(int pinchPin, int wristPin, int elbowPin, int waistPin, Limits &inLimits, Limits &outLimits) :
    Arm(),
    pinchPin(pinchPin),
    wristPin(wristPin),
    elbowPin(elbowPin),
    waistPin(waistPin) {

    iRange = inLimits;
    oRange = outLimits;

    pinMode(pinchPin, OUTPUT);
    pinMode(wristPin, OUTPUT);
    pinMode(elbowPin, OUTPUT);
    pinMode(waistPin, OUTPUT);
  }

  // Attach the output pins to their servos
  // 
  void attachServos() {
    pinchServo.attach(pinchPin);
    wristServo.attach(wristPin);
    elbowServo.attach(elbowPin);
    waistServo.attach(waistPin);
  }

  // Detach the output pins from their servos 
  // 
  void detachServos() {
    pinchServo.detach();
    wristServo.detach();
    elbowServo.detach();
    waistServo.detach();
  }

  // convert the position of an input arm to our local ranges
  // 
  OutputArm & operator = (InputArm &arm) {
    pinch = map(arm.pinch, iRange.a.pinch, iRange.b.pinch, oRange.a.pinch, oRange.b.pinch);
    wrist = map(arm.wrist, iRange.a.wrist, iRange.b.wrist, oRange.a.wrist, oRange.b.wrist);
    elbow = map(arm.elbow, iRange.a.elbow, iRange.b.elbow, oRange.a.elbow, oRange.b.elbow);
    waist = map(arm.waist, iRange.a.waist, iRange.b.waist, oRange.a.waist, oRange.b.waist);
    target = *(dynamic_cast<Pos*>(this));

    return *this;
  }

  // update our position to a specific position
  // 
  OutputArm & operator = (Pos &pos) {
    target = *(dynamic_cast<Pos*>(this)) = pos;

    return *this;
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

    attachServos();
    parkMoves.foreach([](Pos &pos) -> int {
      (*me) = pos;
      (*me).write();
      delay(1000);
      return 0;
    });
  }
  
};

#endif // #ifndef OUTPUT_ARM_H_INCL
