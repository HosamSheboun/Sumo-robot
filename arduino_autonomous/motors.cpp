// =============================================================================
// motors.cpp — L298N dual H-bridge motor control implementation.
//
// GA25-370 motor wire connections:
//   Red       → L298N OUT1 (left) / OUT3 (right)   motor power +
//   Black     → L298N OUT2 (left) / OUT4 (right)   motor power −
//   Green     → Uno 5V                              encoder VCC (must be powered)
//   Dark Blue → GND                                 encoder GND (must be grounded)
//   Yellow    → leave unconnected                   encoder channel A (unused)
//   White     → leave unconnected                   encoder channel B (unused)
//
// L298N truth table for one motor channel:
//   IN1=HIGH, IN2=LOW  → forward
//   IN1=LOW,  IN2=HIGH → reverse
//   IN1=LOW,  IN2=LOW  → coast (stop)
//   IN1=HIGH, IN2=HIGH → brake (avoid — can stress the driver)
// =============================================================================

#include "motors.h"

// ── Private helpers ───────────────────────────────────────────────────────────

static void setLeft(bool forward) {
    digitalWrite(LEFT_IN1, forward ? HIGH : LOW);
    digitalWrite(LEFT_IN2, forward ? LOW  : HIGH);
}

static void setRight(bool forward) {
    digitalWrite(RIGHT_IN3, forward ? HIGH : LOW);
    digitalWrite(RIGHT_IN4, forward ? LOW  : HIGH);
}

static void stopLeft() {
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, LOW);
}

static void stopRight() {
    digitalWrite(RIGHT_IN3, LOW);
    digitalWrite(RIGHT_IN4, LOW);
}

// ── Public functions ──────────────────────────────────────────────────────────

void motorsInit() {
    // Set pin modes first, THEN call motorStop — pins must be OUTPUT before writing
    pinMode(LEFT_IN1,  OUTPUT);
    pinMode(LEFT_IN2,  OUTPUT);
    pinMode(RIGHT_IN3, OUTPUT);
    pinMode(RIGHT_IN4, OUTPUT);
    motorStop();
}

void motorForward() {
    setLeft(true);
    setRight(true);
}

void motorReverse() {
    setLeft(false);
    setRight(false);
}

void motorSpinLeft() {
    // Pivot left: left wheel reverses, right wheel drives forward
    setLeft(false);
    setRight(true);
}

void motorSpinRight() {
    // Pivot right: left wheel drives forward, right wheel reverses
    setLeft(true);
    setRight(false);
}

void motorArcLeft() {
    // Curve left: right wheel drives, left wheel coasts
    stopLeft();
    setRight(true);
}

void motorArcRight() {
    // Curve right: left wheel drives, right wheel coasts
    setLeft(true);
    stopRight();
}

void motorStop() {
    stopLeft();
    stopRight();
}
