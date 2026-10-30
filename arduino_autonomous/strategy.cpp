// strategy.cpp — Adaptive priority FSM for heavyweight sumo competition.
//
// State priority (highest → lowest):
//   STATE_EDGE_AVOID — white line detected; immediate escape maneuver
//   STATE_RAM        — opponent < CLOSE_DISTANCE_CM; burst push
//   STATE_ATTACK     — opponent < ATTACK_DISTANCE_CM; full charge + heading correction
//   STATE_STALK      — opponent visible but far; arc toward it
//   STATE_ORBIT      — flanking arc (FLANKER tactic only)
//   STATE_SEARCH     — no opponent; 360° spin-scan
//   STATE_START      — mandatory 5-second pre-match delay
//
// TacticAdaptor switches between RUSHER / STALKER / FLANKER based on:
//   - Edge hits during the match (being pushed = switch to FLANKER)
//   - Successful pushes (charges working = stay RUSHER)
//   - Match time > 150s (force RUSHER regardless)

#include "strategy.h"
#include "motors.h"
#include "sensors.h"

// ── State machine variables ───────────────────────────────────────────────────
static State         _state      = STATE_START;
static unsigned long _stateStart = 0;

// ── Tactic adaptor variables ──────────────────────────────────────────────────
static Tactic        _tactic         = TACTIC_RUSHER;
static uint8_t       _edgeHits       = 0;
static uint8_t       _pushSuccesses  = 0;
static unsigned long _matchStart     = 0;

// ── Scan tracker variables ────────────────────────────────────────────────────
static int8_t        _scanDir        = 1;    // +1 = clockwise, -1 = CCW
static float         _scanCovered    = 0.0f; // degrees covered this sweep
static unsigned long _scanStepStart  = 0;

// ── Edge avoidance context ────────────────────────────────────────────────────
static bool          _avoidFwdAfter  = false; // true = back sensor fired

// ── RAM context ───────────────────────────────────────────────────────────────
static unsigned long _ramStart       = 0;
static float         _ramDistBefore  = 999.0f;

// ── Orbit context ─────────────────────────────────────────────────────────────
static unsigned long _orbitStart     = 0;

// ── Spin degrees per tick estimate ───────────────────────────────────────────
// Approximate degrees the robot rotates per LOOP_MS tick at SPEED_SEARCH.
// Tune this value if the tracker bearing drifts noticeably.
static const float SPIN_DEG_PER_TICK =
    (float)SPEED_SEARCH / 255.0f * 360.0f / (1000.0f / LOOP_MS * 0.6f);

static int8_t _lastSpinDir = 0;

// ── Helpers ───────────────────────────────────────────────────────────────────
static unsigned long elapsed() { return millis() - _stateStart; }

static void goTo(State s) {
    _state      = s;
    _stateStart = millis();
}

static Tactic getTactic() {
    // Force RUSHER in final 30 seconds of a 3-minute match
    if (millis() - _matchStart > 150000UL) return TACTIC_RUSHER;
    return _tactic;
}

static void recordEdgeHit() {
    _edgeHits++;
    if (_edgeHits >= 3) _tactic = TACTIC_FLANKER;
}

static void recordPushSuccess() {
    _pushSuccesses++;
    if (_pushSuccesses >= 2) _tactic = TACTIC_RUSHER;
}

static void triggerEdgeAvoid() {
    recordEdgeHit();
    _avoidFwdAfter = edgeBack() && !edgeFront();
    goTo(STATE_EDGE_AVOID);
}

// ── State handlers ────────────────────────────────────────────────────────────

static void doStart() {
    motorStop();
    if (elapsed() >= START_DELAY_MS) goTo(STATE_SEARCH);
}

static void doEdgeAvoid() {
    unsigned long t       = elapsed();
    unsigned long revEnd  = REVERSE_DURATION_MS;
    unsigned long spinEnd = revEnd + SPIN_DURATION_MS;

    if (t < revEnd) {
        // Phase 1: reverse away from boundary
        if (_avoidFwdAfter) motorForward(SPEED_REVERSE);
        else                motorReverse(SPEED_REVERSE);
    } else if (t < spinEnd) {
        // Phase 2: spin to face ring centre
        motorSpinRight(SPEED_TURN);
    } else {
        goTo(STATE_SEARCH);
    }
}

static void doRam(float dist) {
    if (anyEdge()) { triggerEdgeAvoid(); return; }

    if (millis() - _ramStart > RAM_BURST_MS) {
        if (dist > _ramDistBefore + 5.0f) recordPushSuccess();
        goTo(STATE_ATTACK);
    } else {
        motorForward(SPEED_FULL);
    }
}

static void doAttack(float dist) {
    if (anyEdge()) { triggerEdgeAvoid(); return; }

    if (dist < CLOSE_DISTANCE_CM) {
        _ramStart      = millis();
        _ramDistBefore = dist;
        goTo(STATE_RAM);
        return;
    }

    if (dist <= ATTACK_DISTANCE_CM) {
        float err = headingError();
        if (err > 5.0f) {
            motorArcRight(SPEED_FULL, SPEED_FULL - SPEED_CORRECT);
        } else if (err < -5.0f) {
            motorArcLeft(SPEED_FULL, SPEED_FULL - SPEED_CORRECT);
        } else {
            motorForward(SPEED_FULL);
        }
        resetEncoders();
    } else {
        if (trackerDetected()) goTo(STATE_STALK);
        else                   goTo(STATE_SEARCH);
    }
}

static void doStalk(float dist) {
    if (anyEdge()) { triggerEdgeAvoid(); return; }
    if (dist <= ATTACK_DISTANCE_CM) { goTo(STATE_ATTACK); return; }
    if (trackerLost())              { goTo(STATE_SEARCH);  return; }

    if (getTactic() == TACTIC_FLANKER) {
        _orbitStart = millis();
        goTo(STATE_ORBIT);
        return;
    }

    float bearing = trackerBearing();
    if (abs(bearing) < 15.0f) {
        motorForward(SPEED_STALK);
    } else if (bearing > 0) {
        motorArcRight(SPEED_STALK, SPEED_STALK - SPEED_CORRECT);
    } else {
        motorArcLeft(SPEED_STALK, SPEED_STALK - SPEED_CORRECT);
    }
}

static void doOrbit(float dist) {
    if (anyEdge()) { triggerEdgeAvoid(); return; }
    if (dist <= ATTACK_DISTANCE_CM)                    { goTo(STATE_ATTACK); return; }
    if (millis() - _orbitStart > ORBIT_DURATION_MS)   { goTo(STATE_STALK);  return; }
    motorArcRight(SPEED_STALK, SPEED_STALK - SPEED_CORRECT);
}

static void doSearch(float dist) {
    if (anyEdge())                    { triggerEdgeAvoid(); return; }
    if (dist <= ATTACK_DISTANCE_CM)   { goTo(STATE_ATTACK); return; }
    if (dist <= STALK_DISTANCE_CM)    { goTo(STATE_STALK);  return; }

    // Advance scan step timer
    if (millis() - _scanStepStart >= SCAN_STEP_MS) {
        _scanCovered   += (float)SPEED_SEARCH / 255.0f * 360.0f * SCAN_STEP_MS / 600.0f;
        _scanStepStart  = millis();
        if (_scanCovered >= 360.0f) {
            _scanCovered = 0.0f;
            _scanDir    *= -1;   // reverse direction after full rotation
        }
    }

    _lastSpinDir = _scanDir;
    if (_scanDir > 0) motorSpinRight(SPEED_SEARCH);
    else              motorSpinLeft(SPEED_SEARCH);
}

// ── Public ────────────────────────────────────────────────────────────────────
void strategyInit() {
    _state         = STATE_START;
    _stateStart    = millis();
    _matchStart    = millis();
    _scanStepStart = millis();
    _tactic        = TACTIC_RUSHER;
    _edgeHits      = 0;
    _pushSuccesses = 0;
    _scanDir       = 1;
    _scanCovered   = 0.0f;
    _lastSpinDir   = 0;
}

void strategyTick(float dist_cm) {
    // Update opponent tracker with latest distance and spin estimate
    float spinDelta = _lastSpinDir * SPIN_DEG_PER_TICK;
    trackerUpdate(dist_cm, spinDelta);
    _lastSpinDir = 0;

    switch (_state) {
        case STATE_START:      doStart();          break;
        case STATE_EDGE_AVOID: doEdgeAvoid();       break;
        case STATE_RAM:        doRam(dist_cm);      break;
        case STATE_ATTACK:     doAttack(dist_cm);   break;
        case STATE_STALK:      doStalk(dist_cm);    break;
        case STATE_ORBIT:      doOrbit(dist_cm);    break;
        case STATE_SEARCH:     doSearch(dist_cm);   break;
    }
}

State getCurrentState() { return _state; }
