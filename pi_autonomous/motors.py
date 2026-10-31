"""
motors.py — L298N dual H-bridge controller with full quadrature encoder feedback.

Quadrature decoding (channels A + B) gives:
  - Pulse count for distance estimation
  - Rotation direction verification
  - Velocity estimation (ticks / second)

Wire mapping (GA25-370):
  Red / Black  → L298N motor output terminals
  Green        → 5 V encoder VCC  (MT3608 rail)
  Dark Blue    → GND
  Yellow       → Channel A  (GPIO interrupt)
  White        → Channel B  (GPIO read at A-edge for direction)
"""

import time
import threading
import RPi.GPIO as gpio
import config


class _Encoder:
    """
    Quadrature encoder for one GA25-370 motor.
    Counts signed ticks: positive = forward, negative = reverse.
    """

    def __init__(self, pin_a: int, pin_b: int):
        self._pin_a = pin_a
        self._pin_b = pin_b
        self._lock  = threading.Lock()
        self.ticks  = 0          # signed cumulative count
        self._last_time = time.monotonic()
        self._last_ticks = 0
        self.velocity = 0.0      # ticks per second (updated on each A edge)

        gpio.setup(pin_a, gpio.IN, pull_up_down=gpio.PUD_UP)
        gpio.setup(pin_b, gpio.IN, pull_up_down=gpio.PUD_UP)
        gpio.add_event_detect(pin_a, gpio.RISING, callback=self._on_a_rise)

    def _on_a_rise(self, _channel):
        now = time.monotonic()
        # Direction: if B is HIGH when A rises → forward; LOW → reverse
        direction = 1 if gpio.input(self._pin_b) == gpio.HIGH else -1
        with self._lock:
            self.ticks += direction
            dt = now - self._last_time
            if dt > 0:
                self.velocity = (self.ticks - self._last_ticks) / dt
            self._last_time   = now
            self._last_ticks  = self.ticks

    def reset(self):
        with self._lock:
            self.ticks        = 0
            self._last_ticks  = 0
            self._last_time   = time.monotonic()
            self.velocity     = 0.0

    def get_ticks(self) -> int:
        with self._lock:
            return self.ticks

    def get_velocity(self) -> float:
        """Ticks per second — positive = forward."""
        with self._lock:
            return self.velocity


class MotorController:
    """
    Controls both GA25-370 motors through the L298N H-bridge.
    Exposes high-level movement commands and encoder data.
    """

    def __init__(self):
        gpio.setmode(gpio.BCM)
        gpio.setwarnings(False)

        # Direction control pins
        for pin in (config.LEFT_IN1, config.LEFT_IN2,
                    config.RIGHT_IN3, config.RIGHT_IN4):
            gpio.setup(pin, gpio.OUT)
            gpio.output(pin, gpio.LOW)

        # PWM speed pins
        gpio.setup(config.LEFT_ENA,  gpio.OUT)
        gpio.setup(config.RIGHT_ENB, gpio.OUT)
        self._lpwm = gpio.PWM(config.LEFT_ENA,  config.PWM_FREQ)
        self._rpwm = gpio.PWM(config.RIGHT_ENB, config.PWM_FREQ)
        self._lpwm.start(0)
        self._rpwm.start(0)

        # Full quadrature encoders
        self.left_enc  = _Encoder(config.LEFT_ENC_A,  config.LEFT_ENC_B)
        self.right_enc = _Encoder(config.RIGHT_ENC_A, config.RIGHT_ENC_B)

    # ── Low-level primitives ──────────────────────────────────────────────────

    def _drive_left(self, speed: int, forward: bool):
        gpio.output(config.LEFT_IN1, gpio.HIGH if forward else gpio.LOW)
        gpio.output(config.LEFT_IN2, gpio.LOW  if forward else gpio.HIGH)
        self._lpwm.ChangeDutyCycle(max(0, min(100, abs(speed))))

    def _drive_right(self, speed: int, forward: bool):
        gpio.output(config.RIGHT_IN3, gpio.HIGH if forward else gpio.LOW)
        gpio.output(config.RIGHT_IN4, gpio.LOW  if forward else gpio.HIGH)
        self._rpwm.ChangeDutyCycle(max(0, min(100, abs(speed))))

    # ── High-level movement commands ─────────────────────────────────────────

    def forward(self, speed: int = config.SPEED_FULL):
        self._drive_left(speed,  True)
        self._drive_right(speed, True)

    def reverse(self, speed: int = config.SPEED_REVERSE):
        self._drive_left(speed,  False)
        self._drive_right(speed, False)

    def spin_left(self, speed: int = config.SPEED_TURN):
        """Counter-clockwise rotation in place."""
        self._drive_left(speed,  False)
        self._drive_right(speed, True)

    def spin_right(self, speed: int = config.SPEED_TURN):
        """Clockwise rotation in place."""
        self._drive_left(speed,  True)
        self._drive_right(speed, False)

    def arc_left(self, outer: int = config.SPEED_FULL,
                 inner: int = config.SPEED_STALK):
        """Curve left: right motor faster than left."""
        self._drive_left(inner,  True)
        self._drive_right(outer, True)

    def arc_right(self, outer: int = config.SPEED_FULL,
                  inner: int = config.SPEED_STALK):
        """Curve right: left motor faster than right."""
        self._drive_left(outer, True)
        self._drive_right(inner, True)

    def stop(self):
        self._drive_left(0,  True)
        self._drive_right(0, True)

    # ── Encoder helpers ───────────────────────────────────────────────────────

    def reset_encoders(self):
        self.left_enc.reset()
        self.right_enc.reset()

    def avg_ticks(self) -> float:
        return (self.left_enc.get_ticks() + self.right_enc.get_ticks()) / 2.0

    def avg_velocity(self) -> float:
        """Average forward velocity in ticks/second across both motors."""
        return (self.left_enc.get_velocity() + self.right_enc.get_velocity()) / 2.0

    def heading_error(self) -> float:
        """
        Signed tick difference (left − right).
        Positive → drifting right; negative → drifting left.
        """
        return float(self.left_enc.get_ticks() - self.right_enc.get_ticks())

    # ── Cleanup ───────────────────────────────────────────────────────────────

    def cleanup(self):
        self.stop()
        self._lpwm.stop()
        self._rpwm.stop()
