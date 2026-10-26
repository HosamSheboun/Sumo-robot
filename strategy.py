"""
strategy.py — Finite State Machine (FSM) for autonomous sumo competition.

State priority (highest → lowest):
  1. START       — mandatory 5-second pre-match delay.
  2. EDGE_AVOID  — triggered by white-line detection; highest runtime priority.
  3. ATTACK      — opponent detected within ATTACK_DISTANCE_CM; charge at full speed.
  4. SEARCH      — no opponent visible; rotate to scan the ring.

Transitions are evaluated every FSM tick (called from main.py at LOOP_HZ).
"""

import time
from enum import Enum, auto

import config
from motors  import MotorController
from sensors import UltrasonicSensor, IRSensors


class State(Enum):
    START      = auto()
    EDGE_AVOID = auto()
    ATTACK     = auto()
    SEARCH     = auto()


class SumoFSM:
    """
    Encapsulates all match logic. Call tick() in a tight loop from main.py.
    """

    def __init__(self, motors: MotorController,
                 ultrasonic: UltrasonicSensor,
                 ir: IRSensors):
        self._m   = motors
        self._us  = ultrasonic
        self._ir  = ir

        self._state      = State.START
        self._state_start = time.monotonic()   # timestamp of last state entry
        self._search_dir  = 1                  # +1 = spin right, -1 = spin left

    # ── Public interface ─────────────────────────────────────────────────────

    def tick(self):
        """Evaluate one FSM cycle. Call this at LOOP_HZ from main.py."""
        {
            State.START:      self._state_start_delay,
            State.EDGE_AVOID: self._state_edge_avoid,
            State.ATTACK:     self._state_attack,
            State.SEARCH:     self._state_search,
        }[self._state]()

    # ── State handlers ───────────────────────────────────────────────────────

    def _state_start_delay(self):
        """Wait START_DELAY seconds before any movement (mandatory sumo rule)."""
        self._m.stop()
        if self._elapsed() >= config.START_DELAY:
            self._transition(State.SEARCH)

    def _state_edge_avoid(self):
        """
        Reverse away from the boundary, then spin toward the ring centre.
        The spin direction is chosen to move away from whichever sensor fired.
        """
        elapsed = self._elapsed()

        if elapsed < config.REVERSE_DURATION:
            self._m.reverse()
        elif elapsed < config.REVERSE_DURATION + config.SPIN_DURATION:
            # Spin away from the detected edge
            if self._spin_away_right:
                self._m.spin_right()
            else:
                self._m.spin_left()
        else:
            # Avoidance complete — resume normal operation
            self._transition(State.SEARCH)

    def _state_attack(self):
        """Charge at full speed toward the detected opponent."""
        # Edge detection overrides attack at all times
        if self._ir.any_edge():
            self._record_edge_side()
            self._transition(State.EDGE_AVOID)
            return

        dist = self._us.distance_cm()
        if dist <= config.ATTACK_DISTANCE_CM:
            self._m.forward(config.SPEED_FULL)
        else:
            # Opponent lost — switch to search
            self._transition(State.SEARCH)

    def _state_search(self):
        """Rotate in place to scan for the opponent."""
        # Edge detection is still the highest priority
        if self._ir.any_edge():
            self._record_edge_side()
            self._transition(State.EDGE_AVOID)
            return

        # Check ultrasonic first (longer range)
        dist = self._us.distance_cm()
        if dist <= config.ATTACK_DISTANCE_CM:
            self._transition(State.ATTACK)
            return

        # Check short-range front IR as a secondary trigger
        if self._ir.obstacle_detected():
            self._transition(State.ATTACK)
            return

        # Alternate spin direction every 0.6 s to cover the full ring
        if self._elapsed() > 0.6:
            self._search_dir *= -1
            self._state_start = time.monotonic()

        if self._search_dir > 0:
            self._m.spin_right(config.SPEED_SEARCH)
        else:
            self._m.spin_left(config.SPEED_SEARCH)

    # ── Helpers ──────────────────────────────────────────────────────────────

    def _elapsed(self) -> float:
        """Seconds since the current state was entered."""
        return time.monotonic() - self._state_start

    def _transition(self, new_state: State):
        self._state       = new_state
        self._state_start = time.monotonic()

    def _record_edge_side(self):
        """
        Decide spin direction for edge avoidance based on which sensor fired.
        spin_away_right=True  → spin clockwise (away from left edge trigger).
        spin_away_right=False → spin counter-clockwise (away from right edge trigger).
        """
        # If only the right sensor fired, spin left; otherwise spin right.
        self._spin_away_right = not (
            self._ir.edge_right() and not self._ir.edge_left()
        )
