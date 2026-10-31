// =============================================================================
// arduino_autonomous.ino — Main sketch for the Arduino Uno sumo robot.
//
// 2-minute match technique timeline:
//   0:00 – 0:30  MATADOR   — bait the opponent's charge, dodge sideways,
//                             counter-attack from the side (up to 4 dodges)
//   0:30 – 1:00  BULLDOZER — wander the ring in a pattern, charge and push
//                             anything the sensor finds
//   1:00 – 1:30  FLANKER   — arc wide around the opponent, slam into its side
//   1:30 – 2:00  BERSERKER — full aggression: fake retreat then charge,
//                             unpredictable against strong bots
//
// All tuning is done in config.h only. Never hardcode numbers here.
// =============================================================================

#include <Arduino.h>
#include "config.h"
#include "motors.h"

// =============================================================================
// SECTION 1 — Basic I/O helpers
// =============================================================================

static inline bool switchIsOn() { return digitalRead(SWITCH_PIN) == LOW; }
static inline void ledOn()      { digitalWrite(LED_PIN, HIGH); }
static inline void ledOff()     { digitalWrite(LED_PIN, LOW);  }

// =============================================================================
// SECTION 2 — Ultrasonic sensor
// =============================================================================

// Returns distance to nearest object in cm.
// Returns NO_DETECT_CM if nothing is detected within range.
static float readDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 25000UL); // 25ms timeout ≈ 430cm max
    if (duration == 0) return (float)NO_DETECT_CM;
    return duration * 0.0343f / 2.0f;
}

// Returns how fast (cm/s) the opponent is closing toward us.
// Positive value = opponent moving toward us.
// Negative value = opponent moving away.
// Returns 0 if not enough data yet.
static float getClosingSpeed(float dist) {
    static float         prevDist = NO_DETECT_CM;
    static unsigned long prevTime = 0;

    unsigned long now = millis();
    float dt  = (now - prevTime) / 1000.0f;
    float spd = 0.0f;

    if (dt > 0.01f && prevDist < NO_DETECT_CM && dist < NO_DETECT_CM)
        spd = (prevDist - dist) / dt;

    prevDist = dist;
    prevTime = now;
    return spd;
}

// =============================================================================
// SECTION 3 — Switch and LED state machine
// =============================================================================

// Blocks until the rocker switch is turned ON.
// LED slow-blinks while waiting.
static void waitForSwitch() {
    motorStop();
    ledOff();
    while (!switchIsOn()) {
        ledOn();  delay(200);
        ledOff(); delay(800);
    }
}

// Mandatory start delay — LED fast-blinks, robot sits still.
// Duration set by START_DELAY_MS in config.h.
static void startDelay() {
    unsigned long t = millis();
    while (millis() - t < START_DELAY_MS) {
        ledOn();  delay(100);
        ledOff(); delay(100);
    }
    ledOn(); // solid ON = robot is now active
}

// =============================================================================
// SECTION 4 — Shared movement primitives
// =============================================================================

// Spin right in place for SPIN_STEP_MS then stop.
// Used by all phases when no opponent is detected.
static void spinSearch() {
    motorSpinRight();
    delay(SPIN_STEP_MS);
    motorStop();
}

// =============================================================================
// SECTION 5 — Phase 1: MATADOR (0:00 – 0:30)
//
// Strategy: creep forward to bait the opponent into charging.
// When it charges, sidestep right to let it run past, then spin to face it
// and slam into its side. After MATADOR_MAX_DODGES successful dodges,
// switch to direct attacks for the rest of this phase.
// =============================================================================

static uint8_t dodgeCount = 0; // global so it resets correctly on restart

static void matadorDodge() {
    motorArcRight();              // sidestep right — let charge run past our left
    delay(MATADOR_SIDESTEP_MS);
    motorSpinRight();             // spin to face where opponent ended up
    delay(MATADOR_REFACE_MS);
    motorForward();               // counter-attack slam into opponent's side
    delay(MATADOR_SLAM_MS);
    motorStop();
}

static void runMatador(float dist, float spd) {
    if (dist >= NO_DETECT_CM) {
        // No opponent visible — spin to search
        spinSearch();

    } else if (spd > CHARGE_SPEED_CM_S && dist < ATTACK_DIST_CM
               && dodgeCount < MATADOR_MAX_DODGES) {
        // Opponent is charging at us — execute the matador dodge
        matadorDodge();
        dodgeCount++;

    } else if (dist < ATTACK_DIST_CM) {
        // Opponent is close but not charging (or dodges used up) — attack directly
        motorForward();

    } else {
        // Opponent visible but far — creep forward slowly to bait a charge
        motorForward();
        delay(MATADOR_BAIT_MS);
        motorStop();
    }
}

// =============================================================================
// SECTION 6 — Phase 2: BULLDOZER (0:30 – 1:00)
//
// Strategy: roam the ring in a repeating wander pattern to cover ground.
// The moment the sensor finds the opponent, drop everything and charge.
// Good against opponents that hide or stay still.
// =============================================================================

static void wander() {
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
    if (dist < ATTACK_DIST_CM) {
        motorForward(); // charge and push — hold it until opponent is gone
    } else {
        wander();
    }
}

// =============================================================================
// SECTION 7 — Phase 3: FLANKER (1:00 – 1:30)
//
// Strategy: instead of charging head-on, arc wide around the opponent to
// approach from the side where it is harder to defend. Strong bots often
// have their best pushing power at the front — hitting the side bypasses that.
// =============================================================================

static void flankerArc(float dist) {
    motorArcRight();              // arc wide around the opponent's right side
    delay(FLANKER_ARC_MS);
    motorStop();

    // Re-read sensor after the arc — opponent may have moved
    float newDist = readDistance();
    if (newDist < ATTACK_DIST_CM) {
        motorForward();           // slam into the side
        delay(FLANKER_SLAM_MS);
        motorStop();
    }
    // If opponent moved away, fall back to main loop which will spinSearch
}

static void runFlanker(float dist) {
    if (dist >= NO_DETECT_CM) {
        spinSearch();
    } else if (dist < ATTACK_DIST_CM) {
        flankerArc(dist);
    } else {
        // Opponent visible but far — move toward it to get within arc range
        motorForward();
        delay(MATADOR_BAIT_MS);
        motorStop();
    }
}

// =============================================================================
// SECTION 8 — Phase 4: BERSERKER (1:30 – 2:00)
//
// Strategy: pure aggression with unpredictability. Every 3rd attack does a
// fake retreat (reverse briefly then charge) to confuse opponents that try
// to dodge or counter. The rest of the time it charges directly.
// This is the last 30 seconds — no time for subtlety.
// =============================================================================

static void fakeRetreat() {
    motorReverse();               // short real reverse — looks like retreating
    delay(BERSERKER_RETREAT_MS);
    motorForward();               // immediate full charge — opponent is confused
    delay(BERSERKER_CHARGE_MS);
    motorStop();
}

static void runBerserker(float dist) {
    static uint8_t attackCount = 0;

    if (dist >= NO_DETECT_CM) {
        spinSearch();

    } else if (dist < ATTACK_DIST_CM) {
        if (attackCount % 3 == 0) {
            fakeRetreat();        // every 3rd attack: fake retreat then charge
        } else {
            motorForward();       // other attacks: direct charge
        }
        attackCount++;

    } else {
        // Opponent visible but outside attack range — close the gap fast
        motorForward();
        delay(BERSERKER_CREEP_MS);
        motorStop();
    }
}

// =============================================================================
// SECTION 9 — Setup and main loop
// =============================================================================

static unsigned long matchStart = 0;

void setup() {
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(LED_PIN,    OUTPUT);
    pinMode(TRIG_PIN,   OUTPUT);
    pinMode(ECHO_PIN,   INPUT);
    ledOff();
    motorsInit();
    waitForSwitch();
    startDelay();
    matchStart = millis();
    dodgeCount = 0;
}

void loop() {
    // If switch is turned OFF mid-match — stop everything and wait for restart
    if (!switchIsOn()) {
        waitForSwitch();
        startDelay();
        matchStart = millis();
        dodgeCount = 0;
        return;
    }

    float dist    = readDistance();
    float spd     = getClosingSpeed(dist);
    unsigned long elapsed = millis() - matchStart;

    // Dispatch to the correct phase based on elapsed match time
    if      (elapsed < PHASE1_END_MS) runMatador(dist, spd);  // 0:00 – 0:30
    else if (elapsed < PHASE2_END_MS) runBulldozer(dist);     // 0:30 – 1:00
    else if (elapsed < PHASE3_END_MS) runFlanker(dist);       // 1:00 – 1:30
    else                              runBerserker(dist);      // 1:30 – 2:00
}
