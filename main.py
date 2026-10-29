"""
main.py — Entry point for the autonomous sumo robot.

Wires all modules together and runs the FSM at a fixed LOOP_HZ rate.
Handles SIGTERM (systemd stop) and KeyboardInterrupt with clean GPIO release.
"""

import time
import signal
import sys
import logging

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


def main():
    log.info("Booting sumo robot — initialising hardware.")
    motors     = MotorController()
    ultrasonic = UltrasonicSensor()
    ir         = IRSensors()
    fsm        = SumoFSM(motors, ultrasonic, ir)

    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))

    log.info("FSM started — %s-second start delay in progress.", config.START_DELAY)
    try:
        while True:
            t0 = time.monotonic()
            fsm.tick()
            remaining = _TICK - (time.monotonic() - t0)
            if remaining > 0:
                time.sleep(remaining)
    except (KeyboardInterrupt, SystemExit):
        log.info("Shutdown signal received.")
    except Exception as exc:
        log.exception("Unhandled exception: %s", exc)
    finally:
        motors.cleanup()
        try:
            import RPi.GPIO as gpio
            gpio.cleanup()
        except Exception:
            pass
        log.info("GPIO released. Robot stopped safely.")


if __name__ == "__main__":
    main()
