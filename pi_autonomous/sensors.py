"""
sensors.py — Sensor abstraction layer.

Devices
  HC-SR04  : Front-facing ultrasonic distance sensor.
             Echo pin outputs 5 V — use 1 kΩ + 2 kΩ resistor divider → 3.33 V
             before connecting to Pi BCM25.
  IR front : Downward-facing IR line sensor at the front of the chassis.
             Active-LOW: reads LOW when white boundary line is detected.
  IR back  : Downward-facing IR line sensor at the rear of the chassis.
             Active-LOW: reads LOW when white boundary line is detected.
"""

import time
import collections
import RPi.GPIO as gpio
import config


class UltrasonicSensor:
    """
    HC-SR04 non-blocking distance measurement.
    Returns 999.0 cm on timeout (no echo = no obstacle in range).
    """

    # Rolling window for simple noise filtering
    _HISTORY = 4

    def __init__(self):
        gpio.setmode(gpio.BCM)
        gpio.setwarnings(False)
        gpio.setup(config.TRIG_PIN, gpio.OUT, initial=gpio.LOW)
        gpio.setup(config.ECHO_PIN, gpio.IN)
        self._history = collections.deque(
            [999.0] * self._HISTORY, maxlen=self._HISTORY
        )

    def distance_cm(self) -> float:
        """
        Fire one ultrasonic pulse and return filtered distance in cm.
        Uses a 4-sample median filter to reject spurious spikes.
        """
        raw = self._raw_distance()
        self._history.append(raw)
        sorted_h = sorted(self._history)
        # Median of 4 samples (average of middle two)
        return (sorted_h[1] + sorted_h[2]) / 2.0

    def _raw_distance(self) -> float:
        # 10 µs trigger pulse
        gpio.output(config.TRIG_PIN, gpio.HIGH)
        time.sleep(0.00001)
        gpio.output(config.TRIG_PIN, gpio.LOW)

        deadline = time.monotonic() + config.ULTRASONIC_TIMEOUT

        while gpio.input(config.ECHO_PIN) == gpio.LOW:
            if time.monotonic() > deadline:
                return 999.0
        t_start = time.monotonic()

        while gpio.input(config.ECHO_PIN) == gpio.HIGH:
            if time.monotonic() > deadline:
                return 999.0
        t_end = time.monotonic()

        return (t_end - t_start) * 17150.0   # cm = µs × 0.01715


class IRSensors:
    """
    Front and back downward-facing IR line sensors.
    Both are active-LOW: GPIO LOW = white boundary line detected.
    """

    def __init__(self):
        gpio.setmode(gpio.BCM)
        gpio.setwarnings(False)
        for pin in (config.IR_FRONT_PIN, config.IR_BACK_PIN):
            gpio.setup(pin, gpio.IN, pull_up_down=gpio.PUD_UP)

    def edge_front(self) -> bool:
        """True when the front IR sensor sees the white boundary line."""
        return gpio.input(config.IR_FRONT_PIN) == gpio.LOW

    def edge_back(self) -> bool:
        """True when the back IR sensor sees the white boundary line."""
        return gpio.input(config.IR_BACK_PIN) == gpio.LOW

    def any_edge(self) -> bool:
        return self.edge_front() or self.edge_back()


class OpponentTracker:
    """
    Lightweight opponent position estimator.

    Combines ultrasonic distance readings with the robot's own spin history
    to maintain a best-guess bearing to the opponent even when the sensor
    briefly loses the target.

    Bearing convention:
      0   = directly ahead
      +ve = opponent to the right
      -ve = opponent to the left
    """

    # How many ticks of no detection before confidence drops to zero
    _DECAY_TICKS = int(config.LOOP_HZ * config.LOST_TIMEOUT)

    def __init__(self):
        self.last_distance   = 999.0   # cm
        self.last_bearing    = 0.0     # degrees (estimated)
        self.confidence      = 0.0     # 0.0 – 1.0
        self._no_detect_ticks = 0
        self._distance_history = collections.deque(maxlen=6)

    def update(self, distance_cm: float, spin_delta_deg: float = 0.0):
        """
        Call every FSM tick with the latest ultrasonic reading and how many
        degrees the robot spun since the last tick (positive = clockwise).

        spin_delta_deg lets the tracker rotate the estimated bearing even
        when the sensor returns no echo.
        """
        # Rotate stored bearing by how much the robot turned
        self.last_bearing = _wrap_180(self.last_bearing - spin_delta_deg)

        if distance_cm < config.STALK_DISTANCE_CM:
            self._distance_history.append(distance_cm)
            self.last_distance    = distance_cm
            self.last_bearing     = 0.0   # ultrasonic is front-facing → bearing 0
            self._no_detect_ticks = 0
            self.confidence       = 1.0
        else:
            self._no_detect_ticks += 1
            decay = self._no_detect_ticks / self._DECAY_TICKS
            self.confidence = max(0.0, 1.0 - decay)

    def is_detected(self) -> bool:
        return self.confidence > 0.5

    def is_lost(self) -> bool:
        return self.confidence < 0.1

    def avg_distance(self) -> float:
        if not self._distance_history:
            return 999.0
        return sum(self._distance_history) / len(self._distance_history)

    def closing_fast(self) -> bool:
        """True if the last two distance readings show rapid approach."""
        h = list(self._distance_history)
        if len(h) < 2:
            return False
        return (h[-2] - h[-1]) > 8.0   # closing > 8 cm per tick


def _wrap_180(angle: float) -> float:
    """Wrap an angle to the range (−180, +180]."""
    while angle >  180.0:
        angle -= 360.0
    while angle <= -180.0:
        angle += 360.0
    return angle
