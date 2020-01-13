#ifndef OUTPUT_ARM_H_INCL
#define OUTPUT_ARM_H_INCL

#include <Servo.h>
#include "mimic.h"

class OutputArm : public Arm {
private:

  OutputArm() :
    iRange(iRange.a, iRange.b),
    oRange(oRange.a, oRange.b)
  {
  }

protected:

  uint8_t pinchPin, wristPin, elbowPin, waistPin;
  Servo pinchServo, wristServo, elbowServo, waistServo;
  Limits iRange;
  Limits oRange;

public:

  OutputArm(int pinchPin, int wristPin, int elbowPin, int waistPin, Limits &inLimits, Limits &outLimits) :
    pinchPin(pinchPin),
    wristPin(wristPin),
    elbowPin(elbowPin),
    waistPin(waistPin),
    iRange(inLimits.a, inLimits.b),
    oRange(outLimits.a, outLimits.b) {
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

  OutputArm & operator = (Arm &arm) {
    pinch = arm.pinch;
    wrist = arm.wrist;
    elbow = arm.elbow;
    waist = arm.waist;
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
  
  void writePinch(int value, int mS = 1) {
    UNUSED(mS);
    pinchServo.write(pinch = clip(value, oRange.a.pinch, oRange.b.pinch));
  }

  void writeWrist(const int &value, int mS = 1) {
    UNUSED(mS);
    wristServo.write(wrist = clip(value, oRange.a.wrist, oRange.b.wrist));
  }

  void writeElbow(const int &value, int mS = 1) {
    UNUSED(mS);
    elbowServo.write(elbow = clip(value, oRange.a.elbow, oRange.b.elbow));
  }

  void writeWaist(const int &value, int mS = 1) {
    UNUSED(mS);
    waistServo.write(waist = clip(value, oRange.a.waist, oRange.b.waist));
  }

  void write() {
    writePinch(pinch);
    writeWrist(wrist);
    writeElbow(elbow);
    writeWaist(waist);
  }

  // "Park" the output arm so it lays
  // down across the top of the box
  // 
  void park() {
    Arm posUpFwd(20, 150, 95, 101);
//  Arm posDownFwd(20, 150, 17, 101);
    Arm posUpLeft(20, 150, 95, 7);
    Arm posDownLeft(20, 150, 17, 7);
    Arm posParkLeft(20, 165, 1, 7);
    
    *this = posUpFwd;
    write();
    delay(1500);

    *this = posUpLeft;
    write();
    delay(1500);

    *this = posDownLeft;
    write();
    delay(1500);

    *this = posParkLeft;
    write();
    delay(1500);
  }
  
};

#endif // #ifndef OUTPUT_ARM_H_INCL
