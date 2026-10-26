"""
sensors.py — Sensor abstraction for HC-SR04 ultrasonic, front IR obstacle,
             and downward-facing IR edge/line detectors.

HC-SR04 Echo pin protection:
  The Echo pin outputs 5 V. Use a resistor divider (1 kΩ series + 2 kΩ to GND)
  to reduce it to ~3.3 V before connecting to the Pi GPIO. See wiring guide.
"""

import time
import RPi.GPIO as gpio
import config


class UltrasonicSensor:
    """HC-SR04 distance measurement with a hard timeout to avoid blocking."""

    def __init__(self):
        gpio.setmode(gpio.BCM)
        gpio.setwarnings(False)
        gpio.setup(config.TRIG_PIN, gpio.OUT, initial=gpio.LOW)
        gpio.setup(config.ECHO_PIN, gpio.IN)

    def distance_cm(self) -> float:
        """
        Return measured distance in centimetres.
        Returns 999.0 on timeout (no echo received within ULTRASONIC_TIMEOUT).
        """
        # Send 10 µs trigger pulse
        gpio.output(config.TRIG_PIN, gpio.HIGH)
        time.sleep(0.00001)
        gpio.output(config.TRIG_PIN, gpio.LOW)

        deadline = time.monotonic() + config.ULTRASONIC_TIMEOUT

        # Wait for echo to go HIGH
        while gpio.input(config.ECHO_PIN) == gpio.LOW:
            if time.monotonic() > deadline:
                return 999.0
        pulse_start = time.monotonic()

        # Wait for echo to go LOW
        while gpio.input(config.ECHO_PIN) == gpio.HIGH:
            if time.monotonic() > deadline:
                return 999.0
        pulse_end = time.monotonic()

        # Speed of sound: 34300 cm/s; divide by 2 for one-way distance
        return (pulse_end - pulse_start) * 17150.0


class IRSensors:
    """
    Reads the front-facing IR obstacle sensor and two downward edge sensors.

    All three sensors use active-LOW digital outputs:
      - obstacle_detected() → True when the front IR sees an object.
      - edge_left()         → True when the left edge sensor sees white line.
      - edge_right()        → True when the right edge sensor sees white line.
    """

    def __init__(self):
        gpio.setmode(gpio.BCM)
        gpio.setwarnings(False)
        for pin in (config.IR_FRONT_PIN,
                    config.EDGE_LEFT_PIN,
                    config.EDGE_RIGHT_PIN):
            # Internal pull-up keeps the line HIGH when sensor is idle
            gpio.setup(pin, gpio.IN, pull_up_down=gpio.PUD_UP)

    def obstacle_detected(self) -> bool:
        """True when the front IR sensor output is LOW (obstacle present)."""
        return gpio.input(config.IR_FRONT_PIN) == gpio.LOW

    def edge_left(self) -> bool:
        """True when the left downward IR sensor detects the white boundary."""
        return gpio.input(config.EDGE_LEFT_PIN) == gpio.LOW

    def edge_right(self) -> bool:
        """True when the right downward IR sensor detects the white boundary."""
        return gpio.input(config.EDGE_RIGHT_PIN) == gpio.LOW

    def any_edge(self) -> bool:
        """True if either edge sensor is triggered."""
        return self.edge_left() or self.edge_right()
