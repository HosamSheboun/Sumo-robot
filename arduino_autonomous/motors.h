// motors.h — L298N motor controller declarations.
//
// Only forward and stop are used in this build.
// All other movement functions are removed.

#pragma once
#include <Arduino.h>
#include "config.h"

void motorsInit();
void motorForward(uint8_t speed = SPEED_FULL);
void motorSpinLeft(uint8_t speed = SPEED_SPIN);
void motorSpinRight(uint8_t speed = SPEED_SPIN);
void motorStop();
