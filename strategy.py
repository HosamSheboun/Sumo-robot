"""
strategy.py — Adaptive priority FSM for heavyweight sumo competition.

States (highest → lowest priority at runtime):
  EDGE_AVOID  — white line detected; immediate escape maneuver.
  RAM         — opponent is very close (< CLOSE_DISTANCE_CM); burst push.
  ATTACK      — opponent detected within ATTACK_DISTANCE_CM; full-speed charge.
  STALK       — opponent visible but far; arc toward it while tracking.
  ORBIT       — flanking arc to approach from the side.
  SEARCH      — no opponent; spin-scan to locate.
  START       — mandatory 5-second pre-match delay.

TacticAdaptor monitors in-match events and switches between three styles:
  RUSHER   — charge immediately, maximum aggression.
  STALKER  — approach carefully, use arcs to get side-on.
  FLANKER  — orbit wide then ram from the side.

The adaptor promotes/demotes styles based on:
  - How many times the robot has been pushed to the edge (defensive pressure).
  - Whether direct charges have been effective (opponent moved back).
  - Time elapsed in the match (escalate aggression as time runs out).
"""

import time
from enum import Enum, auto

import config
from motors  import MotorController
from sensors import UltrasonicSensor, IRSensors, OpponentTracker


# ── Enumerations ──────────────────────────────────────────────────────────────

class State(Enum):
    START      = auto()
    SEARCH     = auto()
    STALK      = auto()
    ORBIT      = auto()
    ATTACK     = auto()
    RAM        = auto()
    EDGE_AVOID = auto()


class Tactic(Enum):
    RUSHER  = auto()   # Charge hard and fast
    STALKER = auto()   # Track and approach carefully
    FLANKER = auto()   # Arc wide, attack from the side


# ── Tactic adaptor ────────────────────────────────────────────────────────────

class TacticAdaptor:
    """
    Monitors match events and selects the best tactic style.
    Call record_edge_hit() when the robot hits the boundary,
    record_push_success() when an opponent retreats after a charge.
    """

    def __init__(self):
        self._tactic        = Tactic.RUSHER
        self._edge_hits     = 0
        self._push_successes = 0
        self._match_start   = time.monotonic()

    def record_edge_hit(self):
        self._edge_hits += 1
        # If we keep hitting the edge, we are being pushed — switch to flanking
        if self._edge_hits >= 3:
            self._tactic = Tactic.FLANKER

    def record_push_success(self):
        self._push_successes += 1
        # Direct charges are working — stay aggressive
        if self._push_successes >= 2:
            self._tactic = Tactic.RUSHER

    def get(self) -> Tactic:
        # Escalate to RUSHER in the final 30 seconds regardless of history
        elapsed = time.monotonic() - self._match_start
        if elapsed > 150.0:   # 2.5 min into a 3-min match
            return Tactic.RUSHER
        return self._tactic

    def reset(self):
        self.__init__()


# ── Spin-scan direction tracker ───────────────────────────────────────────────

class _ScanTracker:
    """
    Tracks cumulative spin angle during SEARCH so the robot covers
    a full 360° before reversing direction, avoiding repetitive patterns.
    """

    _STEP_DEG = 360.0 * (config.SCAN_STEP_TIME / (60.0 / config.SPEED_SEARCH))

    def __init__(self):
        self._covered   = 0.0
        self._direction = 1      # +1 = clockwise, -1 = counter-clockwise
        self._step_start = time.monotonic()

    def tick(self) -> int:
        """Return current spin direction (+1 or -1). Flip after 360°."""
        if time.monotonic() - self._step_start >= config.SCAN_STEP_TIME:
            self._covered    += self._STEP_DEG
            self._step_start  = time.monotonic()
            if self._covered >= 360.0:
                self._covered    = 0.0
                self._direction *= -1
        return self._direction

    def reset(self):
        self.__init__()


# ── Main FSM ──────────────────────────────────────────────────────────────────

class SumoFSM:
    """
    Adaptive priority FSM.  Call tick() at LOOP_HZ from main.py.
    """

    def __init__(self, motors: MotorController,
                 ultrasonic: UltrasonicSensor,
                 ir: IRSensors):
        self._m       = motors
        self._us      = ultrasonic
        self._ir      = ir
        self._tracker = OpponentTracker()
        self._adaptor = TacticAdaptor()
        self._scan    = _ScanTracker()

        self._state       = State.START
        self._state_start = time.monotonic()

        # Spin angle accumulator for tracker bearing updates
        self._spin_deg_per_tick = (
            config.SPEED_SEARCH / 100.0 * 360.0 / (config.LOOP_HZ * 0.6)
        )
        self._last_spin_dir = 0   # last spin direction used in SEARCH

        # Edge avoidance context
        self._avoid_forward_after = False  # True = drive forward after spin

        # RAM burst context
        self._ram_start = 0.0

        # Orbit context
        self._orbit_start = 0.0
        self._orbit_dir   = 1    # +1 = arc right, -1 = arc left

    # ── Public ───────────────────────────────────────────────────────────────

    def tick(self):
        dist = self._us.distance_cm()
        self._tracker.update(
            dist,
            spin_delta_deg=self._last_spin_dir * self._spin_deg_per_tick
        )
        self._last_spin_dir = 0

        {
            State.START:      self._do_start,
            State.EDGE_AVOID: self._do_edge_avoid,
            State.RAM:        self._do_ram,
            State.ATTACK:     self._do_attack,
            State.STALK:      self._do_stalk,
            State.ORBIT:      self._do_orbit,
            State.SEARCH:     self._do_search,
        }[self._state](dist)

    # ── State handlers ────────────────────────────────────────────────────────

    def _do_start(self, dist: float):
        self._m.stop()
        if self._elapsed() >= config.START_DELAY:
            self._go(State.SEARCH)

    def _do_edge_avoid(self, dist: float):
        """
        Phase 1 (0 → REVERSE_DURATION)   : reverse away from boundary.
        Phase 2 (→ + SPIN_DURATION)       : spin 180° away from the edge.
        Phase 3                           : transition to SEARCH.
        """
        t = self._elapsed()
        rev_end  = config.REVERSE_DURATION
        spin_end = rev_end + config.SPIN_DURATION

        if t < rev_end:
            self._m.reverse()
        elif t < spin_end:
            if self._avoid_forward_after:
                # Back edge hit — we reversed forward, now spin to face ring
                self._m.spin_right()
            else:
                # Front edge hit — we reversed backward, spin to face ring
                self._m.spin_right()
        else:
            self._go(State.SEARCH)

    def _do_ram(self, dist: float):
        """Full-speed burst when opponent is right in front."""
        # Edge check still overrides
        if self._ir.any_edge():
            self._trigger_edge_avoid()
            return
        if time.monotonic() - self._ram_start > config.RAM_BURST_DURATION:
            # Check if push was effective
            if dist > self._tracker.last_distance + 5.0:
                self._adaptor.record_push_success()
            self._go(State.ATTACK)
        else:
            self._m.forward(config.SPEED_FULL)

    def _do_attack(self, dist: float):
        """Charge at full speed; apply heading correction via encoders."""
        if self._ir.any_edge():
            self._trigger_edge_avoid()
            return

        if dist < config.CLOSE_DISTANCE_CM:
            self._ram_start = time.monotonic()
            self._go(State.RAM)
            return

        if dist <= config.ATTACK_DISTANCE_CM:
            # Heading correction: if drifting, nudge the slower motor
            err = self._m.heading_error()
            if err > 5:
                self._m.arc_right(config.SPEED_FULL,
                                  config.SPEED_FULL - config.SPEED_CORRECT)
            elif err < -5:
                self._m.arc_left(config.SPEED_FULL,
                                 config.SPEED_FULL - config.SPEED_CORRECT)
            else:
                self._m.forward(config.SPEED_FULL)
            self._m.reset_encoders()
        else:
            # Lost the opponent
            if self._tracker.is_detected():
                self._go(State.STALK)
            else:
                self._go(State.SEARCH)

    def _do_stalk(self, dist: float):
        """
        Opponent is visible but beyond ATTACK_DISTANCE_CM.
        Arc toward the estimated bearing to close the gap.
        Tactic-dependent: FLANKER orbits wide; others approach directly.
        """
        if self._ir.any_edge():
            self._trigger_edge_avoid()
            return

        if dist <= config.ATTACK_DISTANCE_CM:
            self._go(State.ATTACK)
            return

        if self._tracker.is_lost():
            self._go(State.SEARCH)
            return

        tactic = self._adaptor.get()

        if tactic == Tactic.FLANKER:
            # Switch to orbit to approach from the side
            self._orbit_dir   = 1
            self._orbit_start = time.monotonic()
            self._go(State.ORBIT)
            return

        bearing = self._tracker.last_bearing
        if abs(bearing) < 15.0:
            self._m.forward(config.SPEED_STALK)
        elif bearing > 0:
            self._m.arc_right(config.SPEED_STALK,
                              config.SPEED_STALK - config.SPEED_CORRECT)
        else:
            self._m.arc_left(config.SPEED_STALK,
                             config.SPEED_STALK - config.SPEED_CORRECT)

    def _do_orbit(self, dist: float):
        """
        Arc around the opponent to approach from the side (FLANKER tactic).
        After ORBIT_DURATION, re-evaluate.
        """
        if self._ir.any_edge():
            self._trigger_edge_avoid()
            return

        if dist <= config.ATTACK_DISTANCE_CM:
            self._go(State.ATTACK)
            return

        if time.monotonic() - self._orbit_start > config.ORBIT_DURATION:
            self._go(State.STALK)
            return

        if self._orbit_dir > 0:
            self._m.arc_right(config.SPEED_STALK,
                              config.SPEED_STALK - config.SPEED_CORRECT)
        else:
            self._m.arc_left(config.SPEED_STALK,
                             config.SPEED_STALK - config.SPEED_CORRECT)

    def _do_search(self, dist: float):
        """
        Spin-scan to locate the opponent.
        Covers a full 360° then reverses direction to avoid cable wrap.
        Transitions immediately on any detection.
        """
        if self._ir.any_edge():
            self._trigger_edge_avoid()
            return

        if dist <= config.ATTACK_DISTANCE_CM:
            self._go(State.ATTACK)
            return

        if dist <= config.STALK_DISTANCE_CM:
            self._go(State.STALK)
            return

        direction = self._scan.tick()
        self._last_spin_dir = direction

        if direction > 0:
            self._m.spin_right(config.SPEED_SEARCH)
        else:
            self._m.spin_left(config.SPEED_SEARCH)

    # ── Helpers ───────────────────────────────────────────────────────────────

    def _elapsed(self) -> float:
        return time.monotonic() - self._state_start

    def _go(self, state: State):
        self._state       = state
        self._state_start = time.monotonic()

    def _trigger_edge_avoid(self):
        """Record the edge hit, choose avoidance direction, enter EDGE_AVOID."""
        self._adaptor.record_edge_hit()
        # If back sensor fired → we need to drive forward after spin
        self._avoid_forward_after = self._ir.edge_back() and not self._ir.edge_front()
        self._go(State.EDGE_AVOID)
