// =============================================================================
// arduino_nano.ino — Arduino Nano sumo robot main sketch.
// Identical strategy to arduino_uno but uses Nano pin layout.
// Upload via: Tools → Board → Arduino Nano
//             Tools → Processor → ATmega328P (Old Bootloader) if upload fails
// =============================================================================

#include <Arduino.h>
#include "config.h"
#include "motors.h"

static inline bool switchIsOn() { return digitalRead(SWITCH_PIN) == LOW; }
static inline bool irFront()    { return digitalRead(IR_FRONT) == LOW; }
static inline bool irBack()     { return digitalRead(IR_BACK)  == LOW; }
static inline bool anyEdge()    { return irFront() || irBack(); }
static inline void ledOn()      { digitalWrite(LED_PIN, HIGH); }
static inline void ledOff()     { digitalWrite(LED_PIN, LOW);  }

static float readDistance() {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long dur = pulseIn(ECHO_PIN, HIGH, 25000UL);
    if (dur == 0) return (float)NO_DETECT_CM;
    return dur * 0.0343f / 2.0f;
}

static float getClosingSpeed(float dist) {
    static float prev = NO_DETECT_CM;
    static unsigned long prevT = 0;
    unsigned long now = millis();
    float dt = (now - prevT) / 1000.0f;
    float spd = 0;
    if (dt > 0.01f && prev < NO_DETECT_CM && dist < NO_DETECT_CM)
        spd = (prev - dist) / dt;
    prev = dist; prevT = now;
    return spd;
}

static bool handleEdge() {
    if (!anyEdge()) return false;
    if (irFront() && !irBack()) {
        motorReverse();   delay(EDGE_REVERSE_MS);
        motorSpinRight(); delay(EDGE_SPIN_MS);
    } else if (irBack() && !irFront()) {
        motorForward();   delay(EDGE_REVERSE_MS);
        motorSpinRight(); delay(EDGE_SPIN_MS);
    } else {
        motorReverse();   delay(EDGE_REVERSE_MS + 100);
        motorSpinRight(); delay(EDGE_SPIN_MS + 100);
    }
    motorStop();
    return true;
}

static void waitForSwitch() {
    motorStop(); ledOff();
    while (!switchIsOn()) { ledOn(); delay(200); ledOff(); delay(800); }
}
static void startDelay() {
    unsigned long t = millis();
    while (millis() - t < START_DELAY_MS) { ledOn(); delay(100); ledOff(); delay(100); }
    ledOn();
}
static void spinSearch() {
    if (handleEdge()) return;
    motorSpinRight(); delay(SPIN_STEP_MS); motorStop();
}

static uint8_t dodgeCount = 0;
static void matadorDodge() {
    motorArcRight();  delay(MATADOR_SIDESTEP_MS);
    motorSpinRight(); delay(MATADOR_REFACE_MS);
    motorForward();   delay(MATADOR_SLAM_MS);
    motorStop();
}
static void runMatador(float dist, float spd) {
    if (handleEdge()) return;
    if (dist >= NO_DETECT_CM) spinSearch();
    else if (spd > CHARGE_SPEED_CM_S && dist < ATTACK_DIST_CM && dodgeCount < MATADOR_MAX_DODGES)
        { matadorDodge(); dodgeCount++; }
    else if (dist < ATTACK_DIST_CM) motorForward();
    else { motorForward(); delay(MATADOR_BAIT_MS); motorStop(); }
}

static void wander() {
    if (handleEdge()) return;
    static uint8_t step = 0;
    switch (step % 6) {
        case 0: motorForward();   delay(WANDER_FWD1_MS);  break;
        case 1: motorSpinRight(); delay(WANDER_SPIN1_MS); break;
        case 2: motorForward();   delay(WANDER_FWD2_MS);  break;
        case 3: motorSpinLeft();  delay(WANDER_SPIN2_MS); break;
        case 4: motorForward();   delay(WANDER_FWD3_MS);  break;
        case 5: motorSpinRight(); delay(WANDER_SPIN3_MS); break;
    }
    step++; motorStop();
}
static void runBulldozer(float dist) {
    if (handleEdge()) return;
    if (dist < ATTACK_DIST_CM) motorForward(); else wander();
}

static void flankerArc() {
    if (handleEdge()) return;
    motorArcRight(); delay(FLANKER_ARC_MS); motorStop();
    if (handleEdge()) return;
    if (readDistance() < ATTACK_DIST_CM) { motorForward(); delay(FLANKER_SLAM_MS); motorStop(); }
}
static void runFlanker(float dist) {
    if (handleEdge()) return;
    if (dist >= NO_DETECT_CM) spinSearch();
    else if (dist < ATTACK_DIST_CM) flankerArc();
    else { motorForward(); delay(MATADOR_BAIT_MS); motorStop(); }
}

static void fakeRetreat() {
    if (handleEdge()) return;
    motorReverse(); delay(BERSERKER_RETREAT_MS);
    motorForward(); delay(BERSERKER_CHARGE_MS);
    motorStop();
}
static void runBerserker(float dist) {
    if (handleEdge()) return;
    static uint8_t n = 0;
    if (dist >= NO_DETECT_CM) spinSearch();
    else if (dist < ATTACK_DIST_CM) { if (n%3==0) fakeRetreat(); else motorForward(); n++; }
    else { motorForward(); delay(BERSERKER_CREEP_MS); motorStop(); }
}

static unsigned long matchStart = 0;

void setup() {
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(LED_PIN,    OUTPUT);
    pinMode(TRIG_PIN,   OUTPUT);
    pinMode(ECHO_PIN,   INPUT);
    pinMode(IR_FRONT,   INPUT_PULLUP);
    pinMode(IR_BACK,    INPUT_PULLUP);
    ledOff(); motorsInit();
    waitForSwitch(); startDelay();
    matchStart = millis(); dodgeCount = 0;
}

void loop() {
    if (!switchIsOn()) {
        waitForSwitch(); startDelay();
        matchStart = millis(); dodgeCount = 0; return;
    }
    float dist = readDistance();
    float spd  = getClosingSpeed(dist);
    unsigned long e = millis() - matchStart;
    if      (e < PHASE1_END_MS) runMatador(dist, spd);
    else if (e < PHASE2_END_MS) runBulldozer(dist);
    else if (e < PHASE3_END_MS) runFlanker(dist);
    else                        runBerserker(dist);
}
