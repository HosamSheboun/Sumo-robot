// =============================================================================
// motors.cpp — L298N motor control for ESP32 using LEDC hardware PWM.
// =============================================================================
#include "motors.h"

static void setLeft(uint8_t speed, bool fwd) {
    digitalWrite(LEFT_IN1, fwd ? HIGH : LOW);
    digitalWrite(LEFT_IN2, fwd ? LOW  : HIGH);
    ledcWrite(PWM_CH_LEFT, speed);
}
static void setRight(uint8_t speed, bool fwd) {
    digitalWrite(RIGHT_IN3, fwd ? HIGH : LOW);
    digitalWrite(RIGHT_IN4, fwd ? LOW  : HIGH);
    ledcWrite(PWM_CH_RIGHT, speed);
}

void motorsInit() {
    pinMode(LEFT_IN1,  OUTPUT); pinMode(LEFT_IN2,  OUTPUT);
    pinMode(RIGHT_IN3, OUTPUT); pinMode(RIGHT_IN4, OUTPUT);

    // Attach LEDC PWM channels
    ledcSetup(PWM_CH_LEFT,  PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CH_RIGHT, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LEFT_ENA,  PWM_CH_LEFT);
    ledcAttachPin(RIGHT_ENB, PWM_CH_RIGHT);

    motorStop();
}

void motorForward(uint8_t speed)  { setLeft(speed, true);  setRight(speed, true);  }
void motorReverse(uint8_t speed)  { setLeft(speed, false); setRight(speed, false); }
void motorSpinLeft(uint8_t speed) { setLeft(speed, false); setRight(speed, true);  }
void motorSpinRight(uint8_t speed){ setLeft(speed, true);  setRight(speed, false); }

void motorArcLeft(uint8_t outer, uint8_t inner) {
    setLeft(inner, true);
    setRight(outer, true);
}
void motorArcRight(uint8_t outer, uint8_t inner) {
    setLeft(outer, true);
    setRight(inner, true);
}
void motorStop() {
    ledcWrite(PWM_CH_LEFT,  0);
    ledcWrite(PWM_CH_RIGHT, 0);
    digitalWrite(LEFT_IN1,  LOW); digitalWrite(LEFT_IN2,  LOW);
    digitalWrite(RIGHT_IN3, LOW); digitalWrite(RIGHT_IN4, LOW);
}
