# Pi Zero 2 W Autonomous Mini-Sumo Robot

A production-grade autonomous competitive mini-sumo robot built on a Raspberry Pi Zero 2 W,
featuring a priority-based Finite State Machine, encoder-assisted motor control, ultrasonic
opponent detection, and dual IR edge/boundary detection.

---

## Table of Contents
1. [Hardware Bill of Materials](#1-hardware-bill-of-materials)
2. [Power Distribution Architecture](#2-power-distribution-architecture)
3. [Wiring & Pinout Reference](#3-wiring--pinout-reference)
4. [HC-SR04 Echo Pin Protection](#4-hc-sr04-echo-pin-protection)
5. [Software Architecture](#5-software-architecture)
6. [Setup & Deployment](#6-setup--deployment)
7. [Run on Boot (systemd)](#7-run-on-boot-systemd)
8. [Tuning Guide](#8-tuning-guide)

---

## 1. Hardware Bill of Materials

| # | Component                                      | Qty | Notes                                  |
|---|------------------------------------------------|-----|----------------------------------------|
| 1 | Raspberry Pi Zero 2 W                          | 1   | Main controller                        |
| 2 | GA25-370 DC Gear Motor w/ Encoder (12 V 130 RPM)| 2  | Left & right drive                     |
| 3 | Robot Wheel 65 mm × 26 mm + 4 mm metal coupler | 2   | Drive wheels                           |
| 4 | Metal caster wheel                             | 1   | Rear support                           |
| 5 | L298N Dual H-Bridge Motor Driver Module        | 1   | Motor driver                           |
| 6 | 18650 3.7 V 1200 mAh button-top Li-ion cell    | 3   | In series → ~11.1 V nominal            |
| 7 | 3-slot 18650 series battery holder             | 1   | Battery pack                           |
| 8 | MT3608 DC-DC Boost Converter                   | 1   | Boost to 5 V for Pi & sensors          |
| 9 | HC-SR04 Ultrasonic Sensor                      | 1   | Opponent detection                     |
|10 | IR Obstacle Avoidance Sensor (3-pin digital)   | 1   | Short-range front obstacle             |
|11 | Downward IR Line Sensor (3-pin digital)        | 2   | Left & right edge/boundary detection   |
|12 | 1 kΩ resistor                                  | 1   | HC-SR04 Echo voltage divider (series)  |
|13 | 2 kΩ resistor (or 2× 1 kΩ in series)          | 1   | HC-SR04 Echo voltage divider (to GND)  |

---

## 2. Power Distribution Architecture

```
3× 18650 in series
  └─ ~11.1 V (9.6 V min / 12.6 V max)
       │
       ├──► L298N "12V" VCC input  ──► powers both GA25-370 motors directly
       │
       └──► MT3608 Boost Converter IN
                 │
                 └──► MT3608 OUT set to 5.0 V
                           │
                           ├──► Raspberry Pi Zero 2 W  5V pin (Pin 2 or 4)
                           ├──► HC-SR04 VCC
                           ├──► IR Obstacle Sensor VCC
                           └──► IR Edge Sensor VCC (×2)

L298N 5V output pin  ──► DO NOT USE for Pi power (noisy, unreliable under load)
                         Use the MT3608 output exclusively.

Common GND rail: Battery (–) ── L298N GND ── MT3608 GND ── Pi GND ── All sensor GNDs
```

### MT3608 Output Voltage Adjustment
1. Connect the MT3608 input to the battery pack (11.1 V nominal).
2. With a multimeter on the output, turn the onboard trimmer potentiometer
   clockwise until the output reads exactly **5.00 V** before connecting the Pi.
3. Never exceed 5.1 V on the Pi's 5 V rail.

---

## 3. Wiring & Pinout Reference

### Pi Zero 2 W GPIO Header (BCM numbering)

```
Pi Physical Pin │ BCM  │ Connected To
────────────────┼──────┼──────────────────────────────────────────
Pin  2 (5V)     │  —   │ MT3608 5 V output (+)
Pin  6 (GND)    │  —   │ Common GND rail
Pin 11          │ BCM17│ L298N IN1  (Left motor direction A)
Pin 13          │ BCM27│ L298N IN2  (Left motor direction B)
Pin 12          │ BCM18│ L298N ENA  (Left motor PWM)  ← HW PWM
Pin 15          │ BCM22│ L298N IN3  (Right motor direction A)
Pin 16          │ BCM23│ L298N IN4  (Right motor direction B)
Pin 33          │ BCM13│ L298N ENB  (Right motor PWM) ← HW PWM
Pin 29          │ BCM 5│ Left encoder channel A
Pin 31          │ BCM 6│ Right encoder channel A
Pin 18          │ BCM24│ HC-SR04 TRIG
Pin 22          │ BCM25│ HC-SR04 ECHO (via 1kΩ/2kΩ divider — see §4)
Pin 32          │ BCM12│ IR Obstacle Sensor OUT
Pin 35          │ BCM19│ Left Edge IR Sensor OUT
Pin 37          │ BCM26│ Right Edge IR Sensor OUT
```

### L298N Module Wiring

```
L298N Terminal │ Connected To
───────────────┼──────────────────────────────────────
12V            │ Battery pack (+) ~11.1 V
GND            │ Common GND rail
5V (output)    │ NOT USED (leave unconnected)
IN1            │ Pi BCM17
IN2            │ Pi BCM27
IN3            │ Pi BCM22
IN4            │ Pi BCM23
ENA            │ Pi BCM18  (jumper removed)
ENB            │ Pi BCM13  (jumper removed)
OUT1 / OUT2    │ Left GA25-370 motor terminals
OUT3 / OUT4    │ Right GA25-370 motor terminals
```

> **Important:** Remove the ENA and ENB jumpers on the L298N board so that
> PWM signals from the Pi control motor speed. With jumpers installed, motors
> run at full speed only.

---

## 4. HC-SR04 Echo Pin Protection

The HC-SR04 Echo pin outputs **5 V** logic, which will damage the Pi's 3.3 V GPIO.
Use a simple resistor voltage divider:

```
HC-SR04 ECHO ──┬── 1 kΩ ──► Pi BCM25 (ECHO_PIN)
               │
              2 kΩ
               │
              GND

Output voltage = 5 V × (2kΩ / (1kΩ + 2kΩ)) = 3.33 V  ✓
```

Use 1% tolerance resistors for accuracy. A 3.3 V Zener diode in parallel with
the 2 kΩ resistor provides additional over-voltage clamping.

---

## 5. Software Architecture

```
main.py          ← Entry point; fixed-rate FSM loop at LOOP_HZ
  │
  ├── config.py  ← All pin numbers, speeds, and thresholds (edit here only)
  ├── motors.py  ← MotorController: PWM + direction + encoder interrupts
  ├── sensors.py ← UltrasonicSensor, IRSensors (non-blocking reads)
  └── strategy.py← SumoFSM: START → SEARCH ↔ ATTACK, EDGE_AVOID (override)
```

### FSM State Diagram

```
         ┌─────────────────────────────────────────────┐
         │  any_edge() == True (HIGHEST PRIORITY)       │
         ▼                                              │
[START] ──5s──► [SEARCH] ◄──────────────── [ATTACK]   │
                   │  ▲                       │  ▲     │
          opponent │  │ opponent lost         │  │     │
          detected │  └───────────────────────┘  │     │
                   └──────────────────────────────┘     │
                                                        │
                   [EDGE_AVOID] ──avoidance done──► [SEARCH]
```

### Module Responsibilities

| Module        | Responsibility                                                  |
|---------------|-----------------------------------------------------------------|
| `config.py`   | Single source of truth for all hardware constants               |
| `motors.py`   | L298N PWM control; encoder tick counting via GPIO interrupts    |
| `sensors.py`  | Non-blocking HC-SR04 ranging; digital IR reads                  |
| `strategy.py` | Priority FSM; all match logic isolated from hardware details    |
| `main.py`     | Wires modules together; fixed-rate loop; clean shutdown         |

---

## 6. Setup & Deployment

### 6.1 Flash Raspberry Pi OS Lite (64-bit)
```bash
# Use Raspberry Pi Imager → choose "Raspberry Pi OS Lite (64-bit)"
# In advanced settings: set hostname, enable SSH, configure Wi-Fi
```

### 6.2 First Boot — System Update
```bash
ssh pi@<robot-ip>
sudo apt update && sudo apt full-upgrade -y
sudo apt install -y python3-pip python3-venv git
```

### 6.3 Enable Required Interfaces
```bash
sudo raspi-config
# → Interface Options → Legacy Camera: Disable
# → Interface Options → I2C: Enable  (future expansion)
# → Interface Options → SPI: Enable  (future expansion)
# → Performance → GPU Memory: set to 16 MB (headless)
# → Finish → Reboot
```

### 6.4 Clone & Install Python Dependencies
```bash
git clone https://github.com/<your-username>/sumo-robot.git
cd sumo-robot
python3 -m venv .venv
source .venv/bin/activate
pip install RPi.GPIO
```

> `RPi.GPIO` is the only runtime dependency. It is pre-installed on Raspberry Pi OS;
> the venv install ensures a pinned version.

### 6.5 Test Hardware Before a Match
```bash
# Quick motor test (runs forward 2 s then stops)
python3 - <<'EOF'
import time, config
from motors import MotorController
m = MotorController()
m.forward(50); time.sleep(2); m.cleanup()
EOF

# Quick sensor test
python3 - <<'EOF'
from sensors import UltrasonicSensor, IRSensors
us = UltrasonicSensor(); ir = IRSensors()
print(f"Distance: {us.distance_cm():.1f} cm")
print(f"Obstacle: {ir.obstacle_detected()}, Edge L: {ir.edge_left()}, Edge R: {ir.edge_right()}")
EOF
```

---

## 7. Run on Boot (systemd)

Create a service file so the robot starts autonomously when powered on:

```bash
sudo nano /etc/systemd/system/sumo.service
```

Paste the following (adjust paths if needed):

```ini
[Unit]
Description=Sumo Robot FSM
After=multi-user.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/sumo-robot
ExecStart=/home/pi/sumo-robot/.venv/bin/python main.py
Restart=on-failure
RestartSec=3

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable sumo.service
sudo systemctl start  sumo.service
sudo journalctl -u sumo.service -f   # live log monitoring
```

To disable auto-start (e.g. for development):
```bash
sudo systemctl disable sumo.service
```

---

## 8. Tuning Guide

| Parameter              | File        | Effect of increasing value                          |
|------------------------|-------------|-----------------------------------------------------|
| `ATTACK_DISTANCE_CM`   | config.py   | Robot charges from further away (more aggressive)   |
| `SPEED_FULL`           | config.py   | Faster attack charge (watch motor current draw)     |
| `SPEED_SEARCH`         | config.py   | Faster scan rotation (may overshoot opponent)       |
| `REVERSE_DURATION`     | config.py   | Moves further from edge before spinning             |
| `SPIN_DURATION`        | config.py   | Rotates more before resuming search                 |
| `LOOP_HZ`              | config.py   | Higher = faster sensor reaction (CPU load increases)|

### Motor Direction Correction
If a motor spins the wrong direction, swap the two wires on that motor's L298N
output terminals (OUT1/OUT2 or OUT3/OUT4). Do **not** change the IN1/IN2 logic
in software — physical swap is safer and more reliable.

### Edge Sensor Sensitivity
Each downward IR sensor has an onboard potentiometer. Adjust it until the LED
indicator turns ON over the black ring surface and OFF over the white boundary
line. Test on the actual competition dohyo surface before a match.

---

## License
MIT — free to use, modify, and distribute for educational and competition purposes.
