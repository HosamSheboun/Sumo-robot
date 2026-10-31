# Autonomous Heavyweight Sumo Robot — Multi-Controller Competition System

A fully autonomous 3 kg sumo robot project with **complete firmware for 4 different controllers** — Raspberry Pi Zero 2 W, Arduino Uno, Arduino Nano, and ESP32 — each with battle-tested strategies, edge avoidance, and ultrasonic opponent tracking.

No remote control. Flip the switch, place on the ring, it fights entirely on its own.

---

## Project Structure

```
Sumo robot/
│
├── README.md                        ← This file — master overview of the whole project
│
├── pi_autonomous/                   ← Raspberry Pi Zero 2 W — most capable version
│   ├── config.py                    ← All BCM pins and tuning constants
│   ├── main.py                      ← Entry point, switch, LED, watchdog, 50Hz loop
│   ├── motors.py                    ← L298N + quadrature encoder control
│   ├── sensors.py                   ← HC-SR04, IR sensors, OpponentTracker
│   ├── strategy.py                  ← 7-state adaptive FSM + TacticAdaptor
│   ├── setup_pi.sh                  ← One-time Pi setup script
│   └── README.md                    ← Full wiring, FSM explanation, tuning guide
│
├── arduino_uno/                     ← Arduino Uno — 4-phase combat timeline
│   ├── arduino_uno.ino              ← Main sketch: Matador→Bulldozer→Flanker→Berserker
│   ├── config.h                     ← All pins and timing constants
│   ├── motors.h / motors.cpp        ← L298N motor control (7 movement functions)
│   └── README.md                    ← Full wiring guide and tuning reference
│
├── arduino_nano/                    ← Arduino Nano — compact build, same strategy as Uno
│   ├── arduino_nano.ino             ← Main sketch (Nano-adapted pins)
│   ├── config.h                     ← Nano-specific pins
│   ├── motors.h / motors.cpp        ← L298N motor control
│   └── README.md                    ← Nano-specific upload guide (Old Bootloader)
│
├── esp32/                           ← ESP32 — dual-core, PWM speed control, fastest
│   ├── esp32_sumo.ino               ← Dual-core FreeRTOS main sketch
│   ├── config.h                     ← GPIO pins, speeds, timing, PWM settings
│   ├── motors.h / motors.cpp        ← LEDC hardware PWM motor control
│   └── README.md                    ← Voltage divider guide, board support install
│
├── manual_control/                  ← Raspberry Pi Zero 2 W — Bluetooth manual drive
│   ├── main.py                      ← Entry point: switch, LED, joystick loop
│   ├── motors.py                    ← L298N + encoder control with tank-mix
│   ├── bluetooth.py                 ← Dabble SPP packet parser, RFCOMM socket server
│   ├── config.py                    ← Pins, joystick settings, BT name
│   └── README.md                    ← Pairing guide, driving controls, systemd setup
│
├── Sumo_Robot_V2_3D_files/          ← 3D-printed chassis design (OpenSCAD + STL)
│   ├── Sumo_Box_Concept_v2.scad     ← Parametric source (edit this)
│   ├── Sumo_Box_Chassis_v2.stl      ← Main enclosure body
│   ├── Sumo_Box_Lid_v2.stl          ← Removable lid
│   ├── Sumo_Electronics_Tray_v2.stl ← Internal electronics deck
│   ├── Sumo_Front_Blade_v2.stl      ← Front wedge/blade
│   ├── README_Sumo_Box_v2.md        ← Print settings, assembly notes
│   └── *.png                        ← Render previews
│
└── docs/                            ← Documentation and competition reference
    ├── DEVLOG.md                    ← Full Terra Hack Club development log
    ├── Poster_Rulebook.pdf          ← Competition poster and rulebook
    └── Sumo Robot Competition Handbook V1.pdf ← Full competition handbook
```

---

## Which Controller Should I Use?

| Version | Controller | Brain | Sensors | Best For |
|---------|-----------|-------|---------|----------|
| **Pi Autonomous** (`pi_autonomous/`) | Raspberry Pi Zero 2 W | Python FSM, 7 states, encoder feedback, opponent tracker, tactic adaptor | HC-SR04 + 2× IR + encoders | Most capable — competition top pick |
| **ESP32** (`esp32/`) | ESP32 dual-core 240MHz | Dual-core FreeRTOS, LEDC PWM, 5-state priority FSM + 2-min timeline | HC-SR04 + 2× IR | Fastest reaction time — best Arduino-class option |
| **Arduino Uno** (`arduino_uno/`) | Arduino Uno | 4-phase technique timeline, edge avoid, charge detection | HC-SR04 + 2× IR | Reliable, simple build |
| **Arduino Nano** (`arduino_nano/`) | Arduino Nano | Same strategy as Uno, compact form factor | HC-SR04 + 2× IR | Space-constrained chassis |
| **Manual Control** (`manual_control/`) | Raspberry Pi Zero 2 W | Dabble Bluetooth joystick | — | Testing, demos, practice driving |

> Each folder is completely standalone. No shared code or imports between folders.

---

## Hardware Overview

### Competition Specs
- **Weight limit:** ≤ 3 kg
- **Size limit:** ≤ 20 × 20 cm
- **Arena:** Black circular dohyo, white boundary ring

### Master Parts List

| # | Component | Qty | Pi | Uno/Nano | ESP32 |
|---|-----------|-----|----|----|-------|
| 1 | Raspberry Pi Zero 2 W | 1 | ✅ | — | — |
| 2 | Arduino Uno (ATmega328P) | 1 | — | ✅ | — |
| 3 | Arduino Nano (ATmega328P) | 1 | — | ✅ | — |
| 4 | ESP32 Dev Module (WROOM-32) | 1 | — | — | ✅ |
| 5 | GA25-370 DC Gear Motor (12V, 130RPM) | 2 | ✅ | ✅ | ✅ |
| 6 | Robot Wheel 65mm × 26mm + 4mm coupler | 2 | ✅ | ✅ | ✅ |
| 7 | Metal swivel caster wheel | 1 | ✅ | ✅ | ✅ |
| 8 | L298N Dual H-Bridge Motor Driver | 1 | ✅ | ✅ | ✅ |
| 9 | HC-SR04 Ultrasonic Sensor | 1 | ✅ | ✅ | ✅ |
| 10 | Downward IR Line Sensor (TCRT5000 / 3-pin digital) | 2 | ✅ | ✅ | ✅ |
| 11 | 3S 18650 Li-ion Battery Pack (~11.1V) | 1 | ✅ | ✅ | ✅ |
| 12 | MT3608 DC-DC Boost Converter (HW-045) | 1 | ✅ | — | — |
| 13 | 1kΩ resistor (ECHO divider) | 1 | ✅ | — | ✅ |
| 14 | 2kΩ resistor (ECHO divider) | 1 | ✅ | — | ✅ |
| 15 | Rocker Switch (2-pin SPST) | 1 | ✅ | ✅ | ✅ |
| 16 | LED (any colour) + 330Ω resistor | 1 | ✅ | ✅ | ✅ |

---

## Power Architecture

### Pi Version (MT3608 single-cell tap)

```
3× 18650 in series (~11.1V)
         │
         ├───────────────────────────────► L298N 12V terminal → Both motors
         │
         └── Single cell tap (~3.7V) ──► MT3608 VIN+ → 5.0V output
                                          │
              ┌──────────────────────────┬┴──────────────────────┐
              ▼                          ▼                       ▼
       Pi Zero 2 W Pin 2          HC-SR04 VCC           Both IR sensors VCC
                                                   Both encoder green wires

Common GND: Battery(−) ── L298N GND ── MT3608 VOUT− ── Pi GND ── All sensor GNDs
```

> ⚠️ MT3608 is boost-only. Tap a single cell (~3.7V), NOT the full pack.  
> Set output to exactly 5.00V with a multimeter before connecting the Pi.

### Arduino Uno / Nano (L298N 5V output)

```
3× 18650 in series (~11.1V)
         │
         ├───────────────────────────────► L298N 12V terminal → Both motors
         │
[L298N 5V-EN Jumper ON]
         │
         └─ L298N "5V" Screw Terminal ──► Arduino 5V pin (Uno/Nano)
                                           ├── Uno/Nano logic
                                           ├── HC-SR04 VCC
                                           ├── Both IR sensor VCC
                                           └── Both encoder green wires

Common GND: Battery(−) ── L298N GND ── Arduino GND ── All sensor GNDs
```

> ⚠️ Wire to the Arduino **5V pin** (NOT VIN). VIN requires 7V+ minimum.

### ESP32 (L298N 5V → ESP32 VIN)

```
3× 18650 in series (~11.1V)
         │
         ├───────────────────────────────► L298N 12V terminal → Both motors
         │
[L298N 5V-EN Jumper ON]
         │
         └─ L298N "5V" Screw Terminal ──► ESP32 Dev Board VIN pin
                                           └── Onboard 3.3V LDO → ESP32 logic

HC-SR04 ECHO (5V) ─── 1kΩ ─── GPIO19 ─── 2kΩ ─── GND   (divider → 3.33V)
```

> ⚠️ ESP32 GPIO is 3.3V only. HC-SR04 ECHO **MUST** go through a 1kΩ + 2kΩ voltage divider.

---

## GA25-370 Motor — 6-Wire Reference

| Wire Colour | Function | Pi version | Uno/Nano | ESP32 |
|-------------|----------|-----------|----------|-------|
| Red | Motor terminal (+) | L298N OUT1/OUT3 | L298N OUT1/OUT3 | L298N OUT1/OUT3 |
| Black | Motor terminal (−) | L298N OUT2/OUT4 | L298N OUT2/OUT4 | L298N OUT2/OUT4 |
| **Green** | **Encoder VCC 5V** | **MT3608 5V rail** | **L298N 5V terminal** | **L298N 5V terminal** |
| **Dark Blue** | **Encoder GND** | **Common GND** | **Common GND** | **Common GND** |
| Yellow | Encoder Channel A | BCM5 (L) / BCM19 (R) | Unconnected | Unconnected |
| White | Encoder Channel B | BCM6 (L) / BCM26 (R) | Unconnected | Unconnected |

> **Green and Dark Blue MUST be connected in all builds**, even when encoder signals are unused. A floating encoder board can generate back-EMF that blocks the motor from spinning.

---

## HC-SR04 ECHO Voltage Divider (Pi + ESP32)

The HC-SR04 ECHO pin outputs 5V. Both Pi and ESP32 GPIO are 3.3V max.

```
HC-SR04 ECHO (5V)
        │
       [1kΩ]
        │
        ├──────────────► GPIO input (sees ~3.33V ✓)
        │
       [2kΩ]
        │
       GND

Maths: 5V × (2000 ÷ 3000) = 3.33V ✓
```

> Arduino Uno/Nano are 5V logic — connect ECHO directly, no divider needed.

---

## Pin Reference

### Pi Zero 2 W — BCM Assignments

```
BCM Pin │ Physical │ Connected To
────────┼──────────┼──────────────────────────────────────────────────
BCM 17  │ Pin 11   │ L298N IN1   — left motor direction A
BCM 18  │ Pin 12   │ L298N ENA   — left motor PWM speed
BCM 27  │ Pin 13   │ L298N IN2   — left motor direction B
BCM 22  │ Pin 15   │ L298N IN3   — right motor direction A
BCM 23  │ Pin 16   │ L298N IN4   — right motor direction B
BCM  5  │ Pin 29   │ Left encoder Channel A (yellow wire)
BCM  6  │ Pin 31   │ Left encoder Channel B (white wire)
BCM 13  │ Pin 33   │ L298N ENB   — right motor PWM speed
BCM 19  │ Pin 35   │ Right encoder Channel A (yellow wire)
BCM 16  │ Pin 36   │ Back IR sensor OUT
BCM 26  │ Pin 37   │ Right encoder Channel B (white wire)
BCM 20  │ Pin 38   │ Status LED anode (via 330Ω to GND)
BCM 21  │ Pin 40   │ Rocker switch (other pin → GND)
BCM 24  │ Pin 18   │ HC-SR04 TRIG
BCM 25  │ Pin 22   │ HC-SR04 ECHO (after 1kΩ+2kΩ divider)
BCM 12  │ Pin 32   │ Front IR sensor OUT
```

### Arduino Uno — Pin Assignments

```
Uno Pin │ Connected To
────────┼──────────────────────────────────────────────────
5V      │ L298N "5V" Screw Terminal (power IN)
GND     │ Common GND rail
D4      │ L298N IN3  — right motor direction A
D5      │ L298N IN4  — right motor direction B
D7      │ L298N IN1  — left motor direction A
D8      │ L298N IN2  — left motor direction B
D11     │ HC-SR04 TRIG
D12     │ HC-SR04 ECHO (5V direct — Uno is 5V logic)
A0      │ Front IR sensor OUT (active LOW)
A1      │ Back IR sensor OUT  (active LOW)
D2      │ Rocker switch (other pin → GND)
D13     │ Status LED anode (via 330Ω to GND)
ENA     │ Jumper ON — no wire to Arduino needed
ENB     │ Jumper ON — no wire to Arduino needed
```

### Arduino Nano — Pin Assignments

```
Nano Pin │ Connected To
─────────┼──────────────────────────────────────────────────
5V       │ L298N "5V" Screw Terminal (power IN)
GND      │ Common GND rail
D4       │ L298N IN3  — right motor direction A
D5       │ L298N IN4  — right motor direction B
D7       │ L298N IN1  — left motor direction A
D8       │ L298N IN2  — left motor direction B
D9       │ HC-SR04 TRIG
D10      │ HC-SR04 ECHO (5V direct — Nano is 5V logic)
A0       │ Front IR sensor OUT (active LOW)
A1       │ Back IR sensor OUT  (active LOW)
D2       │ Rocker switch (other pin → GND)
D13      │ Status LED anode (via 330Ω to GND)
ENA      │ Jumper ON — no wire to Nano needed
ENB      │ Jumper ON — no wire to Nano needed
```

### ESP32 — GPIO Assignments

```
GPIO    │ Connected To
────────┼──────────────────────────────────────────────────
VIN     │ L298N "5V" Screw Terminal (power IN to onboard LDO)
GND     │ Common GND rail
GPIO27  │ L298N IN1  — left motor direction A
GPIO26  │ L298N IN2  — left motor direction B
GPIO14  │ L298N ENA  — left motor PWM  (LEDC Channel 0)
GPIO25  │ L298N IN3  — right motor direction A
GPIO33  │ L298N IN4  — right motor direction B
GPIO32  │ L298N ENB  — right motor PWM  (LEDC Channel 1)
GPIO18  │ HC-SR04 TRIG (3.3V direct)
GPIO19  │ HC-SR04 ECHO (via 1kΩ+2kΩ divider → 3.33V)
GPIO4   │ Front IR sensor OUT (active LOW)
GPIO16  │ Back IR sensor OUT  (active LOW)
GPIO13  │ Rocker switch (other pin → GND; has internal pull-up)
GPIO2   │ Status LED anode (via 330Ω to GND; mirrors built-in LED)
ENA     │ Jumper REMOVED — PWM via GPIO14
ENB     │ Jumper REMOVED — PWM via GPIO32
```

---

## Strategy Overview

### Pi Autonomous — Priority FSM

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

### ESP32 / Uno / Nano — 2-Minute Technique Timeline

| Phase | Time | Name | Strategy |
|-------|------|------|----------|
| 1 | 0:00–0:30 | **Matador** | Bait the charge, sideways dodge, counter from the flank |
| 2 | 0:30–1:00 | **Bulldozer** | Sweep ring in a search pattern, full-speed ram on contact |
| 3 | 1:00–1:30 | **Flanker** | Arc wide around opponent, slam into exposed side |
| 4 | 1:30–2:00 | **Berserker** | Fake retreat then charge every 3rd attack — unpredictable |

Edge avoidance (IR sensors) always interrupts any phase — it is always the top priority.

---

## Quick Start

### Pi Autonomous

```bash
# One-time Pi setup
bash pi_autonomous/setup_pi.sh && sudo reboot

# Run manually
cd pi_autonomous && source ../.venv/bin/activate && python main.py
```

### Arduino Uno / Nano

1. Open `arduino_uno/arduino_uno.ino` or `arduino_nano/arduino_nano.ino` in Arduino IDE
2. Select the correct board under **Tools → Board**
3. For Nano clones: **Tools → Processor → ATmega328P (Old Bootloader)**
4. Click **Upload**

### ESP32

1. Add ESP32 board URL in Arduino IDE Preferences
2. Install **esp32 by Espressif Systems** from Boards Manager
3. Open `esp32/esp32_sumo.ino`
4. Select **Tools → Board → ESP32 Dev Module**
5. Click **Upload** (hold BOOT button if board lacks auto-reset)

---

## LED Status Reference

| LED State | Meaning |
|-----------|---------|
| **OFF** | Switch is OFF — safe to handle |
| **Slow blink** (200ms on / 800ms off) | Waiting for switch ON |
| **Fast blink** (100ms on / 100ms off) | 5-second pre-match countdown |
| **Solid ON** | Robot is active and competing |

---

## Troubleshooting

| Problem | Most Likely Cause | Fix |
|---------|------------------|-----|
| Robot does not move at all | ENA/ENB jumpers missing, or no battery power | Check L298N jumpers (Uno/Nano); check battery voltage |
| One motor runs backward | Motor wires reversed | Swap Red/Black in L298N OUT terminals for that motor |
| Robot drives off ring | IR sensor not calibrated | Adjust potentiometer on each IR module until it triggers on white paper |
| No opponent detected | ECHO wiring wrong | Pi/ESP32: check voltage divider; Uno/Nano: direct ECHO connection |
| Pi resets when motors start | MT3608 can't deliver current spike | Add 1000µF/10V capacitor across MT3608 VOUT terminals |
| ESP32 crashes on motor start | LDO undervoltage from current spike | Add 1000µF/16V capacitor across L298N 5V terminal and GND |
| Nano won't upload | Clone board old bootloader | **Tools → Processor → ATmega328P (Old Bootloader)** |
| ESP32 upload fails | No auto-reset circuit | Hold BOOT button on ESP32 when Arduino IDE shows `Connecting...` |

---

## Docs

See [`docs/DEVLOG.md`](docs/DEVLOG.md) for the full Terra Hack Club development log.  
See `docs/` for competition rulebook and official competition handbook PDFs.

---

## License

MIT — free to use, modify, and distribute for educational and competition purposes.
