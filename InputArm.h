#ifndef INPUTARM_H_INCL
#define INPUTARM_H_INCL

#include "mimic.h"

#define DEFAULT_SAMPLES   1

class InputArm : public Arm {
protected:

  int samples;

  uint16_t analogReadAvg(int pin, int num = 0) {
    long total = 0;
    if (num == 0)
      num = samples;
    for (int i=0; i < num; i++)
      total += analogRead(pin);
    return total / num;
  }

public:

  InputArm() = delete;

  InputArm(int pinch_pin, int wrist_pin, int elbow_pin, int waist_pin, Limits &limits) :
    Arm(pinch_pin, wrist_pin, elbow_pin, waist_pin, limits),
    samples(DEFAULT_SAMPLES) {
    pinMode(pinchPin, INPUT);
    pinMode(wristPin, INPUT);
    pinMode(elbowPin, INPUT);
    pinMode(waistPin, INPUT);
  }

  uint16_t readPinch() {
    return pinch = clip(analogReadAvg(pinchPin), range.a.pinch, range.b.pinch);
  }

  uint16_t readWrist() {
    return wrist = clip(analogReadAvg(wristPin), range.a.wrist, range.b.wrist);
  }

  uint16_t readElbow() {
    return elbow = clip(analogReadAvg(elbowPin), range.a.elbow, range.b.elbow);
  }

  uint16_t readWaist() {
    return waist = clip(analogReadAvg(waistPin), range.a.waist, range.b.waist);
  }

  InputArm &read() {
    readPinch();
    readWrist();
    readElbow();
    readWaist();

    return *this;
  }
};

#endif // #ifndef INPUTARM_H_INCL
