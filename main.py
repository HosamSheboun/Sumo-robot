"""
main.py — Entry point for the autonomous sumo robot.

Features added:
  - Rocker switch (BCM21): when OFF, motors stop instantly; Pi stays alive.
  - Status LED (BCM20): OFF=stopped, BLINK=start delay, ON=armed & running.
  - Hardware watchdog (/dev/watchdog): reboots Pi automatically if code freezes.
  - No-sleep: writes to /sys/class/leds to keep the Pi fully awake at all times.

Rocker switch logic:
  Switch ON  → BCM21 reads LOW  (closed to GND, internal pull-up active)
  Switch OFF → BCM21 reads HIGH (open circuit, pull-up holds HIGH)
"""

import time
import signal
import sys
import os
import logging
import threading

import RPi.GPIO as gpio

import config
from motors   import MotorController
from sensors  import UltrasonicSensor, IRSensors
from strategy import SumoFSM

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)
log = logging.getLogger(__name__)

_TICK = 1.0 / config.LOOP_HZ

# ── Watchdog ──────────────────────────────────────────────────────────────────
# The Linux hardware watchdog reboots the Pi if it is not "pet" every interval.
# We pet it every second from a background thread.
_WATCHDOG_PATH     = "/dev/watchdog"
_WATCHDOG_INTERVAL = 5   # seconds between pets (must be < watchdog timeout)
_watchdog_fd       = None

def _open_watchdog():
    global _watchdog_fd
    try:
        _watchdog_fd = open(_WATCHDOG_PATH, "w", buffering=1)
        log.info("Hardware watchdog opened.")
    except OSError:
        log.warning("Watchdog not available — continuing without it.")

def _pet_watchdog():
    """Write a keep-alive byte to the watchdog device."""
    if _watchdog_fd:
        try:
            _watchdog_fd.write("1")
            _watchdog_fd.flush()
        except OSError:
            pass

def _close_watchdog():
    """Write magic 'V' to disarm watchdog before clean shutdown."""
    if _watchdog_fd:
        try:
            _watchdog_fd.write("V")
            _watchdog_fd.flush()
            _watchdog_fd.close()
        except OSError:
            pass

def _watchdog_thread():
    """Background thread that pets the watchdog every _WATCHDOG_INTERVAL s."""
    while _keep_running:
        _pet_watchdog()
        time.sleep(_WATCHDOG_INTERVAL)

# ── No-sleep (prevent Pi from idling) ────────────────────────────────────────
def _disable_sleep():
    """
    Prevent the Pi from entering any CPU idle/sleep state.
    Writes 'performance' governor and disables HDMI blanking (headless safe).
    """
    # Set CPU governor to performance (no frequency scaling)
    try:
        for cpu in range(4):   # Pi Zero 2 W has 4 cores
            path = f"/sys/devices/system/cpu/cpu{cpu}/cpufreq/scaling_governor"
            if os.path.exists(path):
                with open(path, "w") as f:
                    f.write("performance")
        log.info("CPU governor set to performance.")
    except OSError:
        log.warning("Could not set CPU governor (may need root).")

    # Disable console blanking
    try:
        with open("/sys/class/graphics/fbcon/cursor_blink", "w") as f:
            f.write("0")
    except OSError:
        pass

# ── LED helpers ───────────────────────────────────────────────────────────────
def _led_on():
    gpio.output(config.LED_PIN, gpio.HIGH)

def _led_off():
    gpio.output(config.LED_PIN, gpio.LOW)

def _led_blink(times: int = 1, on_ms: int = 120, off_ms: int = 120):
    """Blocking blink — only call from main thread during start delay."""
    for _ in range(times):
        _led_on();  time.sleep(on_ms  / 1000)
        _led_off(); time.sleep(off_ms / 1000)

# ── Switch helper ─────────────────────────────────────────────────────────────
def _switch_is_on() -> bool:
    """
    Returns True when the rocker switch is in the ON position.
    Switch ON  → pin pulled LOW (closed to GND).
    Switch OFF → pin HIGH (internal pull-up, open circuit).
    """
    return gpio.input(config.SWITCH_PIN) == gpio.LOW

# ── GPIO setup ────────────────────────────────────────────────────────────────
def _setup_gpio():
    gpio.setmode(gpio.BCM)
    gpio.setwarnings(False)
    gpio.setup(config.SWITCH_PIN, gpio.IN,  pull_up_down=gpio.PUD_UP)
    gpio.setup(config.LED_PIN,    gpio.OUT, initial=gpio.LOW)

# ── Global flag for watchdog thread ──────────────────────────────────────────
_keep_running = True

# ── Main ──────────────────────────────────────────────────────────────────────
def main():
    global _keep_running

    _setup_gpio()
    _disable_sleep()
    _open_watchdog()

    # Start watchdog background thread
    wd_thread = threading.Thread(target=_watchdog_thread, daemon=True)
    wd_thread.start()

    log.info("Initialising hardware.")
    motors     = MotorController()
    ultrasonic = UltrasonicSensor()
    ir         = IRSensors()
    fsm        = SumoFSM(motors, ultrasonic, ir)

    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))

    log.info("Robot ready. Waiting for rocker switch to be turned ON.")

    # Blink LED slowly while waiting for switch to be flipped ON
    while not _switch_is_on():
        _led_blink(times=1, on_ms=200, off_ms=800)

    log.info("Switch ON — FSM starting with %ss start delay.", config.START_DELAY)

    try:
        while True:
            # ── Switch OFF check — highest priority, checked every tick ──────
            if not _switch_is_on():
                motors.stop()
                _led_off()
                log.info("Switch turned OFF — motors stopped. Pi still running.")

                # Wait here until switch is turned back ON
                while not _switch_is_on():
                    _led_blink(times=1, on_ms=200, off_ms=800)
                    _pet_watchdog()

                # Switch turned back ON — reset FSM from scratch
                log.info("Switch turned ON — restarting FSM.")
                fsm = SumoFSM(motors, ultrasonic, ir)

            # ── Normal FSM tick ───────────────────────────────────────────────
            t0 = time.monotonic()

            # LED behaviour reflects FSM state
            from strategy import State
            if fsm._state == State.START:
                # Blink during start delay to show countdown
                _led_blink(times=1, on_ms=100, off_ms=100)
            else:
                _led_on()

            fsm.tick()

            remaining = _TICK - (time.monotonic() - t0)
            if remaining > 0:
                time.sleep(remaining)

    except (KeyboardInterrupt, SystemExit):
        log.info("Shutdown signal received.")
    except Exception as exc:
        log.exception("Unhandled exception: %s", exc)
    finally:
        _keep_running = False
        motors.cleanup()
        _led_off()
        _close_watchdog()
        try:
            gpio.cleanup()
        except Exception:
            pass
        log.info("GPIO released. Robot stopped safely.")


if __name__ == "__main__":
    main()
