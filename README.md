# Pi Zero 2 W Autonomous Heavyweight Sumo Robot

A fully autonomous 3 kg sumo robot built around a Raspberry Pi Zero 2 W.
No remote control. Once the switch is flipped, it thinks and fights entirely on its own.

---

## Project Structure

```
Sumo robot/
│
├── config.py              ← Pi autonomous: all BCM pins and tuning constants
├── main.py                ← Pi autonomous: entry point, switch, LED, watchdog
├── motors.py              ← Pi autonomous: L298N + quadrature encoder control
├── sensors.py             ← Pi autonomous: HC-SR04, IR sensors, OpponentTracker
├── strategy.py            ← Pi autonomous: 7-state adaptive FSM + TacticAdaptor
├── setup_pi.sh            ← One-time Pi setup: watchdog, performance governor, no-sleep
├── README.md              ← This file
│
├── arduino_autonomous/    ← Standalone Arduino Uno version (no Pi needed)
│   ├── arduino_autonomous.ino  ← Main sketch: 2-minute technique timeline
│   ├── config.h                ← All pins and timing constants
│   ├── motors.h / motors.cpp   ← L298N motor control (7 movement functions)
│   └── README.md               ← Full wiring guide and tuning reference
│
├── manual_control/        ← Standalone Pi manual drive via Dabble Bluetooth app
│   ├── main.py            ← Entry point: switch, LED, joystick loop
│   ├── motors.py          ← L298N + encoder control with tank-mix
│   ├── bluetooth.py       ← Dabble SPP packet parser, RFCOMM socket server
│   ├── config.py          ← Pins, joystick settings, BT name
│   └── README.md          ← Pairing guide, driving controls, systemd setup
│
└── Sumo_Robot_V2_3D_files/  ← 3D-printed chassis design (OpenSCAD + STL)
    ├── Sumo_Box_Concept_v2.scad      ← Parametric source (edit this)
    ├── Sumo_Box_Chassis_v2.stl       ← Main enclosure body
    ├── Sumo_Box_Lid_v2.stl           ← Removable lid
    ├── Sumo_Electronics_Tray_v2.stl  ← Internal electronics deck
    ├── Sumo_Front_Blade_v2.stl       ← Front wedge/blade
    ├── README_Sumo_Box_v2.md         ← Print settings, assembly notes
    └── *.png                         ← Render previews
```

---

## Which Version Should I Use?

| Version | Controller | What It Does | When to Use |
|---------|-----------|-------------|-------------|
| **Pi Autonomous** (root folder) | Raspberry Pi Zero 2 W | Full 7-state adaptive FSM, encoder feedback, opponent tracking, edge avoidance | Competition — most capable |
| **Arduino Autonomous** (`arduino_autonomous/`) | Arduino Uno | 2-minute technique timeline: Matador → Bulldozer → Flanker → Berserker | Simpler build, no Pi needed |
| **Manual Control** (`manual_control/`) | Raspberry Pi Zero 2 W | Drive with Dabble Bluetooth joystick app on your phone | Testing, demos, practice |

> Each folder is completely standalone. They do not share any code or imports.

---

## Hardware Overview

### Robot Specs
- Weight limit: ≤ 3 kg
- Size limit: ≤ 20 × 20 cm
- Arena: black circular dohyo with white boundary ring

### Parts List

| # | Part | Qty | Used In |
|---|------|-----|---------|
| 1 | Raspberry Pi Zero 2 W | 1 | Pi versions |
| 2 | Arduino Uno (ATmega328P) | 1 | Arduino version |
| 3 | GA25-370 DC Gear Motor with Encoder (12V, 130RPM) | 2 | All versions |
| 4 | Robot Wheel 65mm × 26mm + 4mm metal coupler | 2 | All versions |
| 5 | Metal caster wheel | 1 | All versions |
| 6 | L298N Dual H-Bridge Motor Driver Module | 1 | All versions |
| 7 | 18650 3.7V 1200mAh button-top Li-ion battery | 3 | All versions |
| 8 | 3-slot 18650 series battery holder | 1 | All versions |
| 9 | MT3608 DC-DC Boost Converter (HW-045) | 1 | Pi versions only |
| 10 | HC-SR04 Ultrasonic Sensor | 1 | All versions |
| 11 | Downward IR Line Sensor (3-pin digital) | 2 | Pi versions only |
| 12 | Black Rocker Switch (2-pin ON/OFF) | 1 | All versions |
| 13 | LED (any colour, 3mm or 5mm) | 1 | All versions |
| 14 | 330Ω resistor | 1 | All versions |
| 15 | 1kΩ resistor | 1 | Pi versions only |
| 16 | 2kΩ resistor | 1 | Pi versions only |

---

## Power Architecture

### Pi Versions (Autonomous + Manual Control)

```
3× 18650 in series (~11.1V)
         │
         ├──────────────────────────────────► L298N 12V terminal → Both motors
         │
         └── Single cell tap (~3.7V) ────────► MT3608 VIN+
              Battery (−) ──────────────────► MT3608 VIN−
                                               │
                                        MT3608 VOUT+ = 5.0V
                                               │
                          ┌────────────────────┼──────────────────────┐
                          ▼                    ▼                      ▼
                   Pi Zero 2 W Pin 2     HC-SR04 VCC          Both IR sensors VCC
                                                               Both encoder green wires

Common GND: Battery(−) ── L298N GND ── MT3608 VOUT− ── Pi GND ── All sensor GNDs
```

> ⚠️ The MT3608 is boost-only (step-up). You MUST tap a single cell (~3.7V),
> not the full pack (11.1V). Set output to exactly 5.00V before connecting the Pi.
> Verify with a multimeter. More than 5.1V will permanently destroy the Pi.

### Arduino Version

```
3× 18650 in series (~11.1V)
         │
         ├──────────────────────────────────► L298N 12V terminal → Both motors
         │
         └──────────────────────────────────► Arduino Uno VIN pin
                                               └──► Uno onboard regulator → 5V
                                                    ├──► Uno logic
                                                    ├──► HC-SR04 VCC
                                                    └──► Encoder green wires

Common GND: Battery(−) ── L298N GND ── Uno GND ── HC-SR04 GND ── Encoder dark blue wires
```

---

## Pin Reference

### Pi Zero 2 W — BCM Pin Assignments (all Pi versions)

```
BCM Pin │ Physical │ Connected To
────────┼──────────┼──────────────────────────────────────────────────────
BCM 17  │ Pin 11   │ L298N IN1   — left motor direction A
BCM 18  │ Pin 12   │ L298N ENA   — left motor PWM speed
BCM 27  │ Pin 13   │ L298N IN2   — left motor direction B
BCM 22  │ Pin 15   │ L298N IN3   — right motor direction A
BCM 23  │ Pin 16   │ L298N IN4   — right motor direction B
BCM 24  │ Pin 18   │ HC-SR04 TRIG
BCM 25  │ Pin 22   │ HC-SR04 ECHO (via 1kΩ + 2kΩ resistor divider → 3.33V)
BCM  5  │ Pin 29   │ Left encoder Channel A (yellow wire)
BCM  6  │ Pin 31   │ Left encoder Channel B (white wire)
BCM 12  │ Pin 32   │ Front IR sensor OUT
BCM 13  │ Pin 33   │ L298N ENB   — right motor PWM speed
BCM 19  │ Pin 35   │ Right encoder Channel A (yellow wire)
BCM 16  │ Pin 36   │ Back IR sensor OUT
BCM 26  │ Pin 37   │ Right encoder Channel B (white wire)
BCM 20  │ Pin 38   │ LED anode via 330Ω resistor → GND
BCM 21  │ Pin 40   │ Rocker switch (other pin → GND)
Pin  2  │ 5V       │ MT3608 VOUT+ (power IN to Pi)
Pin  6  │ GND      │ Common GND rail
```

### Arduino Uno — Pin Assignments (arduino_autonomous only)

```
Uno Pin  │ Connected To
─────────┼──────────────────────────────────────────────────────────────
VIN      │ Battery pack (+) ~11.1V
GND      │ Common GND rail
5V       │ HC-SR04 VCC + both encoder green wires
D4       │ L298N IN3  — right motor direction A
D5       │ L298N IN4  — right motor direction B
D7       │ L298N IN1  — left motor direction A
D8       │ L298N IN2  — left motor direction B
D10      │ Rocker switch (other pin → GND)
D11      │ HC-SR04 TRIG
D12      │ HC-SR04 ECHO
D13      │ LED anode via 330Ω resistor → GND
ENA      │ Jumper ON — no wire needed (full power always)
ENB      │ Jumper ON — no wire needed (full power always)
```

---

## GA25-370 Motor — All 6 Wires

```
Wire Colour │ What It Is        │ Pi versions                  │ Arduino version
────────────┼───────────────────┼──────────────────────────────┼──────────────────────
Red         │ Motor power +     │ L298N OUT1 (L) / OUT3 (R)    │ Same
Black       │ Motor power −     │ L298N OUT2 (L) / OUT4 (R)    │ Same
Green       │ Encoder VCC 5V    │ MT3608 5V rail               │ Uno 5V pin
Dark Blue   │ Encoder GND       │ Common GND rail              │ Common GND rail
Yellow      │ Encoder Channel A │ BCM5 (L) / BCM19 (R)         │ Leave unconnected
White       │ Encoder Channel B │ BCM6 (L) / BCM26 (R)         │ Leave unconnected
```

> Green and dark blue MUST always be connected even when encoder signals are unused.

---

## HC-SR04 Voltage Divider (Pi versions only)

The HC-SR04 ECHO pin outputs 5V. The Pi GPIO can only handle 3.3V.
Use a resistor divider to reduce it:

```
HC-SR04 ECHO (5V)
        │
       [1kΩ]
        │
        ├──────────────────► Pi BCM25 (sees ~3.33V ✓)
        │
       [2kΩ]
        │
       GND

Maths: 5V × (2kΩ ÷ (1kΩ + 2kΩ)) = 3.33V ✓
```

> The Arduino Uno is 5V logic — connect ECHO directly, no divider needed.

---

## Software Architecture (Pi Autonomous Version)

```
main.py ──── Entry point. Switch polling, LED state machine,
    │         hardware watchdog, fixed-rate 50Hz loop.
    │
    ├── config.py ──── Single source of truth. Every pin, speed,
    │                  distance, and timing constant. Edit only this.
    │
    ├── motors.py ──── MotorController + _Encoder (quadrature A+B).
    │                  forward, reverse, spin_left, spin_right,
    │                  arc_left, arc_right, stop, heading_error().
    │
    ├── sensors.py ─── UltrasonicSensor (4-sample median filter),
    │                  IRSensors (active-LOW edge detection),
    │                  OpponentTracker (confidence 0–1, bearing estimate).
    │
    └── strategy.py ── SumoFSM (7 states), TacticAdaptor (RUSHER /
                       STALKER / FLANKER), _ScanTracker (360° coverage).
```

### FSM States (highest → lowest priority)

```
1. EDGE_AVOID  — white line detected → reverse + spin away (always wins)
2. RAM         — opponent < 25cm → full-speed burst push
3. ATTACK      — opponent < 80cm → charge with encoder heading correction
4. STALK       — opponent 80–150cm → arc toward estimated bearing
5. ORBIT       — FLANKER tactic only → wide arc to approach from side
6. SEARCH      — nothing detected → spin-scan, reverses direction each 360°
7. START       — mandatory 5-second pre-match pause (lowest priority)
```

### Adaptive Tactic System

| Tactic | Behaviour | Triggered When |
|--------|-----------|----------------|
| RUSHER | Charge immediately at full speed | Default. Also if 2+ direct charges succeeded |
| STALKER | Careful curved approach | If pushed to edge |
| FLANKER | Wide orbit then side attack | If pushed to edge 3+ times |
| RUSHER (forced) | Ignore everything, charge | Final 30 seconds of match |

---

## Software Architecture (Arduino Autonomous Version)

```
arduino_autonomous.ino ── Main sketch. 9 labelled sections.
                           2-minute timed technique dispatch.
config.h ──────────────── All pins and constants. Edit only this.
motors.h / motors.cpp ──── 7 movement functions (no PWM — jumpers ON).
```

### 2-Minute Technique Timeline

| Time | Phase | Strategy |
|------|-------|---------|
| 0:00–0:30 | MATADOR | Bait charge, dodge sideways, counter from the side |
| 0:30–1:00 | BULLDOZER | Wander ring pattern, charge anything found |
| 1:00–1:30 | FLANKER | Arc wide around opponent, slam into side |
| 1:30–2:00 | BERSERKER | Fake retreat then charge, unpredictable aggression |

---

## 3D Chassis (Sumo_Robot_V2_3D_files/)

Box-style chassis designed in OpenSCAD. Target envelope: 194 × 184 × 86mm.

| File | Description |
|------|-------------|
| `Sumo_Box_Concept_v2.scad` | Parametric source — edit this to change dimensions |
| `Sumo_Box_Chassis_v2.stl` | Main enclosure body |
| `Sumo_Box_Lid_v2.stl` | Removable lid with 6× M3 fastener positions |
| `Sumo_Electronics_Tray_v2.stl` | Internal electronics deck |
| `Sumo_Front_Blade_v2.stl` | Front wedge — print at 60–100% infill |

Print in PETG, ASA, or nylon. PLA is too brittle for impact.
See `README_Sumo_Box_v2.md` for full print settings and assembly notes.

---

## Quick Start

### Pi Autonomous Version

```bash
# 1. Flash Raspberry Pi OS Lite (64-bit) using Raspberry Pi Imager
# 2. SSH into the Pi, then:

sudo apt update && sudo apt full-upgrade -y
sudo apt install -y python3-pip python3-venv git

git clone https://github.com/<your-username>/sumo-robot.git
cd sumo-robot

python3 -m venv .venv
source .venv/bin/activate
pip install RPi.GPIO

# One-time Pi setup (watchdog, performance governor, no-sleep)
bash setup_pi.sh
sudo reboot
```

After reboot:
```bash
cd sumo-robot && source .venv/bin/activate
python main.py
```

### Auto-start on Power-Up (systemd)

```bash
sudo nano /etc/systemd/system/sumo.service
```

```ini
[Unit]
Description=Sumo Robot
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
sudo journalctl -u sumo.service -f   # watch live logs
```

### Arduino Autonomous Version

1. Open `arduino_autonomous/arduino_autonomous.ino` in Arduino IDE 2
2. Select **Tools → Board → Arduino Uno**
3. Select the correct COM port
4. Click Upload

See `arduino_autonomous/README.md` for full wiring and tuning guide.

---

## LED Status Reference (all versions)

| LED State | Meaning |
|-----------|---------|
| OFF | Switch is OFF — safe to handle |
| SLOW BLINK (200ms/800ms) | Waiting for switch to be turned ON |
| FAST BLINK (100ms/100ms) | Start delay counting down |
| SOLID ON | Robot is active and competing |

---

## Tuning

All tuning is done by editing the config file for your version:
- Pi versions: `config.py` in the root folder or `manual_control/config.py`
- Arduino version: `arduino_autonomous/config.h`

Never hardcode numbers in any other file.

---

## Troubleshooting

| Problem | Most Likely Cause | Fix |
|---------|------------------|-----|
| Robot does not move | Switch not ON, or ENA/ENB jumpers wrong | Check switch, check L298N jumpers |
| One motor goes backward | Motor wires reversed | Swap red/black on that motor's L298N terminals |
| Robot spins in circles | One motor faster or reversed | Swap wires — never change code for this |
| Pi resets when motors start | MT3608 can't supply enough current | Add 1000µF/10V cap across MT3608 VOUT+/VOUT− |
| No opponent detection | HC-SR04 wiring wrong | Check TRIG/ECHO pins and voltage divider (Pi) |
| Robot falls off ring | IR sensors need calibration | Adjust potentiometer on each IR sensor board |
| SSH connection refused | Pi still booting | Wait 60 seconds, try again |
| Code crashes in match | Check logs | `sudo journalctl -u sumo.service -f` |

---

## License

MIT — free to use, modify, and distribute for educational and competition purposes.
