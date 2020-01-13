#ifndef INPUTARM_H_INCL
#define INPUTARM_H_INCL

#include "mimic.h"

#define DEFAULT_SAMPLES   1

class InputArm : public Arm {
private:

  // disable the default constructor by making it private
  InputArm() {
  }

protected:

  uint8_t pinchPin, wristPin, elbowPin, waistPin;
  Limits limits;
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

  InputArm(int pinch_pin, int wrist_pin, int elbow_pin, int waist_pin, Limits &lim) :
    pinchPin(pinch_pin),
    wristPin(wrist_pin),
    elbowPin(elbow_pin),
    waistPin(waist_pin),
    limits(lim),
    samples(DEFAULT_SAMPLES) {
    pinMode(pinchPin, INPUT);
    pinMode(wristPin, INPUT);
    pinMode(elbowPin, INPUT);
    pinMode(waistPin, INPUT);
  }

  uint16_t readPinch() {
    return pinch = clip(analogReadAvg(pinchPin), limits.a.pinch, limits.b.pinch);
  }

  uint16_t readWrist() {
    return wrist = clip(analogReadAvg(wristPin), limits.a.wrist, limits.b.wrist);
  }

  uint16_t readElbow() {
    return elbow = clip(analogReadAvg(elbowPin), limits.a.elbow, limits.b.elbow);
  }

  uint16_t readWaist() {
    return waist = clip(analogReadAvg(waistPin), limits.a.waist, limits.b.waist);
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
