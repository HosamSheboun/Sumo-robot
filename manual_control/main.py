"""
main.py — Manual joystick control via Dabble Bluetooth app.

How it works:
  1. Pi advertises itself as Bluetooth device named config.BT_NAME ("SumoBot")
  2. Open Dabble app on your phone → connect to "SumoBot" → open Gamepad module
  3. Use the LEFT joystick to drive:
       Push forward  → both motors forward  (drive ahead)
       Push backward → both motors reverse  (drive back)
       Push left     → left motor slower    (turn left)
       Push right    → right motor slower   (turn right)
       Diagonal      → arc turns
  4. Rocker switch OFF → motors stop instantly, Pi stays on
  5. LED solid ON = connected and ready to drive
  6. LED OFF = waiting for Dabble to connect, or switch is OFF

Tank-drive mixing:
  left_motor_speed  = Y + X
  right_motor_speed = Y - X
  (Y = forward/back, X = left/right, both normalised -1.0 to +1.0)
"""

import time
import signal
import sys
import logging

import RPi.GPIO as gpio

import config
from motors    import MotorController
from bluetooth import DabbleBluetooth

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)
log = logging.getLogger(__name__)

_TICK = 1.0 / config.LOOP_HZ


# ── GPIO helpers ──────────────────────────────────────────────────────────────

def _setup_gpio():
    gpio.setmode(gpio.BCM)
    gpio.setwarnings(False)
    gpio.setup(config.SWITCH_PIN, gpio.IN,  pull_up_down=gpio.PUD_UP)
    gpio.setup(config.LED_PIN,    gpio.OUT, initial=gpio.LOW)


def _switch_is_on() -> bool:
    """True when rocker switch is in ON position (pin pulled LOW to GND)."""
    return gpio.input(config.SWITCH_PIN) == gpio.LOW


def _led_on():
    gpio.output(config.LED_PIN, gpio.HIGH)


def _led_off():
    gpio.output(config.LED_PIN, gpio.LOW)


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    _setup_gpio()

    motors = MotorController()
    bt     = DabbleBluetooth()

    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))

    log.info("Starting Bluetooth. Open Dabble app and connect to '%s'.",
             config.BT_NAME)
    bt.start()

    try:
        while True:
            t0 = time.monotonic()

            # ── Switch OFF → stop everything ──────────────────────────────────
            if not _switch_is_on():
                motors.stop()
                _led_off()
                log.info("Switch OFF — motors stopped.")
                while not _switch_is_on():
                    time.sleep(0.1)
                log.info("Switch ON — resuming.")

            # ── Waiting for Dabble to connect ─────────────────────────────────
            if not bt.is_connected():
                motors.stop()
                _led_off()
                # Blink slowly while waiting for phone to connect
                gpio.output(config.LED_PIN, gpio.HIGH); time.sleep(0.15)
                gpio.output(config.LED_PIN, gpio.LOW);  time.sleep(0.85)
                continue

            # ── Connected — drive from joystick ──────────────────────────────
            _led_on()

            x, y = bt.get_xy_normalised()

            # Both axes in deadzone → full stop
            if x == 0.0 and y == 0.0:
                motors.stop()
            else:
                motors.tank_mix(x, y)

            # ── Maintain loop rate ────────────────────────────────────────────
            elapsed = time.monotonic() - t0
            remaining = _TICK - elapsed
            if remaining > 0:
                time.sleep(remaining)

    except (KeyboardInterrupt, SystemExit):
        log.info("Shutdown signal received.")
    except Exception as exc:
        log.exception("Unhandled exception: %s", exc)
    finally:
        motors.stop()
        motors.cleanup()
        bt.stop()
        _led_off()
        try:
            gpio.cleanup()
        except Exception:
            pass
        log.info("Manual control stopped. GPIO released.")


if __name__ == "__main__":
    main()
