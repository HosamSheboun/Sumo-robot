// config.h — Pin assignments and constants for the Arduino Uno sumo robot.
//
// Robot class : Heavyweight sumo (≤ 3 kg, ≤ 20 × 20 cm)
// Controller  : Arduino Uno (ATmega328P)
// Motor driver: L298N dual H-bridge
// Motors      : 2× GA25-370 12V 130RPM DC gear motor
// Power       : 3S 18650 (~11.1V) → L298N 12V pin + Uno VIN pin directly
//               Uno onboard 5V regulator → encoder VCC (green wires)
//
// PWM-capable pins used for ENA/ENB: D9 and D3

#pragma once

// ── L298N — Left motor ────────────────────────────────────────────────────────
#define LEFT_IN1   7    // D7  → L298N IN1  direction bit A
#define LEFT_IN2   8    // D8  → L298N IN2  direction bit B
#define LEFT_ENA   9    // D9  → L298N ENA  PWM speed  (~)

// ── L298N — Right motor ───────────────────────────────────────────────────────
#define RIGHT_IN3  4    // D4  → L298N IN3  direction bit A
#define RIGHT_IN4  5    // D5  → L298N IN4  direction bit B
#define RIGHT_ENB  3    // D3  → L298N ENB  PWM speed  (~)

// ── HC-SR04 Ultrasonic Sensor ───────────────────────────────────────────────────
// TRIG → D12, ECHO → D13 (both 5V logic — connect directly, no resistor divider)
#define TRIG_PIN    11  // D11 → HC-SR04 TRIG
#define ECHO_PIN    12  // D12 → HC-SR04 ECHO

#define ATTACK_DIST_CM   80   // charge forward when opponent closer than this
#define NO_DETECT_CM    200   // treat as "nothing seen" above this distance

// ── GA25-370 Encoders (wired but not used in this build) ─────────────────────
// Kept here for reference. Encoder wires must still be connected to power/GND
// so the encoder board does not float. Yellow and white wires can be left
// unconnected or tied to any unused digital pin.
// Green  → Nano 5V   (encoder VCC — must be connected)
// Dark Blue → GND    (encoder GND — must be connected)
// Yellow → not used
// White  → not used

// ── Rocker Switch ─────────────────────────────────────────────────────────────
// One pin → D10, other pin → GND. Internal pull-up enabled in code.
// Switch ON  → reads LOW  (closed to GND)
// Switch OFF → reads HIGH (pull-up holds HIGH)
#define SWITCH_PIN  10  // D10 → Rocker switch

// ── Status LED ────────────────────────────────────────────────────────────────
// LED anode → 330Ω resistor → D13
// LED cathode → GND
// SLOW BLINK = waiting for switch to be turned ON
// FAST BLINK = 5-second start delay counting down
// SOLID ON   = robot searching / attacking
#define LED_PIN     13  // D13 → LED anode via 330Ω resistor (built-in LED also lights up)

// ── Speed (analogWrite 0–255) ─────────────────────────────────────────────────
#define SPEED_FULL    255  // Full speed forward / attack
#define SPEED_SPIN    180  // Speed while spinning to search

// ── Timing ────────────────────────────────────────────────────────────────────
#define START_DELAY_MS  5000UL  // 5-second mandatory pre-match pause
#define SPIN_STEP_MS     500UL  // spin this long before re-checking sensor
