"""
motors.py — L298N dual H-bridge motor controller for two GA25-370 DC gear motors.

Wiring assumption (BCM pins defined in config.py):
  Left motor  : IN1, IN2 for direction; ENA for PWM speed.
  Right motor : IN3, IN4 for direction; ENB for PWM speed.

The L298N logic inputs accept 3.3 V signals from the Pi GPIO directly.
"""

import RPi.GPIO as gpio
import config


class MotorController:
    """Controls both drive motors through the L298N H-bridge."""

    def __init__(self):
        gpio.setmode(gpio.BCM)
        gpio.setwarnings(False)

        # Direction pins
        for pin in (config.LEFT_IN1, config.LEFT_IN2,
                    config.RIGHT_IN3, config.RIGHT_IN4):
            gpio.setup(pin, gpio.OUT)
            gpio.output(pin, gpio.LOW)

        # PWM speed pins
        gpio.setup(config.LEFT_ENA,  gpio.OUT)
        gpio.setup(config.RIGHT_ENB, gpio.OUT)

        self._left_pwm  = gpio.PWM(config.LEFT_ENA,  config.PWM_FREQ)
        self._right_pwm = gpio.PWM(config.RIGHT_ENB, config.PWM_FREQ)
        self._left_pwm.start(0)
        self._right_pwm.start(0)

        # Encoder pulse counters (incremented by interrupt callbacks)
        self.left_ticks  = 0
        self.right_ticks = 0
        self._setup_encoders()

    # ── Encoder setup ────────────────────────────────────────────────────────

    def _setup_encoders(self):
        """Attach rising-edge interrupts to encoder channel-A pins."""
        for pin, cb in (
            (config.LEFT_ENC_A,  self._left_tick),
            (config.RIGHT_ENC_A, self._right_tick),
        ):
            gpio.setup(pin, gpio.IN, pull_up_down=gpio.PUD_UP)
            gpio.add_event_detect(pin, gpio.RISING, callback=cb)

    def _left_tick(self, _channel):
        self.left_ticks += 1

    def _right_tick(self, _channel):
        self.right_ticks += 1

    def reset_ticks(self):
        """Zero both encoder counters (call before a timed distance move)."""
        self.left_ticks = 0
        self.right_ticks = 0

    # ── Low-level drive primitives ───────────────────────────────────────────

    def _set_left(self, speed: int, forward: bool):
        """Set left motor direction and duty cycle (0–100)."""
        gpio.output(config.LEFT_IN1, gpio.HIGH if forward else gpio.LOW)
        gpio.output(config.LEFT_IN2, gpio.LOW  if forward else gpio.HIGH)
        self._left_pwm.ChangeDutyCycle(max(0, min(100, speed)))

    def _set_right(self, speed: int, forward: bool):
        """Set right motor direction and duty cycle (0–100)."""
        gpio.output(config.RIGHT_IN3, gpio.HIGH if forward else gpio.LOW)
        gpio.output(config.RIGHT_IN4, gpio.LOW  if forward else gpio.HIGH)
        self._right_pwm.ChangeDutyCycle(max(0, min(100, speed)))

    # ── High-level movement commands ─────────────────────────────────────────

    def forward(self, speed: int = config.SPEED_FULL):
        self._set_left(speed,  True)
        self._set_right(speed, True)

    def reverse(self, speed: int = config.SPEED_REVERSE):
        self._set_left(speed,  False)
        self._set_right(speed, False)

    def spin_left(self, speed: int = config.SPEED_TURN):
        """Rotate counter-clockwise in place."""
        self._set_left(speed,  False)
        self._set_right(speed, True)

    def spin_right(self, speed: int = config.SPEED_TURN):
        """Rotate clockwise in place."""
        self._set_left(speed,  True)
        self._set_right(speed, False)

    def stop(self):
        self._set_left(0,  True)
        self._set_right(0, True)

    # ── Resource cleanup ─────────────────────────────────────────────────────

    def cleanup(self):
        """Stop motors and release PWM resources (call on shutdown)."""
        self.stop()
        self._left_pwm.stop()
        self._right_pwm.stop()
