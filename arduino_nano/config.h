// =============================================================================
// config.h — Arduino Nano sumo robot — all pins and tunable constants.
//
// Robot class  : Heavyweight sumo (≤ 3 kg, ≤ 20 × 20 cm)
// Controller   : Arduino Nano (ATmega328P)
// Motor driver : L298N dual H-bridge
// Motors       : 2× GA25-370 12V 130RPM DC gear motor
// Sensors      : HC-SR04 ultrasonic (front) + 2× IR line sensors (front/back)
// Power        : 3S 18650 (~11.1V) → L298N 12V terminal → motors
//                L298N 5V output pin → Nano 5V pin → all logic + sensors
//
// POWER NOTE: Wire L298N "5V" screw terminal → Nano 5V pin (NOT VIN).
// The L298N regulator handles 7–35V input and outputs clean 5V at ~500mA.
// This is enough for Nano + HC-SR04 + 2× IR sensors + encoder boards.
// No MT3608 boost converter needed.
//
// Nano vs Uno differences:
//   - Nano uses Mini-USB for programming
//   - Nano has A6 and A7 as analog-input-only (no digital use)
//   - Nano physical size is smaller — fits tighter chassis layouts
//   - Pin numbers are identical to Uno for D0–D13 and A0–A5
// =============================================================================

#pragma once

// ── L298N — Left motor ────────────────────────────────────────────────────────
#define LEFT_IN1    7   // D7  → L298N IN1
#define LEFT_IN2    8   // D8  → L298N IN2
// ENA jumper ON — full power always

// ── L298N — Right motor ───────────────────────────────────────────────────────
#define RIGHT_IN3   4   // D4  → L298N IN3
#define RIGHT_IN4   5   // D5  → L298N IN4
// ENB jumper ON — full power always

// ── HC-SR04 Ultrasonic Sensor ─────────────────────────────────────────────────
#define TRIG_PIN    9   // D9  → HC-SR04 TRIG
#define ECHO_PIN   10   // D10 → HC-SR04 ECHO

// ── IR Line / Edge Sensors ────────────────────────────────────────────────────
// Active-LOW: reads LOW when white boundary line detected.
#define IR_FRONT   A0   // A0  → Front IR sensor OUT
#define IR_BACK    A1   // A1  → Back  IR sensor OUT

// ── Rocker Switch ─────────────────────────────────────────────────────────────
#define SWITCH_PIN  2   // D2  → rocker switch (other pin → GND)

// ── Status LED ────────────────────────────────────────────────────────────────
#define LED_PIN    13   // D13 → LED anode via 330Ω → GND (also Nano built-in LED)

// ── GA25-370 Encoder wiring (signals unused) ──────────────────────────────────
// Green     → L298N 5V output  (MUST be connected)
// Dark Blue → GND              (MUST be connected)
// Yellow, White → leave unconnected

// =============================================================================
// SENSOR THRESHOLDS
// =============================================================================
#define ATTACK_DIST_CM     60
#define NO_DETECT_CM      200
#define CHARGE_SPEED_CM_S  55

// =============================================================================
// TIMING CONSTANTS (milliseconds)
// =============================================================================
#define START_DELAY_MS    5000UL
#define SPIN_STEP_MS       400UL

#define EDGE_REVERSE_MS    300UL
#define EDGE_SPIN_MS       400UL

#define MATADOR_SIDESTEP_MS  350UL
#define MATADOR_REFACE_MS    260UL
#define MATADOR_SLAM_MS      500UL
#define MATADOR_BAIT_MS      180UL
#define MATADOR_MAX_DODGES     4

#define WANDER_FWD1_MS     350UL
#define WANDER_SPIN1_MS    220UL
#define WANDER_FWD2_MS     280UL
#define WANDER_SPIN2_MS    180UL
#define WANDER_FWD3_MS     400UL
#define WANDER_SPIN3_MS    300UL

#define FLANKER_ARC_MS     550UL
#define FLANKER_SLAM_MS    480UL

#define BERSERKER_RETREAT_MS  180UL
#define BERSERKER_CHARGE_MS   600UL
#define BERSERKER_CREEP_MS    150UL

#define PHASE1_END_MS   30000UL
#define PHASE2_END_MS   60000UL
#define PHASE3_END_MS   90000UL
