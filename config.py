"""
config.py — Centralized hardware configuration for the Pi Zero 2 W Sumo Robot.

All BCM GPIO pin numbers, PWM settings, speed constants, and timing thresholds
are defined here. Edit ONLY this file when adapting to different wiring layouts.

BCM pin numbering is used throughout (pass gpio.setmode(gpio.BCM) in every module).
"""

# ─────────────────────────────────────────────
# L298N Motor Driver — Left Motor (Motor A)
# ─────────────────────────────────────────────
# IN1 / IN2 control direction; ENA is the PWM speed pin.
LEFT_IN1  = 17   # BCM 17 → L298N IN1
LEFT_IN2  = 27   # BCM 27 → L298N IN2
LEFT_ENA  = 18   # BCM 18 → L298N ENA  (hardware PWM capable on Pi Zero 2 W)

# ─────────────────────────────────────────────
# L298N Motor Driver — Right Motor (Motor B)
# ─────────────────────────────────────────────
RIGHT_IN3 = 22   # BCM 22 → L298N IN3
RIGHT_IN4 = 23   # BCM 23 → L298N IN4
RIGHT_ENB = 13   # BCM 13 → L298N ENB  (hardware PWM capable on Pi Zero 2 W)

# ─────────────────────────────────────────────
# GA25-370 Encoder Inputs (optional feedback)
# Each motor has two Hall-effect encoder channels (A & B).
# Only channel A is used here for basic speed sensing.
# ─────────────────────────────────────────────
LEFT_ENC_A  = 5   # BCM 5  → Left motor encoder channel A
RIGHT_ENC_A = 6   # BCM 6  → Right motor encoder channel A

# ─────────────────────────────────────────────
# HC-SR04 Ultrasonic Sensor
# TRIG: 3.3 V tolerant — connect directly.
# ECHO: outputs 5 V — use a 1kΩ/2kΩ resistor voltage divider to bring to ~3.3 V.
# ─────────────────────────────────────────────
TRIG_PIN = 24    # BCM 24 → HC-SR04 TRIG
ECHO_PIN = 25    # BCM 25 → HC-SR04 ECHO (after voltage divider)

# ─────────────────────────────────────────────
# IR Obstacle Sensor (front-facing, digital OUT)
# Active-LOW: GPIO reads LOW when obstacle detected.
# ─────────────────────────────────────────────
IR_FRONT_PIN = 12  # BCM 12 → IR sensor digital OUT

# ─────────────────────────────────────────────
# Downward-Facing IR Line / Edge Sensors
# Active-LOW: GPIO reads LOW when white boundary line detected.
# Mount one sensor at the front-left and one at the front-right of the chassis.
# ─────────────────────────────────────────────
EDGE_LEFT_PIN  = 19  # BCM 19 → Left edge IR sensor
EDGE_RIGHT_PIN = 26  # BCM 26 → Right edge IR sensor

# ─────────────────────────────────────────────
# PWM Configuration
# ─────────────────────────────────────────────
PWM_FREQ = 1000   # Hz — 1 kHz is a good balance for L298N and DC motors

# ─────────────────────────────────────────────
# Speed Constants  (duty cycle, 0–100 %)
# ─────────────────────────────────────────────
SPEED_FULL    = 100   # Maximum attack speed
SPEED_SEARCH  = 60    # Rotation speed during opponent search
SPEED_REVERSE = 80    # Reverse speed during edge-avoidance maneuver
SPEED_TURN    = 75    # Turn speed during edge-avoidance spin

# ─────────────────────────────────────────────
# Timing & Distance Thresholds
# ─────────────────────────────────────────────
START_DELAY        = 5.0   # Mandatory pre-match pause (seconds) per sumo rules
REVERSE_DURATION   = 0.35  # How long to reverse when edge is detected (seconds)
SPIN_DURATION      = 0.40  # How long to spin away from edge (seconds)
ULTRASONIC_TIMEOUT = 0.025 # Max echo wait time → ~4.25 m max range (seconds)
ATTACK_DISTANCE_CM = 60.0  # Charge at opponent when closer than this (cm)
LOOP_HZ            = 50    # Main FSM loop frequency (iterations per second)
