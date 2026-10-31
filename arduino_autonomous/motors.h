// =============================================================================
// motors.h — L298N dual H-bridge motor control declarations.
//
// All motors run at full power — ENA and ENB jumpers on the L298N board
// must be left ON. No PWM speed control is used.
//
// Movement functions:
//   motorForward()   — both wheels forward
//   motorReverse()   — both wheels backward
//   motorSpinLeft()  — left wheel back, right wheel forward  (pivot left)
//   motorSpinRight() — left wheel forward, right wheel back  (pivot right)
//   motorArcLeft()   — right wheel drives, left wheel stops  (curve left)
//   motorArcRight()  — left wheel drives, right wheel stops  (curve right)
//   motorStop()      — all outputs LOW (coast/brake)
// =============================================================================

#pragma once
#include <Arduino.h>
#include "config.h"

void motorsInit();
void motorForward();
void motorReverse();
void motorSpinLeft();
void motorSpinRight();
void motorArcLeft();
void motorArcRight();
void motorStop();
