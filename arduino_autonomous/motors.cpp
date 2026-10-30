// motors.cpp — L298N dual H-bridge motor control.
//
// GA25-370 motor wire connections:
//   Red       → L298N OUT1 (left) / OUT3 (right)   motor power +
//   Black     → L298N OUT2 (left) / OUT4 (right)   motor power −
//   Green     → Nano 5V                             encoder VCC (must be powered)
//   Dark Blue → GND                                 encoder GND (must be grounded)
//   Yellow    → leave unconnected                   encoder channel A (unused)
//   White     → leave unconnected                   encoder channel B (unused)

#include "motors.h"

static void setLeft(uint8_t speed, bool forward) {
    digitalWrite(LEFT_IN1, forward ? HIGH : LOW);
    digitalWrite(LEFT_IN2, forward ? LOW  : HIGH);
    analogWrite(LEFT_ENA, speed);
}

static void setRight(uint8_t speed, bool forward) {
    digitalWrite(RIGHT_IN3, forward ? HIGH : LOW);
    digitalWrite(RIGHT_IN4, forward ? LOW  : HIGH);
    analogWrite(RIGHT_ENB, speed);
}

void motorsInit() {
    pinMode(LEFT_IN1,  OUTPUT);
    pinMode(LEFT_IN2,  OUTPUT);
    pinMode(LEFT_ENA,  OUTPUT);
    pinMode(RIGHT_IN3, OUTPUT);
    pinMode(RIGHT_IN4, OUTPUT);
    pinMode(RIGHT_ENB, OUTPUT);
    motorStop();
}

void motorForward(uint8_t speed) {
    setLeft(speed,  true);
    setRight(speed, true);
}

void motorStop() {
    setLeft(0, true);
    setRight(0, true);
}
