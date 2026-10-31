// =============================================================================
// motors.h — L298N motor control declarations for Arduino Uno.
// ENA and ENB jumpers ON — full power, no PWM.
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
