// motors.h — Motor controller and quadrature encoder declarations.

#pragma once
#include <Arduino.h>
#include "config.h"

// ── Encoder ───────────────────────────────────────────────────────────────────
// ISR-driven quadrature encoder. Channel A triggers interrupt; Channel B
// is read inside the ISR to determine rotation direction.
// Ticks are volatile because they are written in ISRs and read in main loop.

extern volatile long leftTicks;   // signed: + = forward, - = reverse
extern volatile long rightTicks;

void encodersInit();
void resetEncoders();
long getLeftTicks();
long getRightTicks();
float headingError();  // left - right ticks (+ = drifting right)

// ── Motor controller ──────────────────────────────────────────────────────────
void motorsInit();
void motorForward(uint8_t speed = SPEED_FULL);
void motorReverse(uint8_t speed = SPEED_REVERSE);
void motorSpinLeft(uint8_t speed = SPEED_TURN);
void motorSpinRight(uint8_t speed = SPEED_TURN);
void motorArcLeft(uint8_t outer = SPEED_FULL, uint8_t inner = SPEED_STALK);
void motorArcRight(uint8_t outer = SPEED_FULL, uint8_t inner = SPEED_STALK);
void motorStop();
