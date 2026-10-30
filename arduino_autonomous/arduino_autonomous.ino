// arduino_autonomous.ino
// Strategy: spin in place to search for opponent → charge when detected → spin again if lost.
// Rocker switch OFF → stop instantly. Switch back ON → restart 5-second delay.

#include <Arduino.h>
#include "config.h"
#include "motors.h"

static inline bool switchIsOn() { return digitalRead(SWITCH_PIN) == LOW; }
static inline void ledOn()      { digitalWrite(LED_PIN, HIGH); }
static inline void ledOff()     { digitalWrite(LED_PIN, LOW);  }

// Returns distance in cm. Returns NO_DETECT_CM if nothing seen.
static float readDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 25000UL); // 25ms timeout ≈ 430cm
    if (duration == 0) return NO_DETECT_CM;
    return duration * 0.0343f / 2.0f;
}

static void waitForSwitch() {
    motorStop();
    ledOff();
    while (!switchIsOn()) {
        ledOn();  delay(200);
        ledOff(); delay(800);
    }
}

static void startDelay() {
    unsigned long start = millis();
    while (millis() - start < START_DELAY_MS) {
        ledOn();  delay(100);
        ledOff(); delay(100);
    }
    ledOn();
}

void setup() {
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(LED_PIN,    OUTPUT);
    pinMode(TRIG_PIN,   OUTPUT);
    pinMode(ECHO_PIN,   INPUT);
    ledOff();
    motorsInit();

    waitForSwitch();
    startDelay();
}

void loop() {
    if (!switchIsOn()) {
        waitForSwitch();
        startDelay();
        return;
    }

    float dist = readDistance();

    if (dist < ATTACK_DIST_CM) {
        // Opponent detected — charge!
        motorForward(SPEED_FULL);
    } else {
        // Nothing seen — spin right to search
        motorSpinRight(SPEED_SPIN);
        delay(SPIN_STEP_MS);
    }
}
