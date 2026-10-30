"""
config.py — Hardware configuration for manual Dabble Bluetooth joystick control.

Controller  : Raspberry Pi Zero 2 W (built-in Bluetooth)
Motor driver: L298N dual H-bridge
Motors      : 2× GA25-370 12V 130RPM DC gear motor with quadrature encoder
Control     : Dabble app (iOS / Android) → Gamepad module → Joystick
Power       : 3S 18650 (11.1V) → L298N/motors direct
              Single cell tap (3.7V) → MT3608 boost → 5.0V → Pi + encoders

GA25-370 encoder wire colours
  Red       → Motor terminal +  (to L298N OUTx)
  Black     → Motor terminal −  (to L298N OUTx)
  Green     → Encoder VCC 5V   (from MT3608 5V rail)
  Dark Blue → Encoder GND      (common GND rail)
  Yellow    → Encoder Channel A (GPIO input)
  White     → Encoder Channel B (GPIO input)
"""

# ── L298N — Left motor (Motor A) ─────────────────────────────────────────────
LEFT_IN1 = 17       # BCM 17 → L298N IN1  direction bit A
LEFT_IN2 = 27       # BCM 27 → L298N IN2  direction bit B
LEFT_ENA = 18       # BCM 18 → L298N ENA  PWM speed

# ── L298N — Right motor (Motor B) ────────────────────────────────────────────
RIGHT_IN3 = 22      # BCM 22 → L298N IN3  direction bit A
RIGHT_IN4 = 23      # BCM 23 → L298N IN4  direction bit B
RIGHT_ENB = 13      # BCM 13 → L298N ENB  PWM speed

# ── GA25-370 Quadrature Encoders ─────────────────────────────────────────────
LEFT_ENC_A  = 5     # BCM  5 → Left  motor yellow wire (Channel A)
LEFT_ENC_B  = 6     # BCM  6 → Left  motor white  wire (Channel B)
RIGHT_ENC_A = 19    # BCM 19 → Right motor yellow wire (Channel A)
RIGHT_ENC_B = 26    # BCM 26 → Right motor white  wire (Channel B)

# ── Rocker Switch ─────────────────────────────────────────────────────────────
# 2-pin black rocker switch. One pin → BCM21, other pin → GND.
# Switch ON  → GPIO reads LOW  (closed to GND, internal pull-up)
# Switch OFF → GPIO reads HIGH (open circuit, pull-up holds HIGH)
SWITCH_PIN = 21     # BCM 21 → Rocker switch (Physical Pin 40)

# ── Status LED ────────────────────────────────────────────────────────────────
# LED + 330Ω resistor in series between BCM20 and GND.
# LED SOLID ON  = Dabble connected, robot ready to drive
# LED OFF       = waiting for Dabble to connect, or switch is OFF
LED_PIN = 20        # BCM 20 → LED anode via 330Ω resistor (Physical Pin 38)

# ── PWM ──────────────────────────────────────────────────────────────────────
PWM_FREQ = 1000     # Hz

# ── Speed constants (duty cycle 0–100%) ──────────────────────────────────────
SPEED_MAX     = 100  # Maximum motor speed (full joystick deflection)
SPEED_MIN     = 20   # Minimum speed to actually move (below this = stop)
                     # Prevents motors humming without moving at tiny joystick values

# ── Joystick settings ─────────────────────────────────────────────────────────
# Dabble joystick returns X and Y values in range -7 to +7.
# DEADZONE: joystick values within ±DEADZONE of centre are treated as zero.
# This prevents the robot creeping when the joystick is released.
JOYSTICK_MAX      = 7      # Maximum value Dabble joystick returns
JOYSTICK_DEADZONE = 1      # Values -1 to +1 treated as zero (centre dead zone)

# ── Bluetooth ─────────────────────────────────────────────────────────────────
# Dabble connects over Bluetooth Serial Port Profile (SPP).
# The PyDabble library handles the connection automatically.
# BT_NAME is what appears in your phone's Bluetooth device list.
BT_NAME = "SumoBot"

# ── Main loop ─────────────────────────────────────────────────────────────────
LOOP_HZ = 50        # Control loop rate (iterations per second)
