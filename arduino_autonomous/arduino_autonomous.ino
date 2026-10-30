// arduino_autonomous.ino — Main entry point for the autonomous sumo robot.
//
// Wires all modules together and runs the FSM at LOOP_MS rate (50Hz).
// Handles rocker switch (instant motor stop, Nano stays on) and LED status.
//
// LED behaviour:
//   SLOW BLINK  = waiting for rocker switch to be turned ON
//   FAST BLINK  = switch ON, 5-second start delay counting down
//   SOLID ON    = robot armed and actively competing
//   OFF         = switch is OFF

#include <Arduino.h>
#include "config.h"
#include "motors.h"
#include "sensors.h"
#include "strategy.h"

// ── Switch & LED helpers ──────────────────────────────────────────────────────
static inline bool switchIsOn() {
    // Switch ON → pin pulled LOW (closed to GND, internal pull-up active)
    return digitalRead(SWITCH_PIN) == LOW;
}

static inline void ledOn()  { digitalWrite(LED_PIN, HIGH); }
static inline void ledOff() { digitalWrite(LED_PIN, LOW);  }

static void ledBlink(uint16_t onMs, uint16_t offMs) {
    ledOn();  delay(onMs);
    ledOff(); delay(offMs);
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(LED_PIN,    OUTPUT);
    ledOff();

    motorsInit();
    encodersInit();
    sensorsInit();

    // Wait for rocker switch to be turned ON before doing anything
    while (!switchIsOn()) {
        ledBlink(200, 800);   // slow blink = waiting for switch
    }

    // Switch is ON — initialise FSM (starts in STATE_START with 5s delay)
    strategyInit();
}

// ── Main loop ─────────────────────────────────────────────────────────────────
void loop() {
    unsigned long t0 = millis();

    // ── Switch OFF check — checked every tick, highest priority ──────────────
    if (!switchIsOn()) {
        motorStop();
        ledOff();

        // Wait here until switch is turned back ON
        while (!switchIsOn()) {
            ledBlink(200, 800);
        }

        // Switch turned back ON — restart FSM from scratch
        strategyInit();
        resetEncoders();
    }

    // ── LED reflects current FSM state ───────────────────────────────────────
    State s = getCurrentState();
    if (s == STATE_START) {
        // Fast blink during start delay to show countdown
        static unsigned long lastToggle = 0;
        static bool ledState = false;
        if (millis() - lastToggle >= 100) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            lastToggle = millis();
        }
    } else {
        ledOn();   // Solid ON = robot is competing
    }

    // ── Read sensors ─────────────────────────────────────────────────────────
    float dist = distanceCm();

    // ── Run FSM tick ──────────────────────────────────────────────────────────
    strategyTick(dist);

    // ── Maintain 50Hz loop rate ───────────────────────────────────────────────
    unsigned long elapsed = millis() - t0;
    if (elapsed < LOOP_MS) {
        delay(LOOP_MS - elapsed);
    }
}
