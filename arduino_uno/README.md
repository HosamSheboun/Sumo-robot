# Arduino Uno Autonomous Sumo Robot — Competition System

This folder contains the complete, battle-tested autonomous sumo robot firmware for the **Arduino Uno** (ATmega328P).

It features a **timed 4-phase adaptive combat timeline** (Matador → Bulldozer → Flanker → Berserker) with **instantaneous edge-avoidance interrupts** using both front and rear downward IR line sensors and front ultrasonic tracking.

---

## Table of Contents

1. [Hardware Bill of Materials](#1-hardware-bill-of-materials)
2. [Unified Power Architecture (No MT3608)](#2-unified-power-architecture-no-mt3608)
3. [Complete Wiring Guide](#3-complete-wiring-guide)
4. [The 2-Minute Technique System](#4-the-2-minute-technique-system)
5. [Code Structure](#5-code-structure)
6. [Uploading to Arduino Uno](#6-uploading-to-arduino-uno)
7. [Pre-Match Testing Routine](#7-pre-match-testing-routine)
8. [Tuning Guide](#8-tuning-guide)
9. [Troubleshooting](#9-troubleshooting)

---

## 1. Hardware Bill of Materials

| # | Component | Qty | Purpose |
|---|-----------|-----|---------|
| 1 | **Arduino Uno R3** (ATmega328P) | 1 | Microcontroller brain |
| 2 | **GA25-370 DC Gear Motors** (12V, 130RPM) | 2 | High-torque left & right drive |
| 3 | **L298N Dual H-Bridge Motor Driver** | 1 | Motor power driver + 5V system power source |
| 4 | **HC-SR04 Ultrasonic Sensor** | 1 | Opponent distance & charge detection (front) |
| 5 | **TCRT5000 / Digital IR Line Sensors** | 2 | Downward edge sensors (1 front, 1 rear) |
| 6 | **3S 18650 Li-ion Battery Pack** (~11.1V – 12.6V) | 1 | Main system power |
| 7 | **3-Slot 18650 Battery Holder** with switch lead | 1 | Pack housing |
| 8 | **Rocker Switch (SPST 2-pin)** | 1 | Safe match arming / disarming switch |
| 9 | **Status LED + 330Ω Resistor** | 1 | Match state indicator (Blink / Armed) |
| 10 | **Robot Wheels (65mm × 26mm)** + 4mm couplers | 2 | High-traction drive wheels |
| 11 | **Metal Swivel Caster** | 1 | Low-friction front balance caster |

> **No MT3608 boost converter is needed.** The on-board regulator on the L298N provides clean, unified 5V power for the Uno, sensors, and encoder logic.

---

## 2. Unified Power Architecture (No MT3608)

The L298N module includes an internal 78M05 linear voltage regulator. With the **5V-EN jumper installed** on the L298N, the driver accepts 11.1V from the 3S battery and outputs a regulated 5.0V on its "5V" screw terminal.

```
3S 18650 Pack (~11.1V)
   │
   ├──────────────────────────────► L298N "12V" Screw Terminal
   │                                 └── Powers both GA25-370 motors at full 11.1V
   │
[L298N 5V-EN Jumper ON]
   │
   └─ L298N "5V" Screw Terminal ──► Arduino Uno "5V" Pin
                                     ├── Powers ATmega328P logic
                                     ├── HC-SR04 VCC (5V)
                                     ├── Front IR Sensor VCC (5V)
                                     ├── Back IR Sensor VCC (5V)
                                     └── Both GA25-370 Motor Green Wires (Encoder VCC)

Common Ground Rail:
   Battery (−) ──── L298N GND ──── Arduino Uno GND ──── All Sensor GNDs ──── Encoder Blue Wires
```

> ⚠️ **IMPORTANT**: Wire the L298N 5V output terminal directly to the Uno **5V pin** (NOT the VIN pin). The VIN pin requires at least 7V to pass through the Uno's secondary regulator, whereas the 5V pin accepts regulated 5.0V directly.

---

## 3. Complete Wiring Guide

### Pin Assignments Table

| Arduino Uno Pin | Connected To | Wire Type / Note |
|-----------------|--------------|------------------|
| **5V** | L298N 5V Screw Terminal | Regulated 5V Power Input |
| **GND** | Common GND Rail | Ground reference |
| **D7** | L298N `IN1` | Left Motor Direction A |
| **D8** | L298N `IN2` | Left Motor Direction B |
| **D4** | L298N `IN3` | Right Motor Direction A |
| **D5** | L298N `IN4` | Right Motor Direction B |
| **D11** | HC-SR04 `TRIG` | Ultrasonic pulse trigger |
| **D12** | HC-SR04 `ECHO` | Ultrasonic echo return (5V direct) |
| **A0** | Front IR Sensor `OUT` | White line detection (Active LOW) |
| **A1** | Back IR Sensor `OUT` | White line detection (Active LOW) |
| **D2** | Rocker Switch Pin 1 | Arming switch (Pin 2 goes to GND) |
| **D13** | Status LED Anode | Through 330Ω resistor to GND (Uno built-in LED mirrors) |

### L298N Driver Jumper Configuration
- **ENA Jumper:** **INSTALLED (ON)** — Left motor operates at 100% full torque.
- **ENB Jumper:** **INSTALLED (ON)** — Right motor operates at 100% full torque.
- **5V-EN Jumper:** **INSTALLED (ON)** — Powers the onboard 5V regulator from the 11.1V battery.

### GA25-370 6-Wire Motor Connections
- **Red:** Motor Terminal (+) → L298N `OUT1` (Left) / `OUT3` (Right)
- **Black:** Motor Terminal (−) → L298N `OUT2` (Left) / `OUT4` (Right)
- **Green:** Encoder VCC → 5V Rail (**MUST be connected** to prevent back-EMF floating noise)
- **Dark Blue:** Encoder GND → Common GND (**MUST be connected**)
- **Yellow:** Channel A → Leave unconnected (signals not used in this build)
- **White:** Channel B → Leave unconnected

---

## 4. The 2-Minute Technique System

Top international competitors win sumo matches not by blind charging, but by counter-tactics. The Arduino Uno firmware executes a **4-phase combat timeline**:

```
Match Start (5s delay)
       │
       ▼
 [0:00 – 0:30] Phase 1: MATADOR
   • Baits opponent charge by creeping forward slowly.
   • Computes closing speed: if opponent charges (>55 cm/s), sideways arc-dodge.
   • Immediately refaces opponent and delivers high-impact flank counter-attack.
       │
       ▼
 [0:30 – 1:00] Phase 2: BULLDOZER
   • Aggressive ring sweeping in alternating forward/spin search patterns.
   • High-traction head-on ramming upon lock-on within 60 cm.
       │
       ▼
 [1:00 – 1:30] Phase 3: FLANKER
   • Wide-radius flanking curve to bypass enemy front blades and strike exposed flanks.
       │
       ▼
 [1:30 – 2:00] Phase 4: BERSERKER
   • Unpredictable feints: every 3rd attack executes a fake reverse (180ms)
     to make the opponent overcommit, then lunges forward with full pack voltage.
```

### Edge Avoidance (Top Priority Interrupt)
At any point during any phase, if either the front (`A0`) or rear (`A1`) IR sensor detects the white dohyo border:
1. All tactics abort immediately.
2. Front sensor triggered → Full-power reverse (300ms) + 90° right spin (400ms).
3. Rear sensor triggered → Full-power forward (300ms) + 90° right spin (400ms).
4. Both triggered → Hard emergency escape reverse + 180° spin.

---

## 5. Code Structure

```
arduino_uno/
├── arduino_uno.ino   # Main loop, FSM phase scheduler, sensor polling
├── config.h          # Single source of truth: pins, thresholds, timings
├── motors.h          # Direction control prototypes
├── motors.cpp        # Optimized direct digital pin controls
└── README.md         # This technical documentation
```

---

## 6. Uploading to Arduino Uno

1. Connect the Arduino Uno to your PC via a **USB Type-B cable**.
2. Open the **Arduino IDE**.
3. Open `arduino_uno/arduino_uno.ino`.
4. Go to **Tools → Board → Arduino AVR Boards → Arduino Uno**.
5. Go to **Tools → Port** and select your Uno's COM port (e.g., `COM3` on Windows or `/dev/ttyUSB0` on Linux).
6. Click the **Upload** arrow button (or press `Ctrl+U`).
7. Once upload completes ("Done uploading"), disconnect the USB cable before running from the battery.

---

## 7. Pre-Match Testing Routine

Before placing the robot in competition:
1. **Edge Sensor Test:** Prop the robot on a block so wheels spin freely. Turn rocker switch ON. Slide a piece of white paper under the front IR sensor — the wheels must immediately spin in reverse! Slide paper under the back IR sensor — wheels must spin forward.
2. **Ultrasonic Range Check:** Wave your hand 30cm in front of the HC-SR04 — the motors should engage in a forward attack drive.
3. **5-Second Delay Verification:** Flip switch ON: LED must flash rapidly for exactly 5.0 seconds before motors engage.

---

## 8. Tuning Guide

All constants are conveniently located in `config.h`:

| Constant | Default | Purpose / Adjustment |
|----------|---------|---------------------|
| `START_DELAY_MS` | `5000UL` | Pre-match countdown (set to `1000UL` for testing, `5000UL` for official match) |
| `ATTACK_DIST_CM` | `60` | Ultrasonic trigger distance for attack |
| `CHARGE_SPEED_CM_S` | `55` | Closing speed threshold for triggering the Matador dodge |
| `EDGE_REVERSE_MS` | `300UL` | Duration of reverse away from border line |
| `EDGE_SPIN_MS` | `400UL` | Spin duration to re-orient toward ring center |
| `MATADOR_MAX_DODGES` | `4` | Max sidestep dodges before transitioning |

---

## 9. Troubleshooting

- **Motors hum but do not turn:** Check battery voltage. If 3S pack is below 10.0V, recharge the cells. Ensure the ENA/ENB jumpers on the L298N are securely seated.
- **Robot spins the wrong direction during forward drive:** Swap the motor wires in the L298N output terminals (swap OUT1/OUT2 for left motor, or OUT3/OUT4 for right motor).
- **Robot drives off the ring without stopping:** Check IR sensor sensitivity potentiometers. Adjust until the onboard LED on the IR module toggles reliably between the black dohyo surface and white paper.
- **Uno reboots when motors start:** Place a 470µF to 1000µF 16V capacitor across the L298N 5V output and GND to buffer peak current draw during motor stall/reversal.
