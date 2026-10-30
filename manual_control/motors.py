"""
motors.py — L298N dual H-bridge motor controller for manual joystick drive.

Exposes:
  drive(left_speed, right_speed) — direct per-motor speed control (-100 to +100)
  tank_mix(x, y)                 — convert joystick X/Y to left/right motor speeds
  stop()                         — immediately stop both motors
  cleanup()                      — release PWM and GPIO resources

Tank-drive mixing:
  Joystick Y (forward/back) controls base speed for both motors.
  Joystick X (left/right)   adds/subtracts from each side to steer.

  left_speed  = Y + X
  right_speed = Y - X

  Both values are clamped to [-100, +100] and scaled from joystick range.
"""

import threading
import time
import RPi.GPIO as gpio
import config


class _Encoder:
    """
    Quadrature encoder — counts signed ticks (+ = forward, − = reverse).
    Uses both Channel A and B for direction detection.
    """

    def __init__(self, pin_a: int, pin_b: int):
        self._pin_b = pin_b
        self._lock  = threading.Lock()
        self.ticks  = 0

        gpio.setup(pin_a, gpio.IN, pull_up_down=gpio.PUD_UP)
        gpio.setup(pin_b, gpio.IN, pull_up_down=gpio.PUD_UP)
        gpio.add_event_detect(pin_a, gpio.RISING, callback=self._on_rise)

    def _on_rise(self, _ch):
        direction = 1 if gpio.input(self._pin_b) == gpio.HIGH else -1
        with self._lock:
            self.ticks += direction

    def get_ticks(self) -> int:
        with self._lock:
            return self.ticks

    def reset(self):
        with self._lock:
            self.ticks = 0


class MotorController:
    """Controls both GA25-370 motors through the L298N H-bridge."""

    def __init__(self):
        gpio.setmode(gpio.BCM)
        gpio.setwarnings(False)

        for pin in (config.LEFT_IN1, config.LEFT_IN2,
                    config.RIGHT_IN3, config.RIGHT_IN4):
            gpio.setup(pin, gpio.OUT)
            gpio.output(pin, gpio.LOW)

        gpio.setup(config.LEFT_ENA,  gpio.OUT)
        gpio.setup(config.RIGHT_ENB, gpio.OUT)
        self._lpwm = gpio.PWM(config.LEFT_ENA,  config.PWM_FREQ)
        self._rpwm = gpio.PWM(config.RIGHT_ENB, config.PWM_FREQ)
        self._lpwm.start(0)
        self._rpwm.start(0)

        self.left_enc  = _Encoder(config.LEFT_ENC_A,  config.LEFT_ENC_B)
        self.right_enc = _Encoder(config.RIGHT_ENC_A, config.RIGHT_ENC_B)

    # ── Low-level ─────────────────────────────────────────────────────────────

    def _set_motor(self, pwm, pin_fwd, pin_rev, speed: int):
        """
        Set one motor. speed is -100 to +100.
        Positive = forward, negative = reverse, 0 = stop.
        """
        speed = max(-100, min(100, speed))
        if abs(speed) < config.SPEED_MIN:
            gpio.output(pin_fwd, gpio.LOW)
            gpio.output(pin_rev, gpio.LOW)
            pwm.ChangeDutyCycle(0)
        elif speed > 0:
            gpio.output(pin_fwd, gpio.HIGH)
            gpio.output(pin_rev, gpio.LOW)
            pwm.ChangeDutyCycle(speed)
        else:
            gpio.output(pin_fwd, gpio.LOW)
            gpio.output(pin_rev, gpio.HIGH)
            pwm.ChangeDutyCycle(abs(speed))

    # ── Public interface ──────────────────────────────────────────────────────

    def drive(self, left_speed: int, right_speed: int):
        """
        Drive both motors independently.
        left_speed, right_speed: -100 (full reverse) to +100 (full forward).
        """
        self._set_motor(self._lpwm, config.LEFT_IN1,  config.LEFT_IN2,  left_speed)
        self._set_motor(self._rpwm, config.RIGHT_IN3, config.RIGHT_IN4, right_speed)

    def tank_mix(self, x: float, y: float):
        """
        Convert normalised joystick X/Y (-1.0 to +1.0) to motor speeds.
        Y = forward/backward, X = left/right steering.
        """
        left  = (y + x) * config.SPEED_MAX
        right = (y - x) * config.SPEED_MAX
        self.drive(int(left), int(right))

    def stop(self):
        self.drive(0, 0)

    def cleanup(self):
        self.stop()
        self._lpwm.stop()
        self._rpwm.stop()
