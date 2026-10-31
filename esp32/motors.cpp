// =============================================================================
// motors.cpp — L298N motor control for ESP32 using LEDC hardware PWM.
// Compatible with both ESP32 Arduino Core 2.x and Core 3.x.
// =============================================================================
#include "motors.h"
#include <esp_arduino_version.h>

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
// ESP32 Arduino Core 3.0+ API
static void writeLeftPwm(uint8_t speed)  { ledcWrite(LEFT_ENA, speed);  }
static void writeRightPwm(uint8_t speed) { ledcWrite(RIGHT_ENB, speed); }
#else
// ESP32 Arduino Core 2.0.x API
static void writeLeftPwm(uint8_t speed)  { ledcWrite(PWM_CH_LEFT, speed);  }
static void writeRightPwm(uint8_t speed) { ledcWrite(PWM_CH_RIGHT, speed); }
#endif

static void setLeft(uint8_t speed, bool fwd) {
    digitalWrite(LEFT_IN1, fwd ? HIGH : LOW);
    digitalWrite(LEFT_IN2, fwd ? LOW  : HIGH);
    writeLeftPwm(speed);
}

static void setRight(uint8_t speed, bool fwd) {
    digitalWrite(RIGHT_IN3, fwd ? HIGH : LOW);
    digitalWrite(RIGHT_IN4, fwd ? LOW  : HIGH);
    writeRightPwm(speed);
}

void motorsInit() {
    pinMode(LEFT_IN1,  OUTPUT);
    pinMode(LEFT_IN2,  OUTPUT);
    pinMode(RIGHT_IN3, OUTPUT);
    pinMode(RIGHT_IN4, OUTPUT);

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcAttach(LEFT_ENA,  PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(RIGHT_ENB, PWM_FREQ, PWM_RESOLUTION);
#else
    ledcSetup(PWM_CH_LEFT,  PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CH_RIGHT, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LEFT_ENA,  PWM_CH_LEFT);
    ledcAttachPin(RIGHT_ENB, PWM_CH_RIGHT);
#endif

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
    writeLeftPwm(0);
    writeRightPwm(0);
    digitalWrite(LEFT_IN1,  LOW);
    digitalWrite(LEFT_IN2,  LOW);
    digitalWrite(RIGHT_IN3, LOW);
    digitalWrite(RIGHT_IN4, LOW);
}
