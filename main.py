"""
main.py — Entry point for the autonomous sumo robot.

Instantiates all hardware abstractions, starts the FSM, and runs the
control loop at LOOP_HZ until interrupted (Ctrl-C or SIGTERM).
GPIO cleanup is guaranteed via try/finally.
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

_TICK_PERIOD = 1.0 / config.LOOP_HZ   # seconds per FSM tick


def main():
    log.info("Initialising hardware…")
    motors     = MotorController()
    ultrasonic = UltrasonicSensor()
    ir         = IRSensors()
    fsm        = SumoFSM(motors, ultrasonic, ir)

    # Allow SIGTERM (e.g. systemd stop) to trigger the same clean exit path
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))

    log.info("Starting FSM — mandatory %s-second start delay.", config.START_DELAY)
    try:
        while True:
            t0 = time.monotonic()
            fsm.tick()
            # Sleep for the remainder of the tick period to maintain LOOP_HZ
            elapsed = time.monotonic() - t0
            sleep_time = _TICK_PERIOD - elapsed
            if sleep_time > 0:
                time.sleep(sleep_time)

    except KeyboardInterrupt:
        log.info("KeyboardInterrupt received — shutting down.")
    except Exception as exc:
        log.exception("Unhandled exception in main loop: %s", exc)
    finally:
        motors.cleanup()
        # RPi.GPIO cleanup is handled inside MotorController.cleanup();
        # call the global cleanup here to release sensor pins as well.
        try:
            import RPi.GPIO as gpio
            gpio.cleanup()
        except Exception:
            pass
        log.info("GPIO released. Robot stopped.")


if __name__ == "__main__":
    main()
