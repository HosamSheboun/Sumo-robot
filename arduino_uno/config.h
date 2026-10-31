// =============================================================================
// config.h — Arduino Uno sumo robot — all pins and tunable constants.
//
// Robot class  : Heavyweight sumo (≤ 3 kg, ≤ 20 × 20 cm)
// Controller   : Arduino Uno (ATmega328P)
// Motor driver : L298N dual H-bridge
// Motors       : 2× GA25-370 12V 130RPM DC gear motor
// Sensors      : HC-SR04 ultrasonic (front) + 2× IR line sensors (front/back)
// Power        : 3S 18650 (~11.1V) → L298N 12V terminal → motors
//                L298N onboard 5V regulator pin → Uno 5V pin → all logic
//
// POWER NOTE: The L298N has a built-in 5V regulator that outputs on its
// "5V" screw terminal when the 12V input is connected. Wire that directly
// to the Uno 5V pin (NOT VIN). This powers the Uno, sensors, and encoders
// from one clean source. No MT3608 boost converter needed.
//
// ⚠️  Only use the L298N 5V output when motor supply is 7–35V.
//     It can supply ~500mA — enough for Uno + HC-SR04 + 2× IR + encoders.
// =============================================================================

#pragma once

// ── L298N — Left motor ────────────────────────────────────────────────────────
// ENA jumper ON — full power, no PWM speed control needed.
#define LEFT_IN1    7   // D7  → L298N IN1  (left motor direction bit A)
#define LEFT_IN2    8   // D8  → L298N IN2  (left motor direction bit B)

// ── L298N — Right motor ───────────────────────────────────────────────────────
// ENB jumper ON — full power, no PWM speed control needed.
#define RIGHT_IN3   4   // D4  → L298N IN3  (right motor direction bit A)
#define RIGHT_IN4   5   // D5  → L298N IN4  (right motor direction bit B)

// ── HC-SR04 Ultrasonic Sensor (front-facing) ──────────────────────────────────
// 5V logic — connect directly, no resistor divider needed.
#define TRIG_PIN   11   // D11 → HC-SR04 TRIG
#define ECHO_PIN   12   // D12 → HC-SR04 ECHO

// ── IR Line / Edge Sensors (downward-facing) ──────────────────────────────────
// Active-LOW: pin reads LOW when white boundary line is detected.
// Both sensors face the floor directly below the robot.
#define IR_FRONT   A0   // A0  → Front IR sensor OUT
#define IR_BACK    A1   // A1  → Back  IR sensor OUT

// ── Rocker Switch ─────────────────────────────────────────────────────────────
#define SWITCH_PIN  2   // D2  → rocker switch (other pin → GND)

// ── Status LED ────────────────────────────────────────────────────────────────
// D13 also drives the Uno built-in LED.
#define LED_PIN    13   // D13 → LED anode via 330Ω → GND

// ── GA25-370 Encoder wiring (signals unused in this build) ────────────────────
// Green     → L298N 5V output  (encoder VCC — MUST be connected)
// Dark Blue → GND              (encoder GND — MUST be connected)
// Yellow    → leave unconnected
// White     → leave unconnected

// =============================================================================
// SENSOR THRESHOLDS
// =============================================================================
#define ATTACK_DIST_CM     60   // charge when opponent closer than this (cm)
#define NO_DETECT_CM      200   // treat as nothing seen above this (cm)
#define CHARGE_SPEED_CM_S  55   // closing speed (cm/s) = opponent is charging

// =============================================================================
// TIMING CONSTANTS (milliseconds)
// =============================================================================
#define START_DELAY_MS    5000UL  // mandatory pre-match pause (competition = 5000)
#define SPIN_STEP_MS       400UL  // spin duration per search step

// Edge avoidance
#define EDGE_REVERSE_MS    300UL  // reverse away from boundary
#define EDGE_SPIN_MS       400UL  // spin away after reversing

// Matador
#define MATADOR_SIDESTEP_MS  350UL
#define MATADOR_REFACE_MS    260UL
#define MATADOR_SLAM_MS      500UL
#define MATADOR_BAIT_MS      180UL
#define MATADOR_MAX_DODGES     4

// Bulldozer wander
#define WANDER_FWD1_MS     350UL
#define WANDER_SPIN1_MS    220UL
#define WANDER_FWD2_MS     280UL
#define WANDER_SPIN2_MS    180UL
#define WANDER_FWD3_MS     400UL
#define WANDER_SPIN3_MS    300UL

// Flanker
#define FLANKER_ARC_MS     550UL
#define FLANKER_SLAM_MS    480UL

// Berserker
#define BERSERKER_RETREAT_MS  180UL
#define BERSERKER_CHARGE_MS   600UL
#define BERSERKER_CREEP_MS    150UL

// Phase boundaries (ms from match start)
#define PHASE1_END_MS   30000UL   // 0:00–0:30  MATADOR
#define PHASE2_END_MS   60000UL   // 0:30–1:00  BULLDOZER
#define PHASE3_END_MS   90000UL   // 1:00–1:30  FLANKER
                                  // 1:30–2:00  BERSERKER
