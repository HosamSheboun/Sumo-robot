// strategy.h — Adaptive FSM state machine declarations.

#pragma once
#include <Arduino.h>
#include "config.h"

// ── FSM States ────────────────────────────────────────────────────────────────
enum State {
    STATE_START,       // Mandatory 5-second pre-match delay
    STATE_SEARCH,      // Spin-scan to locate opponent
    STATE_STALK,       // Opponent visible but far — arc toward it
    STATE_ORBIT,       // Flanking arc (FLANKER tactic only)
    STATE_ATTACK,      // Full-speed charge with heading correction
    STATE_RAM,         // Close-range burst push
    STATE_EDGE_AVOID   // White line detected — reverse and spin away
};

// ── Tactic styles ─────────────────────────────────────────────────────────────
enum Tactic {
    TACTIC_RUSHER,   // Charge immediately at full speed
    TACTIC_STALKER,  // Approach carefully using arcs
    TACTIC_FLANKER   // Orbit wide, attack from the side
};

void strategyInit();
void strategyTick(float dist_cm);   // Call every LOOP_MS from main loop
State getCurrentState();
