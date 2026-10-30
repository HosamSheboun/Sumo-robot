// config.h — All pin assignments and constants for the Arduino Nano sumo robot.
//
// Robot class : Heavyweight sumo (≤ 3 kg, ≤ 20 × 20 cm)
// Controller  : Arduino Nano (ATmega328P)
// Motor driver: L298N dual H-bridge
// Motors      : 2× GA25-370 12V 130RPM DC gear motor with quadrature encoder
// Sensors     : HC-SR04 ultrasonic (front), 2× downward IR line sensors (front/back)
// Power       : 3S 18650 (~11.1V) → L298N 12V pin + Nano VIN pin directly
//               Nano onboard 5V regulator powers sensors and encoders
//
// Arduino Nano interrupt-capable pins: D2, D3 only (used for encoders)
// PWM-capable pins: D3, D5, D6, D9, D10, D11 (ENA/ENB must be on these)
// Analog pins A0–A5 used as digital I/O for non-PWM signals

#pragma once

// ── L298N — Left motor ────────────────────────────────────────────────────────
#define LEFT_IN1   7    // D7  → L298N IN1  direction bit A
#define LEFT_IN2   8    // D8  → L298N IN2  direction bit B
#define LEFT_ENA   9    // D9  → L298N ENA  PWM speed  (PWM capable)

// ── L298N — Right motor ───────────────────────────────────────────────────────
#define RIGHT_IN3  4    // D4  → L298N IN3  direction bit A
#define RIGHT_IN4  5    // D5  → L298N IN4  direction bit B (D5 is PWM but used as dir)
#define RIGHT_ENB  6    // D6  → L298N ENB  PWM speed  (PWM capable)

// ── GA25-370 Quadrature Encoders ──────────────────────────────────────────────
// MUST be on interrupt pins D2 and D3 for reliable pulse counting.
// Channel A triggers the interrupt; Channel B is read inside the ISR for direction.
#define LEFT_ENC_A   2  // D2  → Left  motor yellow wire  Channel A (INT0)
#define LEFT_ENC_B   A0 // A0  → Left  motor white  wire  Channel B (digital read)
#define RIGHT_ENC_A  3  // D3  → Right motor yellow wire  Channel A (INT1)
#define RIGHT_ENC_B  A1 // A1  → Right motor white  wire  Channel B (digital read)

// ── HC-SR04 Ultrasonic Sensor ─────────────────────────────────────────────────
// Nano runs at 5V — HC-SR04 ECHO outputs 5V — safe to connect directly (no divider needed)
#define TRIG_PIN    A2  // A2  → HC-SR04 TRIG
#define ECHO_PIN    A3  // A3  → HC-SR04 ECHO

// ── Downward IR Line / Edge Sensors ──────────────────────────────────────────
// Active-LOW: reads LOW when white boundary line detected.
#define IR_FRONT_PIN  A4  // A4  → Front downward IR sensor OUT
#define IR_BACK_PIN   A5  // A5  → Back  downward IR sensor OUT

// ── Rocker Switch ─────────────────────────────────────────────────────────────
// One pin → D10, other pin → GND. Internal pull-up enabled.
// Switch ON  → reads LOW  (closed to GND)
// Switch OFF → reads HIGH (pull-up holds HIGH)
#define SWITCH_PIN   10  // D10 → Rocker switch

// ── Status LED ────────────────────────────────────────────────────────────────
// LED + 330Ω resistor in series between D11 and GND.
// LED ON  = robot armed and running
// LED BLINK = start delay counting down
// LED OFF = switch off or waiting
#define LED_PIN      11  // D11 → LED anode via 330Ω resistor

// ── PWM frequency ─────────────────────────────────────────────────────────────
// Arduino analogWrite uses ~490Hz on D9/D10 and ~980Hz on D5/D6.
// We use D9 and D6 for ENA/ENB — both are hardware PWM.

// ── Speed constants (analogWrite 0–255) ──────────────────────────────────────
#define SPEED_FULL     255  // Maximum attack speed
#define SPEED_SEARCH   140  // Rotation speed while scanning
#define SPEED_REVERSE  217  // Reverse burst during edge avoidance (~85%)
#define SPEED_TURN     204  // Spin speed during edge avoidance (~80%)
#define SPEED_STALK    178  // Approach speed when tracking (~70%)
#define SPEED_CORRECT  102  // Speed reduction for heading correction (~40%)

// ── Distance thresholds (cm) ──────────────────────────────────────────────────
#define ATTACK_DISTANCE_CM   80.0
#define STALK_DISTANCE_CM   150.0
#define CLOSE_DISTANCE_CM    25.0

// ── Timing (milliseconds) ─────────────────────────────────────────────────────
#define START_DELAY_MS       5000  // Mandatory pre-match pause
#define REVERSE_DURATION_MS   300  // Reverse burst on edge detection
#define SPIN_DURATION_MS      450  // Spin-away after reverse
#define SCAN_STEP_MS          550  // Time per scan step before direction check
#define LOST_TIMEOUT_MS      1200  // Ms without detection before SEARCH
#define ORBIT_DURATION_MS     250  // Arc duration during ORBIT tactic
#define RAM_BURST_MS          180  // Close-range burst push duration
#define LOOP_MS                20  // Main loop period (50Hz = 20ms)

// ── Encoder ───────────────────────────────────────────────────────────────────
#define ENCODER_PPR  204  // Pulses per revolution (GA25-370: 6PPR × gear ratio 34)
