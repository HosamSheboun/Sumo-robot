// motors.h — L298N motor control for Arduino Nano.
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
