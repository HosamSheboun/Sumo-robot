// =============================================================================
// motors.cpp — L298N motor control for Arduino Uno.
// =============================================================================
#include "motors.h"

static void setLeft(bool fwd) {
    digitalWrite(LEFT_IN1, fwd ? HIGH : LOW);
    digitalWrite(LEFT_IN2, fwd ? LOW  : HIGH);
}
static void setRight(bool fwd) {
    digitalWrite(RIGHT_IN3, fwd ? HIGH : LOW);
    digitalWrite(RIGHT_IN4, fwd ? LOW  : HIGH);
}

// Stop motor by setting both IN pins LOW.
// NOTE: L298N behavior for IN_A=LOW, IN_B=LOW depends on driver variant:
//   - Some drivers BRAKE (short motor terminals, fast stop, tight arc radius)
//   - Some drivers COAST (open motor terminals, free-wheel, wide arc radius)
// Clone L298N modules typically coast. Genuine STMicro modules typically brake.
// Test on actual hardware to verify arc radius matches tactical requirements.
static void stopLeft()  { digitalWrite(LEFT_IN1, LOW);  digitalWrite(LEFT_IN2, LOW);  }
static void stopRight() { digitalWrite(RIGHT_IN3, LOW); digitalWrite(RIGHT_IN4, LOW); }

void motorsInit() {
    pinMode(LEFT_IN1,  OUTPUT); pinMode(LEFT_IN2,  OUTPUT);
    pinMode(RIGHT_IN3, OUTPUT); pinMode(RIGHT_IN4, OUTPUT);
    motorStop();
}
void motorForward()   { setLeft(true);  setRight(true);  }
void motorReverse()   { setLeft(false); setRight(false); }
void motorSpinLeft()  { setLeft(false); setRight(true);  }
void motorSpinRight() { setLeft(true);  setRight(false); }
void motorArcLeft()   { stopLeft();     setRight(true);  }
void motorArcRight()  { setLeft(true);  stopRight();     }
void motorStop()      { stopLeft();     stopRight();     }
