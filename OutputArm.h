#ifndef OUTPUT_ARM_H_INCL
#define OUTPUT_ARM_H_INCL

#include <Servo.h>
#include "mimic.h"

class OutputArm : public Arm {
private:

  OutputArm() :
    Arm(),
    iRange(iRange.a, iRange.b),
    oRange(oRange.a, oRange.b)
  {
  }

protected:

  uint8_t pinchPin, wristPin, elbowPin, waistPin;
  Servo pinchServo, wristServo, elbowServo, waistServo;
  Limits iRange;
  Limits oRange;
  Pos last;

public:

  OutputArm(int pinchPin, int wristPin, int elbowPin, int waistPin, Limits &inLimits, Limits &outLimits) :
    Arm(),
    pinchPin(pinchPin),
    wristPin(wristPin),
    elbowPin(elbowPin),
    waistPin(waistPin),
    last(pinch, wrist, elbow, waist) {

    iRange = inLimits;
    oRange = outLimits;

    pinMode(pinchPin, OUTPUT);
    pinMode(wristPin, OUTPUT);
    pinMode(elbowPin, OUTPUT);
    pinMode(waistPin, OUTPUT);
  }

  OutputArm & operator = (InputArm &arm) {
    pinch = map(arm.pinch, iRange.a.pinch, iRange.b.pinch, oRange.a.pinch, oRange.b.pinch);
    wrist = map(arm.wrist, iRange.a.wrist, iRange.b.wrist, oRange.a.wrist, oRange.b.wrist);
    elbow = map(arm.elbow, iRange.a.elbow, iRange.b.elbow, oRange.a.elbow, oRange.b.elbow);
    waist = map(arm.waist, iRange.a.waist, iRange.b.waist, oRange.a.waist, oRange.b.waist);

    return *this;
  }

  OutputArm & operator = (Pos &pos) {
    pinch = pos.pinch;
    wrist = pos.wrist;
    elbow = pos.elbow;
    waist = pos.waist;
    return *this;
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
  
  void writePinch(int value, int mS = 0) {
    UNUSED(mS);
    pinch = clip(value, oRange.a.pinch, oRange.b.pinch);
    if (pinch != last.pinch) {
      pinchServo.write(last.pinch = pinch);
    }
  }

  void writeWrist(const int &value, int mS = 0) {
    UNUSED(mS);
    wrist = clip(value, oRange.a.wrist, oRange.b.wrist);
    if (last.wrist != wrist) {
      wristServo.write(last.wrist = wrist);
    }
  }

  void writeElbow(const int &value, int mS = 0) {
    UNUSED(mS);
    elbow = clip(value, oRange.a.elbow, oRange.b.elbow);
    if (last.elbow != elbow) {
      elbowServo.write(last.elbow = elbow);
    }
  }

  void writeWaist(const int &value, int mS = 0) {
    UNUSED(mS);
    waist = clip(value, oRange.a.waist, oRange.b.waist);
    if (last.waist != waist) {
      waistServo.write(last.waist = waist);
    }
  }

  void write(int mS = 0) {
    writePinch(pinch, mS);
    writeWrist(wrist, mS);
    writeElbow(elbow, mS);
    writeWaist(waist, mS);
  }

  // "Park" the output arm so it lays
  // down across the top of the box
  // 
  void park() {
    static OutputArm *me = this;
    me = this;
    LinkedList<Pos> parkMoves;
    parkMoves.addTail(Pos(20, 150, 95, 101));
    parkMoves.addTail(Pos(20, 150, 95, 7));
    parkMoves.addTail(Pos(20, 150, 17, 7));
    parkMoves.addTail(Pos(20, 165, 1, 7));

    parkMoves.foreach([](Pos &pos) -> int {
      *me = pos;
      (*me).write();
      delay(1500);
      return 0;
    });
  }
  
};

#endif // #ifndef OUTPUT_ARM_H_INCL
