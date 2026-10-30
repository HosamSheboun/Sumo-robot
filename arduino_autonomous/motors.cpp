// motors.cpp — L298N motor control and GA25-370 quadrature encoder implementation.
//
// Encoder wiring (GA25-370):
//   Red       → L298N OUTx  (motor power +)
//   Black     → L298N OUTx  (motor power −)
//   Green     → Nano 5V     (encoder VCC)
//   Dark Blue → GND         (encoder GND)
//   Yellow    → Channel A   (interrupt pin D2 / D3)
//   White     → Channel B   (digital read pin A0 / A1)
//
// Direction detection:
//   When Channel A rises, read Channel B.
//   B = HIGH → forward  (+1 tick)
//   B = LOW  → reverse  (−1 tick)

#include "motors.h"

// ── Encoder tick counters (written by ISR, read by main loop) ─────────────────
volatile long leftTicks  = 0;
volatile long rightTicks = 0;

// ── ISR callbacks ─────────────────────────────────────────────────────────────
static void leftEncoderISR() {
    if (digitalRead(LEFT_ENC_B) == HIGH) leftTicks++;
    else                                  leftTicks--;
}

static void rightEncoderISR() {
    if (digitalRead(RIGHT_ENC_B) == HIGH) rightTicks++;
    else                                   rightTicks--;
}

// ── Encoder public functions ──────────────────────────────────────────────────
void encodersInit() {
    pinMode(LEFT_ENC_A,  INPUT_PULLUP);
    pinMode(LEFT_ENC_B,  INPUT_PULLUP);
    pinMode(RIGHT_ENC_A, INPUT_PULLUP);
    pinMode(RIGHT_ENC_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(LEFT_ENC_A),  leftEncoderISR,  RISING);
    attachInterrupt(digitalPinToInterrupt(RIGHT_ENC_A), rightEncoderISR, RISING);
}

void resetEncoders() {
    noInterrupts();
    leftTicks  = 0;
    rightTicks = 0;
    interrupts();
}

long getLeftTicks() {
    noInterrupts();
    long v = leftTicks;
    interrupts();
    return v;
}

long getRightTicks() {
    noInterrupts();
    long v = rightTicks;
    interrupts();
    return v;
}

float headingError() {
    return (float)(getLeftTicks() - getRightTicks());
}

// ── Motor init ────────────────────────────────────────────────────────────────
void motorsInit() {
    pinMode(LEFT_IN1,  OUTPUT);
    pinMode(LEFT_IN2,  OUTPUT);
    pinMode(LEFT_ENA,  OUTPUT);
    pinMode(RIGHT_IN3, OUTPUT);
    pinMode(RIGHT_IN4, OUTPUT);
    pinMode(RIGHT_ENB, OUTPUT);
    motorStop();
}

// ── Low-level helpers ─────────────────────────────────────────────────────────
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

// ── Movement commands ─────────────────────────────────────────────────────────
void motorForward(uint8_t speed)  { setLeft(speed, true);  setRight(speed, true);  }
void motorReverse(uint8_t speed)  { setLeft(speed, false); setRight(speed, false); }
void motorSpinLeft(uint8_t speed) { setLeft(speed, false); setRight(speed, true);  }
void motorSpinRight(uint8_t speed){ setLeft(speed, true);  setRight(speed, false); }

void motorArcLeft(uint8_t outer, uint8_t inner) {
    // Curve left: right motor faster than left
    setLeft(inner, true);
    setRight(outer, true);
}

void motorArcRight(uint8_t outer, uint8_t inner) {
    // Curve right: left motor faster than right
    setLeft(outer, true);
    setRight(inner, true);
}

void motorStop() {
    setLeft(0, true);
    setRight(0, true);
}
