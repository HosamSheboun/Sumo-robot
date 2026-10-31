// =============================================================================
// esp32_sumo.ino — ESP32 sumo robot main sketch.
//
// Why ESP32 is better than Uno/Nano for sumo:
//   - Dual-core: sensors read on core 0, strategy runs on core 1 simultaneously
//   - Hardware PWM (LEDC): smooth speed control, no jitter
//   - Hardware ultrasonic timer: microsecond-accurate, non-blocking
//   - 240MHz: faster reaction time, tighter control loops
//   - Both IR sensors + ultrasonic used every single tick
//
// Strategy: full 5-state priority FSM + 2-minute technique timeline
//   Priority (checked every tick, highest first):
//     1. EDGE_AVOID  — IR detects white line → escape immediately
//     2. RAM         — opponent < 20cm → full burst push
//     3. Phase-based — MATADOR / BULLDOZER / FLANKER / BERSERKER
//
// Install ESP32 board support:
//   Arduino IDE → File → Preferences → Additional Boards Manager URLs:
//   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
//   Then: Tools → Board → Boards Manager → search "esp32" → install
//   Select: Tools → Board → ESP32 Arduino → ESP32 Dev Module
// =============================================================================

#include <Arduino.h>
#include "config.h"
#include "motors.h"

// =============================================================================
// SECTION 1 — Sensor reading (runs on core 0 via FreeRTOS task)
// =============================================================================

// Shared sensor state — written by core 0, read by core 1
volatile float    g_dist       = NO_DETECT_CM;
volatile float    g_closingSpd = 0.0f;
volatile bool     g_irFront    = false;
volatile bool     g_irBack     = false;
portMUX_TYPE      g_mux        = portMUX_INITIALIZER_UNLOCKED;

// Hardware ultrasonic measurement using pulseIn (blocking but fast on ESP32)
static float measureDistance() {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long dur = pulseIn(ECHO_PIN, HIGH, 25000UL);
    if (dur == 0) return (float)NO_DETECT_CM;
    return dur * 0.0343f / 2.0f;
}

// Sensor task — runs continuously on core 0
void sensorTask(void* param) {
    float prev  = NO_DETECT_CM;
    unsigned long prevT = 0;

    while (true) {
        float d = measureDistance();
        unsigned long now = millis();
        float dt  = (now - prevT) / 1000.0f;
        float spd = 0;
        if (dt > 0.005f && prev < NO_DETECT_CM && d < NO_DETECT_CM)
            spd = (prev - d) / dt;
        prev  = d;
        prevT = now;

        bool front = (digitalRead(IR_FRONT) == LOW);
        bool back  = (digitalRead(IR_BACK)  == LOW);

        portENTER_CRITICAL(&g_mux);
        g_dist       = d;
        g_closingSpd = spd;
        g_irFront    = front;
        g_irBack     = back;
        portEXIT_CRITICAL(&g_mux);

        vTaskDelay(1); // yield to RTOS — ~1ms between readings
    }
}

// Safe reads from strategy task
static float  getDist()    { float v; portENTER_CRITICAL(&g_mux); v=g_dist;       portEXIT_CRITICAL(&g_mux); return v; }
static float  getSpd()     { float v; portENTER_CRITICAL(&g_mux); v=g_closingSpd; portEXIT_CRITICAL(&g_mux); return v; }
static bool   getIrFront() { bool  v; portENTER_CRITICAL(&g_mux); v=g_irFront;    portEXIT_CRITICAL(&g_mux); return v; }
static bool   getIrBack()  { bool  v; portENTER_CRITICAL(&g_mux); v=g_irBack;     portEXIT_CRITICAL(&g_mux); return v; }
static bool   anyEdge()    { return getIrFront() || getIrBack(); }

// =============================================================================
// SECTION 2 — I/O helpers
// =============================================================================
static inline bool switchIsOn() { return digitalRead(SWITCH_PIN) == LOW; }
static inline void ledOn()      { digitalWrite(LED_PIN, HIGH); }
static inline void ledOff()     { digitalWrite(LED_PIN, LOW);  }

// =============================================================================
// SECTION 3 — Edge avoidance
// =============================================================================
static bool handleEdge() {
    if (!anyEdge()) return false;
    bool front = getIrFront();
    bool back  = getIrBack();

    if (front && !back) {
        motorReverse(SPEED_FULL); delay(EDGE_REVERSE_MS);
        motorSpinRight(SPEED_FULL); delay(EDGE_SPIN_MS);
    } else if (back && !front) {
        motorForward(SPEED_FULL); delay(EDGE_REVERSE_MS);
        motorSpinRight(SPEED_FULL); delay(EDGE_SPIN_MS);
    } else {
        motorReverse(SPEED_FULL); delay(EDGE_REVERSE_MS + 100);
        motorSpinRight(SPEED_FULL); delay(EDGE_SPIN_MS + 100);
    }
    motorStop();
    return true;
}

// =============================================================================
// SECTION 4 — RAM burst (opponent very close)
// =============================================================================
static bool handleRam(float dist) {
    if (dist >= RAM_DIST_CM) return false;
    motorForward(SPEED_FULL);
    delay(RAM_BURST_MS);
    return true;
}

// =============================================================================
// SECTION 5 — Switch and LED
// =============================================================================
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
    motorSpinRight(SPEED_SEARCH); delay(SPIN_STEP_MS); motorStop();
}

// =============================================================================
// SECTION 6 — MATADOR (0:00–0:30)
// Proven winning technique from competition GitHub repos:
// Bait the charge → sidestep → counter from the side.
// The arc uses speed differential (outer faster than inner) for a smooth curve.
// =============================================================================
static uint8_t dodgeCount = 0;

static void matadorDodge() {
    motorArcRight(SPEED_FULL, SPEED_ARC_IN); delay(MATADOR_SIDESTEP_MS);
    motorSpinRight(SPEED_FULL);              delay(MATADOR_REFACE_MS);
    motorForward(SPEED_FULL);                delay(MATADOR_SLAM_MS);
    motorStop();
}
static void runMatador(float dist, float spd) {
    if (handleEdge()) return;
    if (handleRam(dist)) return;
    if (dist >= NO_DETECT_CM) spinSearch();
    else if (spd > CHARGE_SPEED_CM_S && dist < ATTACK_DIST_CM && dodgeCount < MATADOR_MAX_DODGES)
        { matadorDodge(); dodgeCount++; }
    else if (dist < ATTACK_DIST_CM) motorForward(SPEED_FULL);
    else { motorForward(SPEED_STALK); delay(MATADOR_BAIT_MS); motorStop(); }
}

// =============================================================================
// SECTION 7 — BULLDOZER (0:30–1:00)
// =============================================================================
static void wander() {
    if (handleEdge()) return;
    static uint8_t step = 0;
    switch (step % 6) {
        case 0: motorForward(SPEED_STALK);   delay(WANDER_FWD1_MS);  break;
        case 1: motorSpinRight(SPEED_SEARCH);delay(WANDER_SPIN1_MS); break;
        case 2: motorForward(SPEED_STALK);   delay(WANDER_FWD2_MS);  break;
        case 3: motorSpinLeft(SPEED_SEARCH); delay(WANDER_SPIN2_MS); break;
        case 4: motorForward(SPEED_STALK);   delay(WANDER_FWD3_MS);  break;
        case 5: motorSpinRight(SPEED_SEARCH);delay(WANDER_SPIN3_MS); break;
    }
    step++; motorStop();
}
static void runBulldozer(float dist) {
    if (handleEdge()) return;
    if (handleRam(dist)) return;
    if (dist < ATTACK_DIST_CM) motorForward(SPEED_FULL); else wander();
}

// =============================================================================
// SECTION 8 — FLANKER (1:00–1:30)
// Smooth arc using speed differential — outer wheel faster than inner.
// Approaches from the side where opponent is weakest.
// =============================================================================
static void flankerArc() {
    if (handleEdge()) return;
    motorArcRight(SPEED_ARC_OUT, SPEED_ARC_IN); delay(FLANKER_ARC_MS); motorStop();
    if (handleEdge()) return;
    if (getDist() < ATTACK_DIST_CM) { motorForward(SPEED_FULL); delay(FLANKER_SLAM_MS); motorStop(); }
}
static void runFlanker(float dist) {
    if (handleEdge()) return;
    if (handleRam(dist)) return;
    if (dist >= NO_DETECT_CM) spinSearch();
    else if (dist < ATTACK_DIST_CM) flankerArc();
    else { motorForward(SPEED_STALK); delay(MATADOR_BAIT_MS); motorStop(); }
}

// =============================================================================
// SECTION 9 — BERSERKER (1:30–2:00)
// =============================================================================
static void fakeRetreat() {
    if (handleEdge()) return;
    motorReverse(SPEED_FULL); delay(BERSERKER_RETREAT_MS);
    motorForward(SPEED_FULL); delay(BERSERKER_CHARGE_MS);
    motorStop();
}
static void runBerserker(float dist) {
    if (handleEdge()) return;
    if (handleRam(dist)) return;
    static uint8_t n = 0;
    if (dist >= NO_DETECT_CM) spinSearch();
    else if (dist < ATTACK_DIST_CM) { if (n%3==0) fakeRetreat(); else motorForward(SPEED_FULL); n++; }
    else { motorForward(SPEED_STALK); delay(BERSERKER_CREEP_MS); motorStop(); }
}

// =============================================================================
// SECTION 10 — Setup and main loop (runs on core 1)
// =============================================================================
static unsigned long matchStart = 0;

void setup() {
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(LED_PIN,    OUTPUT);
    pinMode(TRIG_PIN,   OUTPUT);
    pinMode(ECHO_PIN,   INPUT);
    pinMode(IR_FRONT,   INPUT_PULLUP);
    pinMode(IR_BACK,    INPUT_PULLUP);
    ledOff();
    motorsInit();

    // Start sensor task on core 0 with 4KB stack
    xTaskCreatePinnedToCore(sensorTask, "sensors", 4096, NULL, 1, NULL, 0);

    waitForSwitch();
    startDelay();
    matchStart = millis();
    dodgeCount = 0;
}

void loop() {
    if (!switchIsOn()) {
        waitForSwitch(); startDelay();
        matchStart = millis(); dodgeCount = 0; return;
    }

    float dist = getDist();
    float spd  = getSpd();
    unsigned long e = millis() - matchStart;

    if      (e < PHASE1_END_MS) runMatador(dist, spd);
    else if (e < PHASE2_END_MS) runBulldozer(dist);
    else if (e < PHASE3_END_MS) runFlanker(dist);
    else                        runBerserker(dist);
}
