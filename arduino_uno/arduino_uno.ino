// =============================================================================
// arduino_uno.ino — Arduino Uno sumo robot main sketch.
//
// Hardware used every tick:
//   HC-SR04 ultrasonic  — detects opponent distance (front-facing)
//   IR front sensor     — detects white boundary line ahead
//   IR back sensor      — detects white boundary line behind
//   2× GA25-370 motors  — driven through L298N
//
// Power: L298N 5V output pin → Uno 5V pin. No MT3608 needed.
//
// 2-minute technique timeline:
//   0:00–0:30  MATADOR   — bait charge, dodge, counter from side
//   0:30–1:00  BULLDOZER — wander ring, charge anything found
//   1:00–1:30  FLANKER   — arc around opponent, hit from side
//   1:30–2:00  BERSERKER — fake retreat then charge, pure aggression
//
// Edge avoidance runs at the TOP of every phase — it always wins.
// =============================================================================

#include <Arduino.h>
#include "config.h"
#include "motors.h"
#include "advanced_features.h"

// =============================================================================
// SECTION 1 — I/O helpers
// =============================================================================
static inline bool switchIsOn()  { return digitalRead(SWITCH_PIN) == LOW; }

#if ENABLE_SENSOR_FUSION
// Use sensor fusion (majority-vote) for edge detection
static inline bool irFront()     { return irFusedFront(); }
static inline bool irBack()      { return irFusedBack(); }
#else
// Use direct sensor reads (default)
static inline bool irFront()     { return digitalRead(IR_FRONT) == LOW; }
static inline bool irBack()      { return digitalRead(IR_BACK)  == LOW; }
#endif

static inline bool anyEdge()     { return irFront() || irBack(); }
static inline void ledOn()       { digitalWrite(LED_PIN, HIGH); }
static inline void ledOff()      { digitalWrite(LED_PIN, LOW);  }

// =============================================================================
// SECTION 2 — Ultrasonic sensor
// =============================================================================
static float readDistance() {
    digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long dur = pulseIn(ECHO_PIN, HIGH, 30000UL);
    if (dur == 0) return (float)NO_DETECT_CM;
    return dur * 0.0343f / 2.0f;
}

// Global flag to signal match restart to getClosingSpeed()
static bool g_needSpeedReset = false;

static float getClosingSpeed(float dist) {
    static float         prev  = NO_DETECT_CM;
    static unsigned long prevT = 0;
    unsigned long now = millis();
    
    // Check for explicit reset request (match restart)
    if (g_needSpeedReset) {
        prev  = NO_DETECT_CM;
        prevT = 0;
        g_needSpeedReset = false;
    }
    
    // First-call guard: initialize without calculating speed
    if (prevT == 0) {
        prev  = dist;
        prevT = now;
        return 0;
    }
    
    float dt = (now - prevT) / 1000.0f;
    float spd = 0;
    
    // Only compute speed if dt is reasonable (100ms–1000ms) and both measurements are valid
    if (dt > 0.1f && dt < 1.0f && prev < NO_DETECT_CM && dist < NO_DETECT_CM)
        spd = (prev - dist) / dt;
    
    prev  = dist;
    prevT = now;
    return spd;
}

// Reset getClosingSpeed() state on match restart
static void resetClosingSpeed() {
    g_needSpeedReset = true;
}

// =============================================================================
// SECTION 3 — Edge avoidance (called at top of every phase)
// Returns true if edge was detected and handled — caller must return immediately.
// =============================================================================
static bool handleEdge() {
    if (!anyEdge()) return false;

    if (irFront() && !irBack()) {
        // Front sensor hit — reverse away, then spin right to face ring centre
        motorReverse(); delay(EDGE_REVERSE_MS);
        motorSpinRight(); delay(EDGE_SPIN_MS);
    } else if (irBack() && !irFront()) {
        // Back sensor hit — drive forward away, then spin right
        motorForward(); delay(EDGE_REVERSE_MS);
        motorSpinRight(); delay(EDGE_SPIN_MS);
    } else {
        // Both sensors hit (corner) — reverse hard and spin
        motorReverse(); delay(EDGE_REVERSE_MS + 100);
        motorSpinRight(); delay(EDGE_SPIN_MS + 100);
    }
    motorStop();
    return true;
}

// =============================================================================
// SECTION 4 — Switch and LED
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

// =============================================================================
// SECTION 5 — Shared search primitive
// =============================================================================
static void spinSearch() {
    if (handleEdge()) return;
    
#if ENABLE_SEARCH_MEMORY
    // Use search memory to bias direction toward last-seen opponent
    bool spinRight = getSearchDirection();
    if (spinRight) {
        motorSpinRight();
    } else {
        motorSpinLeft();
    }
    
    delay(SPIN_STEP_MS);
    motorStop();
    
    // After spinning, check if we found the opponent
    float dist = readDistance();
    if (dist < NO_DETECT_CM) {
        // Update memory with actual spin direction used
        updateSearchMemory(dist, spinRight);
    }
#else
    // Default: always spin right
    motorSpinRight();
    delay(SPIN_STEP_MS);
    motorStop();
#endif
}

// =============================================================================
// SECTION 6 — MATADOR phase (0:00–0:30)
// Proven GitHub technique: bait the charge, sidestep, counter from the side.
// Used by top competitors to beat aggressive pushers.
// =============================================================================
static uint8_t dodgeCount = 0;

static void matadorDodge() {
    motorArcRight(); delay(MATADOR_SIDESTEP_MS);  // sidestep — let charge run past
    motorSpinRight(); delay(MATADOR_REFACE_MS);   // face where opponent ended up
    motorForward();  delay(MATADOR_SLAM_MS);       // slam into its side
    motorStop();
}

static void runMatador(float dist, float spd) {
    if (handleEdge()) return;

    if (dist >= NO_DETECT_CM) {
        spinSearch();
    } else if (spd > CHARGE_SPEED_CM_S && dist < ATTACK_DIST_CM
               && dodgeCount < MATADOR_MAX_DODGES) {
        matadorDodge();
        dodgeCount++;
    } else if (dist < ATTACK_DIST_CM) {
        motorForward();
    } else {
        motorForward(); delay(MATADOR_BAIT_MS); motorStop();
    }
}

// =============================================================================
// SECTION 7 — BULLDOZER phase (0:30–1:00)
// Wander pattern covers the whole ring. Charge anything found.
// Effective against passive or hiding opponents.
// =============================================================================
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
    step++;
    motorStop();
}

static void runBulldozer(float dist) {
    if (handleEdge()) return;
    if (dist < ATTACK_DIST_CM) motorForward();
    else wander();
}

// =============================================================================
// SECTION 8 — FLANKER phase (1:00–1:30)
// Arc wide around opponent, hit from the side.
// Bypasses strong front pushers — their weakest point is the side.
// =============================================================================
static void flankerArc() {
    if (handleEdge()) return;
    motorArcRight(); delay(FLANKER_ARC_MS); motorStop();
    if (handleEdge()) return;
    float d = readDistance();
    if (d < ATTACK_DIST_CM) { motorForward(); delay(FLANKER_SLAM_MS); motorStop(); }
}

static void runFlanker(float dist) {
    if (handleEdge()) return;
    if (dist >= NO_DETECT_CM)      spinSearch();
    else if (dist < ATTACK_DIST_CM) flankerArc();
    else { motorForward(); delay(MATADOR_BAIT_MS); motorStop(); }
}

// =============================================================================
// SECTION 9 — BERSERKER phase (1:30–2:00)
// Fake retreat then charge. Every 3rd attack uses the fake to confuse.
// Last 30 seconds — maximum unpredictability.
// =============================================================================
static void fakeRetreat() {
    if (handleEdge()) return;
    motorReverse(); delay(BERSERKER_RETREAT_MS);
    motorForward(); delay(BERSERKER_CHARGE_MS);
    motorStop();
}

static void runBerserker(float dist) {
    if (handleEdge()) return;
    static uint8_t n = 0;
    if (dist >= NO_DETECT_CM) {
        spinSearch();
    } else if (dist < ATTACK_DIST_CM) {
        if (n % 3 == 0) fakeRetreat(); else motorForward();
        n++;
    } else {
        motorForward(); delay(BERSERKER_CREEP_MS); motorStop();
    }
}

// =============================================================================
// SECTION 10 — Setup and main loop
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
    
#if ENABLE_KICKSTART
    // NOTE: Kickstart has no effect with ENA/ENB jumpers installed.
    // Only useful in PWM builds where ENA/ENB are wired to Arduino pins.
    motorKickstart();
#endif
    
    waitForSwitch();
    startDelay();
    matchStart = millis();
    dodgeCount = 0;
}

void loop() {
    if (!switchIsOn()) {
        waitForSwitch(); startDelay();
        matchStart = millis(); 
        dodgeCount = 0;
        resetClosingSpeed();  // Reset closing speed calculation on match restart
#if ENABLE_SEARCH_MEMORY
        resetSearchMemory();
#endif
#if ENABLE_ROLLING_SPEED
        resetRollingSpeed();
#endif
        return;
    }

    float dist = readDistance();
    
#if ENABLE_ROLLING_SPEED
    // Use rolling average for closing speed (smooths ultrasonic jitter)
    float spd = getRollingClosingSpeed(dist);
#else
    // Use direct closing speed calculation (default)
    float spd = getClosingSpeed(dist);
#endif
    
    unsigned long elapsed = millis() - matchStart;

    if      (elapsed < PHASE1_END_MS) runMatador(dist, spd);
    else if (elapsed < PHASE2_END_MS) runBulldozer(dist);
    else if (elapsed < PHASE3_END_MS) runFlanker(dist);
    else                              runBerserker(dist);
}
