# Pi Autonomous — Raspberry Pi Zero 2 W Sumo Robot

This folder contains the full autonomous sumo robot code for the Raspberry Pi Zero 2 W.
This is the most capable version — it uses a 7-state adaptive FSM, encoder feedback,
opponent tracking, edge avoidance, and a tactic system that changes fighting style
based on what's happening in the match.

No remote control. Flip the switch, put it on the ring, it fights on its own.

---

## Files in This Folder

```
config.py    ← Every pin number, speed, distance, and timing constant.
               This is the ONLY file you edit to tune the robot.
               Never hardcode numbers in any other file.

main.py      ← Entry point. Starts everything, runs the 50Hz loop.
               Handles the rocker switch, LED state machine,
               hardware watchdog (/dev/watchdog), and SIGTERM shutdown.

motors.py    ← Controls both GA25-370 motors through the L298N H-bridge.
               Also reads both quadrature encoders (A+B channels).
               Exposes: forward, reverse, spin_left, spin_right,
               arc_left, arc_right, stop, heading_error(), avg_velocity().

sensors.py   ← Reads all three sensors:
               - UltrasonicSensor: HC-SR04 with 4-sample median filter.
                 Returns 999.0cm on timeout (nothing in range).
               - IRSensors: front and back downward IR line sensors.
                 Active-LOW — reads LOW when white boundary detected.
               - OpponentTracker: confidence-based position estimator.
                 Tracks bearing even when sensor loses the opponent.

strategy.py  ← The brain. 7-state priority FSM + TacticAdaptor.
               Never touches hardware directly — only calls motors/sensors.

setup_pi.sh  ← Run this ONCE after first boot on a fresh Pi.
               Enables hardware watchdog, sets performance CPU governor,
               disables Wi-Fi power saving, disables swap.
```

---

## How the FSM Works

Every 50Hz tick, the FSM checks all 7 states in priority order.
The highest-priority state that applies wins.

```
Priority │ State       │ Condition                        │ Action
─────────┼─────────────┼──────────────────────────────────┼──────────────────────────
1 (high) │ EDGE_AVOID  │ IR sensor sees white line        │ Reverse + spin away
2        │ RAM         │ Opponent < 25cm                  │ Full-speed burst push
3        │ ATTACK      │ Opponent < 80cm                  │ Charge with heading fix
4        │ STALK       │ Opponent 80–150cm                │ Arc toward bearing
5        │ ORBIT       │ FLANKER tactic active            │ Wide arc around opponent
6        │ SEARCH      │ Nothing detected                 │ Spin-scan 360° then reverse
7 (low)  │ START       │ First 5 seconds after switch ON  │ Sit still (competition rule)
```

EDGE_AVOID always wins because staying in the ring is more important than anything else.
Even if the opponent is right in front of you, if you're about to fall off you reverse first.

---

## How the Tactic System Works

The TacticAdaptor watches what's happening and switches fighting style automatically.

```
Tactic    │ Behaviour                              │ Triggered When
──────────┼────────────────────────────────────────┼──────────────────────────────
RUSHER    │ Charge immediately at full speed        │ Default. Also if 2+ charges worked
STALKER   │ Careful curved approach                 │ If pushed to edge once
FLANKER   │ Wide orbit then side attack             │ If pushed to edge 3+ times
RUSHER    │ Ignore everything, charge               │ Final 30 seconds (forced)
```

---

## How the Opponent Tracker Works

The OpponentTracker keeps a confidence score (0.0 to 1.0) for how sure it is
about the opponent's position.

- Opponent detected → confidence = 1.0, bearing = 0° (directly ahead)
- Opponent lost → confidence decays over LOST_TIMEOUT seconds
- Robot spins → tracker rotates its bearing estimate by the same angle
- Confidence < 0.1 → switch to SEARCH state

This means the robot doesn't start searching from scratch every time it loses
sight of the opponent — it already has a rough idea of which direction to look.

---

## How the Encoders Work

Each GA25-370 motor has a quadrature encoder (channels A and B).
The code reads both channels to determine direction and count ticks.

- Channel A rising edge → read Channel B
- Channel B HIGH when A rises → forward (+1 tick)
- Channel B LOW when A rises → reverse (−1 tick)

The heading_error() function returns (left_ticks − right_ticks).
If positive, the robot is drifting right → reduce left motor speed slightly.
If negative, drifting left → reduce right motor speed slightly.
This keeps the robot driving straight during a charge.

---

## Wiring Summary

```
BCM Pin │ Connected To
────────┼──────────────────────────────────────────────────────
BCM 17  │ L298N IN1   — left motor direction A
BCM 18  │ L298N ENA   — left motor PWM speed
BCM 27  │ L298N IN2   — left motor direction B
BCM 22  │ L298N IN3   — right motor direction A
BCM 23  │ L298N IN4   — right motor direction B
BCM 24  │ HC-SR04 TRIG
BCM 25  │ HC-SR04 ECHO (via 1kΩ + 2kΩ divider → 3.33V)
BCM  5  │ Left encoder Channel A (yellow wire)
BCM  6  │ Left encoder Channel B (white wire)
BCM 12  │ Front IR sensor OUT
BCM 13  │ L298N ENB   — right motor PWM speed
BCM 19  │ Right encoder Channel A (yellow wire)
BCM 16  │ Back IR sensor OUT
BCM 26  │ Right encoder Channel B (white wire)
BCM 20  │ LED anode via 330Ω resistor → GND
BCM 21  │ Rocker switch (other pin → GND)
Pin  2  │ MT3608 VOUT+ 5.0V (power IN to Pi)
Pin  6  │ Common GND rail
```

---

## Setup and Running

```bash
# One-time Pi setup (run once after first boot)
bash setup_pi.sh
sudo reboot

# Run manually
cd pi_autonomous
source ../.venv/bin/activate
python main.py

# Run as a systemd service (auto-starts on power-up)
# See the root README.md for the full systemd service file
```

---

## Tuning

Edit `config.py` only. Key constants:

| Constant | Default | What It Does |
|----------|---------|-------------|
| `ATTACK_DISTANCE_CM` | 80.0 | Switch from STALK to ATTACK |
| `CLOSE_DISTANCE_CM` | 25.0 | Switch from ATTACK to RAM |
| `STALK_DISTANCE_CM` | 150.0 | Switch from SEARCH to STALK |
| `SPEED_FULL` | 100 | Attack charge speed (%) |
| `SPEED_SEARCH` | 55 | Spin speed during search (%) |
| `START_DELAY` | 5.0 | Pre-match pause (seconds) |
| `LOST_TIMEOUT` | 1.2 | Seconds before opponent considered lost |
| `LOOP_HZ` | 50 | FSM tick rate |
