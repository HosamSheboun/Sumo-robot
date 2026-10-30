// arduino_autonomous.ino
// After power-on: wait 5 seconds, then drive forward at full speed forever.
// Rocker switch OFF → stop instantly. Switch back ON → restart 5-second delay.

#include <Arduino.h>
#include "config.h"
#include "motors.h"

static inline bool switchIsOn() { return digitalRead(SWITCH_PIN) == LOW; }
static inline void ledOn()      { digitalWrite(LED_PIN, HIGH); }
static inline void ledOff()     { digitalWrite(LED_PIN, LOW);  }

void setup() {
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(LED_PIN,    OUTPUT);
    ledOff();
    motorsInit();

    // Wait for rocker switch to be turned ON
    while (!switchIsOn()) {
        ledOn();  delay(200);
        ledOff(); delay(800);
    }

    // 5-second mandatory start delay — LED blinks fast
    unsigned long start = millis();
    while (millis() - start < 5000UL) {
        ledOn();  delay(100);
        ledOff(); delay(100);
    }

    // Armed — LED solid ON, start moving
    ledOn();
    motorForward(SPEED_FULL);
}

void loop() {
    // If switch is flipped OFF, stop immediately
    if (!switchIsOn()) {
        motorStop();
        ledOff();

        // Wait until switch is turned back ON
        while (!switchIsOn()) {
            ledOn();  delay(200);
            ledOff(); delay(800);
        }

        // Restart 5-second delay
        unsigned long start = millis();
        while (millis() - start < 5000UL) {
            ledOn();  delay(100);
            ledOff(); delay(100);
        }

        ledOn();
        motorForward(SPEED_FULL);
    }
}
