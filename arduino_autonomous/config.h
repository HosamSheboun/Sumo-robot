// =============================================================================
// config.h — All pin assignments and tunable constants for the Arduino Uno
//            sumo robot. This is the ONLY file you need to edit to tune
//            the robot's behaviour. Never hardcode numbers in other files.
//
// Robot class  : Heavyweight sumo (≤ 3 kg, ≤ 20 × 20 cm)
// Controller   : Arduino Uno (ATmega328P)
// Motor driver : L298N dual H-bridge (ENA + ENB jumpers ON — no PWM)
// Motors       : 2× GA25-370 12V 130 RPM DC gear motor with encoder
// Sensor       : HC-SR04 ultrasonic (front-facing)
// Power        : 3S 18650 (~11.1V) → L298N 12V + Uno VIN
//                Uno onboard 5V regulator → sensor VCC + encoder VCC
// =============================================================================

#pragma once

// ── L298N — Left motor ────────────────────────────────────────────────────────
// ENA jumper must be ON — motors run at full power, no PWM speed control.
#define LEFT_IN1    7   // D7  → L298N IN1  (left motor direction bit A)
#define LEFT_IN2    8   // D8  → L298N IN2  (left motor direction bit B)

// ── L298N — Right motor ───────────────────────────────────────────────────────
// ENB jumper must be ON — motors run at full power, no PWM speed control.
#define RIGHT_IN3   4   // D4  → L298N IN3  (right motor direction bit A)
#define RIGHT_IN4   5   // D5  → L298N IN4  (right motor direction bit B)

// ── HC-SR04 Ultrasonic Sensor ─────────────────────────────────────────────────
// Both pins are 5V logic — connect directly, no resistor divider needed.
#define TRIG_PIN   11   // D11 → HC-SR04 TRIG
#define ECHO_PIN   12   // D12 → HC-SR04 ECHO

// ── Rocker Switch ─────────────────────────────────────────────────────────────
// One pin → D10, other pin → GND. Internal pull-up enabled in code.
// Switch ON  (closed) → pin reads LOW  → robot runs
// Switch OFF (open)   → pin reads HIGH → robot stops
#define SWITCH_PIN 10   // D10 → rocker switch (other pin → GND)

// ── Status LED ────────────────────────────────────────────────────────────────
// D13 also drives the Uno built-in LED — useful for testing without wiring.
// LED anode → 330Ω resistor → D13 → LED cathode → GND
// SLOW BLINK (200ms/800ms) = waiting for switch ON
// FAST BLINK (100ms/100ms) = start delay counting down
// SOLID ON               = robot active
#define LED_PIN    13   // D13 → LED anode via 330Ω resistor

// ── GA25-370 Encoder wiring reminder (signals not used in this build) ─────────
// Green     → Uno 5V   (encoder VCC — MUST be connected or board floats)
// Dark Blue → GND      (encoder GND — MUST be connected)
// Yellow    → leave unconnected  (channel A — unused)
// White     → leave unconnected  (channel B — unused)

// =============================================================================
// SENSOR THRESHOLDS — adjust these to match your ring size and opponent
// =============================================================================

// Distance at which the robot switches from approach to attack (cm)
#define ATTACK_DIST_CM      80

// Distance above which the sensor is treated as "nothing detected" (cm)
// HC-SR04 max reliable range is ~400cm but we cap at 200 for the ring size
#define NO_DETECT_CM       200

// Closing speed (cm/s) above which we treat the opponent as charging at us
// Lower = robot dodges more easily | Higher = only dodges a real fast charge
#define CHARGE_SPEED_CM_S   55

// =============================================================================
// TIMING CONSTANTS — all delays in milliseconds, named so nothing is magic
// =============================================================================

// Mandatory pause after switch ON before robot moves (competition rule: 5000)
// Set to 1000 for practice, 5000 for competition
#define START_DELAY_MS      1000UL

// How long the robot spins during each search step before re-reading sensor
#define SPIN_STEP_MS         500UL

// ── Matador technique timings ─────────────────────────────────────────────────
#define MATADOR_SIDESTEP_MS  380UL  // how long to arc sideways during dodge
#define MATADOR_REFACE_MS    280UL  // how long to spin to face opponent after dodge
#define MATADOR_SLAM_MS      550UL  // how long to charge after the dodge
#define MATADOR_BAIT_MS      200UL  // how long to creep forward to bait a charge
#define MATADOR_MAX_DODGES     4    // max dodges before switching to direct attack

// ── Bulldozer technique timings ───────────────────────────────────────────────
#define WANDER_FWD1_MS       350UL  // wander step 0: forward burst
#define WANDER_SPIN1_MS      220UL  // wander step 1: spin right
#define WANDER_FWD2_MS       280UL  // wander step 2: forward burst
#define WANDER_SPIN2_MS      180UL  // wander step 3: spin left
#define WANDER_FWD3_MS       400UL  // wander step 4: forward burst
#define WANDER_SPIN3_MS      300UL  // wander step 5: spin right

// ── Flanker technique timings ─────────────────────────────────────────────────
#define FLANKER_ARC_MS       600UL  // how long to arc wide around opponent
#define FLANKER_SLAM_MS      500UL  // how long to charge into opponent's side

// ── Berserker technique timings ───────────────────────────────────────────────
#define BERSERKER_RETREAT_MS 180UL  // fake retreat spin duration
#define BERSERKER_CHARGE_MS  600UL  // charge duration after fake retreat
#define BERSERKER_CREEP_MS   150UL  // creep forward when opponent is far

// ── Phase boundary times (milliseconds from match start) ─────────────────────
#define PHASE1_END_MS      30000UL  // 0:00 – 0:30  MATADOR
#define PHASE2_END_MS      60000UL  // 0:30 – 1:00  BULLDOZER
#define PHASE3_END_MS      90000UL  // 1:00 – 1:30  FLANKER
                                    // 1:30 – 2:00  BERSERKER (remainder)
