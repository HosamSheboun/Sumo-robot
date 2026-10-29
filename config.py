"""
config.py — Single source of truth for all hardware constants.

Robot class : Heavyweight sumo (≤ 3 kg, ≤ 20 × 20 cm)
Controller  : Raspberry Pi Zero 2 W  (BCM pin numbering throughout)
Motor driver: L298N dual H-bridge
Motors      : 2× GA25-370 12 V 130 RPM DC gear motor with quadrature encoder
Sensors     : HC-SR04 ultrasonic (front), 2× downward IR line sensors (front/back)
Power       : 3S 18650 (11.1 V) → L298N/motors direct
              Single cell tap (3.7 V) → MT3608 boost → 5.0 V → Pi + sensors + encoders

GA25-370 encoder wire colours
  Red       → Motor terminal +  (to L298N OUTx)
  Black     → Motor terminal −  (to L298N OUTx)
  Green     → Encoder VCC  5 V  (from MT3608 5 V rail)
  Dark Blue → Encoder GND       (common GND rail)
  Yellow    → Encoder Channel A (GPIO input)
  White     → Encoder Channel B (GPIO input)
"""

# ── L298N — Left motor (Motor A) ─────────────────────────────────────────────
LEFT_IN1 = 17       # BCM 17 → L298N IN1  direction bit A
LEFT_IN2 = 27       # BCM 27 → L298N IN2  direction bit B
LEFT_ENA = 18       # BCM 18 → L298N ENA  PWM speed  (HW-PWM channel 0)

# ── L298N — Right motor (Motor B) ────────────────────────────────────────────
RIGHT_IN3 = 22      # BCM 22 → L298N IN3  direction bit A
RIGHT_IN4 = 23      # BCM 23 → L298N IN4  direction bit B
RIGHT_ENB = 13      # BCM 13 → L298N ENB  PWM speed  (HW-PWM channel 1)

# ── GA25-370 Quadrature Encoders ─────────────────────────────────────────────
# Both channels (A + B) are read for full quadrature direction + speed sensing.
LEFT_ENC_A  = 5     # BCM  5 → Left  motor yellow wire  (Channel A)
LEFT_ENC_B  = 6     # BCM  6 → Left  motor white  wire  (Channel B)
RIGHT_ENC_A = 19    # BCM 19 → Right motor yellow wire  (Channel A)
RIGHT_ENC_B = 26    # BCM 26 → Right motor white  wire  (Channel B)

# Encoder pulses per full wheel revolution (GA25-370 = 6 PPR × gear ratio 34 ≈ 204)
ENCODER_PPR = 204

# ── HC-SR04 Ultrasonic Sensor ─────────────────────────────────────────────────
# TRIG: 3.3 V output from Pi — connect directly.
# ECHO: 5 V output from sensor — use 1 kΩ + 2 kΩ resistor divider → ~3.33 V.
TRIG_PIN = 24       # BCM 24 → HC-SR04 TRIG
ECHO_PIN = 25       # BCM 25 → HC-SR04 ECHO (after voltage divider)

# ── Downward IR Line / Edge Sensors ──────────────────────────────────────────
# Both sensors face the floor.  Active-LOW: GPIO reads LOW over white boundary.
# Front sensor detects the white line ahead; back sensor detects it behind.
IR_FRONT_PIN = 12   # BCM 12 → Front downward IR sensor OUT
IR_BACK_PIN  = 16   # BCM 16 → Back  downward IR sensor OUT

# ── PWM ──────────────────────────────────────────────────────────────────────
PWM_FREQ = 1000     # Hz — 1 kHz suits L298N and GA25-370 well

# ── Speed constants  (duty cycle 0–100 %) ────────────────────────────────────
SPEED_FULL    = 100  # Maximum attack charge speed
SPEED_SEARCH  = 55   # Rotation speed while scanning
SPEED_REVERSE = 85   # Reverse burst during edge avoidance
SPEED_TURN    = 80   # Spin speed during edge avoidance
SPEED_STALK   = 70   # Approach speed when tracking a moving opponent
SPEED_CORRECT = 40   # Speed differential for heading correction

# ── Timing thresholds (seconds) ──────────────────────────────────────────────
START_DELAY        = 5.0    # Mandatory pre-match pause (sumo rules)
REVERSE_DURATION   = 0.30   # Reverse burst length on edge detection
SPIN_DURATION      = 0.45   # Spin-away duration after reverse
ULTRASONIC_TIMEOUT = 0.025  # Echo wait cap → ~4.25 m max range

# ── Distance thresholds (cm) ──────────────────────────────────────────────────
ATTACK_DISTANCE_CM  = 80.0  # Charge at full speed when opponent closer than this
STALK_DISTANCE_CM   = 150.0 # Begin stalking/tracking when opponent closer than this
CLOSE_DISTANCE_CM   = 25.0  # "Ram" distance — opponent is right in front

# ── FSM / strategy timing (seconds) ──────────────────────────────────────────
SCAN_STEP_TIME      = 0.55  # Time to spin one step before re-checking sensors
LOST_TIMEOUT        = 1.2   # Seconds without detection before returning to SEARCH
ORBIT_DURATION      = 0.25  # Arc-turn duration during ORBIT tactic
RAM_BURST_DURATION  = 0.18  # Short full-speed burst when very close

# ── Main loop ─────────────────────────────────────────────────────────────────
LOOP_HZ = 50        # FSM tick rate (iterations per second)

# ── Power warning (read before wiring) ────────────────────────────────────────
# MT3608 HW-045 is BOOST (step-up) only. Input: single 18650 cell (~3.7 V).
# Output: set to exactly 5.00 V. Verify your module's continuous current rating
# under load — some MT3608 boards deliver only 0.5 A continuous despite "2 A"
# labels. If the Pi resets on motor start, add a 1000 µF/10 V capacitor across
# VOUT+/VOUT− to buffer the transient. Motor power (11.1 V) goes directly to
# L298N only — never connect motor supply to the Pi 5 V rail.
