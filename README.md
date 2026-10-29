# Pi Zero 2 W Autonomous Heavyweight Sumo Robot

Autonomous competitive sumo robot — ≤ 3 kg, ≤ 20 × 20 cm.
Built on a Raspberry Pi Zero 2 W with an adaptive priority FSM,
full quadrature encoder feedback, ultrasonic opponent tracking,
and dual downward IR boundary detection.

---

## Table of Contents
1. [Hardware Bill of Materials](#1-hardware-bill-of-materials)
2. [Power Distribution Architecture](#2-power-distribution-architecture)
3. [Wiring & Pinout Reference](#3-wiring--pinout-reference)
4. [HC-SR04 Echo Pin Protection](#4-hc-sr04-echo-pin-protection)
5. [Software Architecture](#5-software-architecture)
6. [FSM & Adaptive Algorithm](#6-fsm--adaptive-algorithm)
7. [Setup & Deployment](#7-setup--deployment)
8. [Run on Boot (systemd)](#8-run-on-boot-systemd)
9. [Tuning Guide](#9-tuning-guide)

---

## 1. Hardware Bill of Materials

| # | Component                                        | Qty | Notes                                    |
|---|--------------------------------------------------|-----|------------------------------------------|
| 1 | Raspberry Pi Zero 2 W                            | 1   | Main controller                          |
| 2 | GA25-370 DC Gear Motor w/ Encoder (12 V 130 RPM) | 2   | Left & right drive                       |
| 3 | Robot Wheel 65 mm × 26 mm + 4 mm metal coupler   | 2   | Drive wheels                             |
| 4 | Metal caster wheel                               | 1   | Rear support                             |
| 5 | L298N Dual H-Bridge Motor Driver Module          | 1   | Motor driver                             |
| 6 | 18650 3.7 V 1200 mAh button-top Li-ion cell      | 3   | In series → ~11.1 V nominal              |
| 7 | 3-slot 18650 series battery holder               | 1   | Battery pack                             |
| 8 | MT3608 DC-DC Boost Converter (HW-045)            | 1   | Boosts single cell 3.7 V → 5.0 V        |
| 9 | HC-SR04 Ultrasonic Sensor                        | 1   | Front opponent detection                 |
|10 | Downward IR Line Sensor (3-pin digital, active-LOW)| 2   | Front & back boundary detection only     |
|11 | 1 kΩ resistor                                    | 1   | HC-SR04 Echo voltage divider (series)    |
|12 | 2 kΩ resistor (or 2× 1 kΩ in series)            | 1   | HC-SR04 Echo voltage divider (to GND)    |

---

## 2. Power Distribution Architecture

### ⚠️ Critical — MT3608 Limitation
The MT3608 (HW-045) is a **boost (step-up) converter only**.
It **cannot** step 11.1 V down to 5 V.
**Solution:** Tap a **single cell** (3.7 V) from the pack as the MT3608 input.

```
3× 18650 in series (~11.1 V pack)
  │
  ├── Full pack (+) ──────────────────────────────► L298N 12V terminal
  │                                                  └──► Both GA25-370 motors
  │
  └── Single cell tap (3.7 V) ──► MT3608 VIN+ / VIN−
                                        │
                                   MT3608 VOUT+ / VOUT− (set to 5.0 V)
                                        │
                          ┌─────────────┼──────────────────┐
                          ▼             ▼                   ▼
                   Pi Zero 2 W    HC-SR04 VCC     Both IR sensors VCC
                   Pin 2 (5V)                     Both encoder VCC (green)

Common GND rail:
  Battery (−) ── L298N GND ── MT3608 VIN− ── MT3608 VOUT− ── Pi GND ── All sensor GNDs
```

### Single Cell Tap Wiring

Your 3-slot holder has 4 metal contact points along the cell chain:

```
[−] Cell 1 [+]─[−] Cell 2 [+]─[−] Cell 3 [+]
 │                    │                      │
GND (~0 V)       ~3.7 V tap            ~11.1 V (+)
(common GND)    (MT3608 VIN+)          (L298N 12V)
```

- **MT3608 VIN+** → junction between cell 1 (+) and cell 2 (−) — reads ~3.7 V
- **MT3608 VIN−** → battery pack (−) / common GND rail

> ⚠️ The single cell (~1200 mAh) powers the Pi and all sensors continuously.
> It will drain faster than the other two cells. Balance-check it before every
> match. Optionally add a **1N5819 Schottky diode** in series with VIN+ to
> prevent reverse current if that cell dips below the adjacent tap voltage.

### MT3608 Output Adjustment
1. Connect MT3608 VIN+/VIN− to the single cell tap only — **nothing else connected yet**.
2. Place a multimeter on VOUT+ / VOUT−.
3. Turn the trimmer potentiometer **clockwise slowly** until output reads exactly **5.00 V**.
4. Double-check the reading. Never exceed **5.1 V** — the Pi is permanently damaged above this.
5. Only after confirming 5.00 V, connect VOUT+ to Pi Pin 2 and all sensor VCC lines.

> ⚠️ Some MT3608 HW-045 boards deliver only ~0.5 A continuous despite the
> "2 A" label. If the Pi resets when motors start, solder a **1000 µF / 10 V**
> electrolytic capacitor across VOUT+ / VOUT− to absorb motor-start transients.

---

## 3. Wiring & Pinout Reference

### Pi Zero 2 W GPIO (BCM numbering)

```
Physical Pin │ BCM  │ Connected To
─────────────┼──────┼────────────────────────────────────────────
Pin  2 (5V)  │  —   │ MT3608 VOUT+ (5.0 V regulated)
Pin  6 (GND) │  —   │ Common GND rail
Pin 11       │ BCM17│ L298N IN1   (Left motor direction A)
Pin 13       │ BCM27│ L298N IN2   (Left motor direction B)
Pin 12       │ BCM18│ L298N ENA   (Left motor PWM)   ← HW-PWM ch0
Pin 15       │ BCM22│ L298N IN3   (Right motor direction A)
Pin 16       │ BCM23│ L298N IN4   (Right motor direction B)
Pin 33       │ BCM13│ L298N ENB   (Right motor PWM)  ← HW-PWM ch1
Pin 29       │ BCM 5│ Left encoder  Channel A (yellow wire)
Pin 31       │ BCM 6│ Left encoder  Channel B (white wire)
Pin 35       │ BCM19│ Right encoder Channel A (yellow wire)
Pin 37       │ BCM26│ Right encoder Channel B (white wire)
Pin 18       │ BCM24│ HC-SR04 TRIG
Pin 22       │ BCM25│ HC-SR04 ECHO (via 1kΩ/2kΩ divider — see §4)
Pin 32       │ BCM12│ Front downward IR line sensor OUT  (edge detection)
Pin 36       │ BCM16│ Back  downward IR line sensor OUT  (edge detection)
```

### GA25-370 Motor Wire Colours

```
Wire Colour │ Function          │ Connect To
────────────┼───────────────────┼──────────────────────────────
Red         │ Motor terminal +  │ L298N OUT1 (left) / OUT3 (right)
Black       │ Motor terminal −  │ L298N OUT2 (left) / OUT4 (right)
Green       │ Encoder VCC 5 V   │ MT3608 5 V rail
Dark Blue   │ Encoder GND       │ Common GND rail
Yellow      │ Encoder Channel A │ Pi BCM5 (left) / BCM19 (right)
White       │ Encoder Channel B │ Pi BCM6 (left) / BCM26 (right)
```

### L298N Module

```
L298N Terminal │ Connected To
───────────────┼──────────────────────────────────────
12V            │ Battery pack (+) ~11.1 V
GND            │ Common GND rail
5V (output)    │ NOT USED — leave unconnected
IN1            │ Pi BCM17
IN2            │ Pi BCM27
IN3            │ Pi BCM22
IN4            │ Pi BCM23
ENA            │ Pi BCM18  (remove ENA jumper)
ENB            │ Pi BCM13  (remove ENB jumper)
OUT1 / OUT2    │ Left  motor red / black wires
OUT3 / OUT4    │ Right motor red / black wires
```

> **Remove the ENA and ENB jumpers** on the L298N board.
> With jumpers in place the motors run at full speed only — PWM has no effect.

---

## 4. HC-SR04 Echo Pin Protection

The HC-SR04 Echo pin outputs **5 V** logic. The Pi GPIO is **3.3 V max**.
Use a resistor voltage divider:

```
HC-SR04 ECHO ──── 1 kΩ ────┬──► Pi BCM25
                            │
                           2 kΩ
                            │
                           GND

Voltage at Pi = 5 V × 2kΩ/(1kΩ+2kΩ) = 3.33 V  ✓
```

Use 1% tolerance resistors. Optionally add a 3.3 V Zener diode across the 2 kΩ
resistor for additional over-voltage clamping.

---

## 5. Software Architecture

```
main.py          ← Fixed-rate FSM loop at LOOP_HZ; clean shutdown
  │
  ├── config.py  ← All pins, speeds, thresholds (edit only this file)
  ├── motors.py  ← MotorController + quadrature _Encoder (A+B channels)
  ├── sensors.py ← UltrasonicSensor (median filter), IRSensors, OpponentTracker
  └── strategy.py← SumoFSM + TacticAdaptor + _ScanTracker
```

---

## 6. FSM & Adaptive Algorithm

### State Priority (highest → lowest)

```
EDGE_AVOID  ← white line detected (always overrides)
    │
    RAM     ← opponent < CLOSE_DISTANCE_CM (burst push)
    │
    ATTACK  ← opponent < ATTACK_DISTANCE_CM (full charge + heading correction)
    │
    STALK   ← opponent visible but far (arc approach)
    │
    ORBIT   ← flanking arc (FLANKER tactic only)
    │
    SEARCH  ← no opponent; 360° spin-scan
    │
    START   ← 5-second mandatory delay
```

### TacticAdaptor — In-Match Style Switching

| Style   | Behaviour                                      | Triggered When                        |
|---------|------------------------------------------------|---------------------------------------|
| RUSHER  | Charge immediately at full speed               | Default; push successes ≥ 2           |
| STALKER | Arc toward opponent carefully                  | Moderate edge pressure                |
| FLANKER | Orbit wide, attack from the side               | Edge hits ≥ 3 (being pushed around)   |
| RUSHER  | Force-override regardless of style             | Match time > 150 seconds              |

### OpponentTracker
Maintains a confidence score (0–1) and estimated bearing to the opponent.
When the ultrasonic sensor loses the target, confidence decays over
`LOST_TIMEOUT` seconds. The bearing estimate rotates with the robot's spin
so the FSM knows which direction to search first.

---

## 7. Setup & Deployment

### 7.1 Flash OS
Use Raspberry Pi Imager → **Raspberry Pi OS Lite (64-bit)**.
In advanced settings: set hostname, enable SSH, configure Wi-Fi.

### 7.2 First Boot
```bash
ssh pi@<robot-ip>
sudo apt update && sudo apt full-upgrade -y
sudo apt install -y python3-pip python3-venv git
```

### 7.3 Enable Interfaces
```bash
sudo raspi-config
# Interface Options → I2C: Enable
# Interface Options → SPI: Enable
# Performance → GPU Memory: 16 MB
# Finish → Reboot
```

### 7.4 Install Project
```bash
git clone https://github.com/<your-username>/sumo-robot.git
cd sumo-robot
python3 -m venv .venv
source .venv/bin/activate
pip install RPi.GPIO
```

### 7.5 Hardware Tests
```bash
# Motor test — forward 2 s
python3 - <<'EOF'
import time
from motors import MotorController
m = MotorController()
m.forward(50); time.sleep(2); m.cleanup()
EOF

# Encoder test — spin and read ticks
python3 - <<'EOF'
import time
from motors import MotorController
m = MotorController()
m.forward(50); time.sleep(1); m.stop()
print(f"Left ticks: {m.left_enc.get_ticks()}, Right ticks: {m.right_enc.get_ticks()}")
m.cleanup()
EOF

# Sensor test
python3 - <<'EOF'
from sensors import UltrasonicSensor, IRSensors
us = UltrasonicSensor(); ir = IRSensors()
print(f"Distance : {us.distance_cm():.1f} cm")
print(f"Edge front: {ir.edge_front()}, Edge back: {ir.edge_back()}")
EOF
```

---

## 8. Run on Boot (systemd)

```bash
sudo nano /etc/systemd/system/sumo.service
```

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
sudo journalctl -u sumo.service -f
```

---

## 9. Tuning Guide

| Parameter            | File       | Effect of increasing                              |
|----------------------|------------|---------------------------------------------------|
| `ATTACK_DISTANCE_CM` | config.py  | Charges from further away                         |
| `STALK_DISTANCE_CM`  | config.py  | Begins tracking from further away                 |
| `SPEED_FULL`         | config.py  | Faster attack (watch motor current)               |
| `SPEED_SEARCH`       | config.py  | Faster scan (may overshoot)                       |
| `REVERSE_DURATION`   | config.py  | Moves further from edge before spinning           |
| `SPIN_DURATION`      | config.py  | Rotates more before resuming search               |
| `SCAN_STEP_TIME`     | config.py  | Longer pause per scan step (slower but thorough)  |
| `LOST_TIMEOUT`       | config.py  | Tracks opponent longer before giving up           |
| `LOOP_HZ`            | config.py  | Higher = faster reaction (more CPU load)          |

### Motor Direction Correction
If a motor spins the wrong direction, **swap the red and black wires**
on that motor's L298N output terminals. Do not change software logic.

### Edge Sensor Calibration
Each IR sensor has an onboard potentiometer. Adjust it until:
- LED **ON** over the black dohyo surface
- LED **OFF** over the white boundary ring
Test on the actual competition surface before the match.

### Encoder Direction Verification
Run the encoder test above. Drive forward — both tick counts should increase.
If one decreases, swap that motor's red/black wires on the L298N.

---

## License
MIT — free to use, modify, and distribute for educational and competition purposes.
