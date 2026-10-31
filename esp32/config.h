// =============================================================================
// config.h — ESP32 sumo robot — all pins and tunable constants.
//
// Robot class  : Heavyweight sumo (≤ 3 kg, ≤ 20 × 20 cm)
// Controller   : ESP32 (dual-core 240MHz, 3.3V logic)
// Motor driver : L298N dual H-bridge
// Motors       : 2× GA25-370 12V 130RPM DC gear motor
// Sensors      : HC-SR04 ultrasonic (front) + 2× IR line sensors (front/back)
// Power        : 3S 18650 (~11.1V) → L298N 12V terminal → motors
//                L298N 5V output → 3.3V LDO on ESP32 devboard → ESP32 logic
//
// POWER NOTE: The L298N 5V output feeds the ESP32 devboard VIN pin (5V).
// The devboard has its own 3.3V LDO regulator. All ESP32 GPIO is 3.3V logic.
//
// ⚠️  HC-SR04 ECHO outputs 5V. Use a 1kΩ + 2kΩ resistor divider to bring
//     it down to 3.3V before connecting to ESP32 GPIO. TRIG is fine direct
//     because 3.3V HIGH is enough to trigger the HC-SR04.
//
// ⚠️  IR sensor OUT pins are typically 3.3V compatible — check your module.
//     If the OUT pin swings to 5V, use a resistor divider on that pin too.
//
// ESP32 ADVANTAGES over Uno/Nano:
//   - Dual-core 240MHz — strategy runs on core 1, sensors on core 0
//   - Hardware PWM (LEDC) — smooth speed control without jitter
//   - WiFi — OTA firmware updates without USB cable
//   - Bluetooth — can receive commands or send telemetry
//   - 12-bit ADC — more precise analog readings
//   - Hardware timers — microsecond-accurate ultrasonic timing
// =============================================================================

#pragma once

// ── L298N — Left motor (PWM speed control via LEDC) ──────────────────────────
#define LEFT_IN1   27   // GPIO27 → L298N IN1
#define LEFT_IN2   26   // GPIO26 → L298N IN2
#define LEFT_ENA   14   // GPIO14 → L298N ENA  (PWM — LEDC channel 0)

// ── L298N — Right motor ───────────────────────────────────────────────────────
#define RIGHT_IN3  25   // GPIO25 → L298N IN3
#define RIGHT_IN4  33   // GPIO33 → L298N IN4
#define RIGHT_ENB  32   // GPIO32 → L298N ENB  (PWM — LEDC channel 1)

// ── HC-SR04 Ultrasonic Sensor ─────────────────────────────────────────────────
// TRIG: 3.3V output — connect directly.
// ECHO: 5V output — use 1kΩ + 2kΩ resistor divider → 3.3V.
#define TRIG_PIN   18   // GPIO18 → HC-SR04 TRIG
#define ECHO_PIN   19   // GPIO19 → HC-SR04 ECHO (after divider)

// ── IR Line / Edge Sensors ────────────────────────────────────────────────────
// Active-LOW: reads LOW when white boundary line detected.
#define IR_FRONT    4   // GPIO4  → Front IR sensor OUT
#define IR_BACK    16   // GPIO16 → Back  IR sensor OUT

// ── Rocker Switch ─────────────────────────────────────────────────────────────
#define SWITCH_PIN 34   // GPIO34 → rocker switch (input-only pin, other pin → GND)

// ── Status LED ────────────────────────────────────────────────────────────────
#define LED_PIN     2   // GPIO2  → LED anode via 330Ω → GND (also ESP32 built-in LED)

// ── GA25-370 Encoder wiring ───────────────────────────────────────────────────
// Green     → L298N 5V output  (MUST be connected)
// Dark Blue → GND              (MUST be connected)
// Yellow    → leave unconnected (or connect to GPIO for future encoder use)
// White     → leave unconnected

// ── PWM (LEDC) ────────────────────────────────────────────────────────────────
#define PWM_FREQ       20000  // 20kHz — above hearing range, smooth motor control
#define PWM_RESOLUTION     8  // 8-bit = 0–255 duty cycle
#define PWM_CH_LEFT        0  // LEDC channel for left motor
#define PWM_CH_RIGHT       1  // LEDC channel for right motor

// ── Speed (0–255) ─────────────────────────────────────────────────────────────
#define SPEED_FULL    255   // maximum attack speed
#define SPEED_SEARCH  160   // spin speed during search
#define SPEED_STALK   180   // approach speed when stalking
#define SPEED_ARC_OUT 220   // outer wheel speed during arc
#define SPEED_ARC_IN   80   // inner wheel speed during arc

// =============================================================================
// SENSOR THRESHOLDS
// =============================================================================
#define ATTACK_DIST_CM     60
#define RAM_DIST_CM        20   // switch to RAM burst when this close
#define NO_DETECT_CM      200
#define CHARGE_SPEED_CM_S  55

// =============================================================================
// TIMING CONSTANTS (milliseconds)
// =============================================================================
#define START_DELAY_MS    5000UL
#define SPIN_STEP_MS       300UL   // shorter on ESP32 — faster loop

#define EDGE_REVERSE_MS    280UL
#define EDGE_SPIN_MS       380UL

#define MATADOR_SIDESTEP_MS  320UL
#define MATADOR_REFACE_MS    240UL
#define MATADOR_SLAM_MS      480UL
#define MATADOR_BAIT_MS      150UL
#define MATADOR_MAX_DODGES     4

#define WANDER_FWD1_MS     300UL
#define WANDER_SPIN1_MS    200UL
#define WANDER_FWD2_MS     250UL
#define WANDER_SPIN2_MS    160UL
#define WANDER_FWD3_MS     350UL
#define WANDER_SPIN3_MS    260UL

#define FLANKER_ARC_MS     500UL
#define FLANKER_SLAM_MS    450UL

#define BERSERKER_RETREAT_MS  160UL
#define BERSERKER_CHARGE_MS   550UL
#define BERSERKER_CREEP_MS    120UL

#define RAM_BURST_MS       200UL   // full-speed burst when very close

#define PHASE1_END_MS   30000UL
#define PHASE2_END_MS   60000UL
#define PHASE3_END_MS   90000UL
