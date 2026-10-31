// =============================================================================
// motors.h — L298N motor control for ESP32 using LEDC hardware PWM.
// Speed control is available on ESP32 unlike the Uno/Nano jumper builds.
// =============================================================================
#pragma once
#include <Arduino.h>
#include "config.h"

void motorsInit();
void motorForward(uint8_t speed = SPEED_FULL);
void motorReverse(uint8_t speed = SPEED_FULL);
void motorSpinLeft(uint8_t speed = SPEED_SEARCH);
void motorSpinRight(uint8_t speed = SPEED_SEARCH);
void motorArcLeft(uint8_t outer = SPEED_ARC_OUT, uint8_t inner = SPEED_ARC_IN);
void motorArcRight(uint8_t outer = SPEED_ARC_OUT, uint8_t inner = SPEED_ARC_IN);
void motorStop();
