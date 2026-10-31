# Arduino Nano Autonomous Sumo Robot — Compact High-Density Build

This folder contains the complete firmware and technical documentation for the **Arduino Nano** (ATmega328P) autonomous sumo robot.

The Nano version delivers the identical competitive strategy as the Uno build, but in an ultra-compact form factor (18mm × 45mm), allowing you to tuck the microcontroller low inside the chassis to maximize low Center-of-Gravity (CoG) and pack extra ballast weight for crushing pushing power.

---

## Table of Contents

1. [Hardware Bill of Materials](#1-hardware-bill-of-materials)
2. [Power Architecture (L298N 5V Rail)](#2-power-architecture-l298n-5v-rail)
3. [Complete Wiring Guide](#3-complete-wiring-guide)
4. [2-Minute Strategy Timeline](#4-2-minute-strategy-timeline)
5. [Uploading to Arduino Nano (Old Bootloader Guide)](#5-uploading-to-arduino-nano-old-bootloader-guide)
6. [Pre-Match Checkout & Calibration](#6-pre-match-checkout--calibration)
7. [Tuning Guide](#7-tuning-guide)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Hardware Bill of Materials

| # | Component | Qty | Notes |
|---|-----------|-----|-------|
| 1 | **Arduino Nano V3.0** (ATmega328P) | 1 | Compact DIP-30 microcontroller |
| 2 | **GA25-370 12V 130RPM DC Gear Motors** | 2 | Heavyweight drive motors |
| 3 | **L298N Dual H-Bridge Motor Driver** | 1 | Motor controller & 5V regulated power hub |
| 4 | **HC-SR04 Ultrasonic Sensor** | 1 | Forward opponent sonar tracking |
| 5 | **TCRT5000 / Digital IR Line Sensors** | 2 | White boundary edge detectors (front & rear) |
| 6 | **3S 18650 Li-ion Battery Pack** (~11.1V – 12.6V) | 1 | High-discharge drive battery |
| 7 | **3-Slot 18650 Holder** | 1 | Pack enclosure |
| 8 | **Rocker Switch** (2-pin SPST) | 1 | Emergency arm/disarm switch |
| 9 | **Status LED + 330Ω Resistor** | 1 | Readiness & 5s delay countdown indicator |
| 10 | **Robot Wheels (65mm × 26mm)** + 4mm couplers | 2 | Drive wheel set |
| 11 | **Metal Swivel Caster** | 1 | Low-profile front pivot wheel |

---

## 2. Power Architecture (L298N 5V Rail)

By keeping the **5V-EN jumper ON** on the L298N driver, its internal 78M05 regulator steps the 11.1V battery power down to a stable 5.0V.

```
3S 18650 Pack (~11.1V)
   │
   ├──────────────────────────────► L298N "12V" Screw Terminal
   │                                 └── Directly drives GA25-370 motors at 11.1V
   │
[L298N 5V-EN Jumper ON]
   │
   └─ L298N "5V" Screw Terminal ──► Arduino Nano "5V" Pin
                                     ├── Powers ATmega328P logic
                                     ├── HC-SR04 VCC (5V)
                                     ├── Front IR Line Sensor VCC (5V)
                                     ├── Back IR Line Sensor VCC (5V)
                                     └── Both GA25-370 Motor Green Wires (Encoder VCC)

Common Ground:
   Battery (−) ──── L298N GND ──── Arduino Nano GND ──── Sensor GNDs ──── Encoder Blue Wires
```

> ⚠️ **CONNECT TO NANO 5V PIN (NOT VIN)**: The Nano VIN pin feeds an onboard LDO regulator that requires 7V+. Since the L298N terminal is already regulated 5.0V, wire it directly into the Nano's **5V pin**.

---

## 3. Complete Wiring Guide

### Pinout Table

| Arduino Nano Pin | Connected To | Wire / Logic Note |
|------------------|--------------|-------------------|
| **5V** | L298N 5V Output Terminal | Regulated 5.0V Power IN |
| **GND** | Common GND Rail | System Ground |
| **D7** | L298N `IN1` | Left Motor Direction A |
| **D8** | L298N `IN2` | Left Motor Direction B |
| **D4** | L298N `IN3` | Right Motor Direction A |
| **D5** | L298N `IN4` | Right Motor Direction B |
| **D9** | HC-SR04 `TRIG` | Ultrasonic pulse trigger output |
| **D10** | HC-SR04 `ECHO` | Ultrasonic echo return (5V logic direct) |
| **A0** | Front IR Sensor `OUT` | White line active-LOW |
| **A1** | Back IR Sensor `OUT` | White line active-LOW |
| **D2** | Rocker Switch Pin 1 | Armed = LOW, Disarmed = HIGH (Pin 2 to GND) |
| **D13** | Status LED Anode | Via 330Ω to GND (mirrored by built-in D13 LED) |

### L298N Driver Jumper Settings
- **ENA:** Jumper **ON** (100% full torque left motor)
- **ENB:** Jumper **ON** (100% full torque right motor)
- **5V-EN:** Jumper **ON** (Powers 5V screw terminal from battery)

### Motor Wiring (GA25-370)
- **Red:** Motor Terminal (+) → L298N OUT1/OUT3
- **Black:** Motor Terminal (−) → L298N OUT2/OUT4
- **Green:** Encoder VCC → 5V Rail (**MUST be powered**)
- **Dark Blue:** Encoder GND → Common GND (**MUST be connected**)
- **Yellow / White:** Leave disconnected (unused in this build)

---

## 4. 2-Minute Strategy Timeline

The robot executes 4 progressive combat phases designed to beat different opponent types:

```
[Switch ON] ──► 5-Second Rapid Blink Delay (Mandatory Rule)
                     │
                     ▼
 [0:00 – 0:30] Phase 1: MATADOR
   • Creeps forward to bait the opponent into charging.
   • Computes closing speed: if opponent accelerates (>55 cm/s), sideways dodge.
   • Refaces and attacks from the exposed flank.
                     │
                     ▼
 [0:30 – 1:00] Phase 2: BULLDOZER
   • Ring-sweeping search pattern; direct ramming upon target acquisition.
                     │
                     ▼
 [1:00 – 1:30] Phase 3: FLANKER
   • Wide-arc circling maneuvers to strike opponent from the side.
                     │
                     ▼
 [1:30 – 2:00] Phase 4: BERSERKER
   • Feint & charge: every 3rd attack performs a quick reverse fake-out before full-throttle ramming.
```

### Edge Interrupt (Highest Priority)
If front IR hits white border: Reverse 300ms → Spin 400ms toward center.
If back IR hits white border: Drive forward 300ms → Spin 400ms.

---

## 5. Uploading to Arduino Nano (Old Bootloader Guide)

1. Connect the Nano to your computer using a **Mini-USB cable**.
2. Open **Arduino IDE**.
3. Open `arduino_nano/arduino_nano.ino`.
4. Go to **Tools → Board → Arduino AVR Boards → Arduino Nano**.
5. Go to **Tools → Processor**:
   - If your Nano is an official board or newer version, choose **ATmega328P**.
   - If your board is a common clone (CH340 chip) or gives an `avrdude: stk500_recv(): programmer is not responding` error, select **ATmega328P (Old Bootloader)**. This fixes 95% of Nano upload issues!
6. Select your COM port under **Tools → Port**.
7. Click **Upload** (`Ctrl+U`).

---

## 6. Pre-Match Checkout & Calibration

1. **IR Line Sensor Tuning:** Place robot on black dohyo. Rotate potentiometer on each IR module until the signal LED just turns off. Slide onto the white border ring — the LED must light up bright red immediately.
2. **Ultrasonic Range Check:** Open Arduino Serial Monitor (or run pre-match test) and observe motor response when an obstacle enters within 60cm.
3. **Switch & Delay Check:** Flip switch ON. Verify the LED blinks for exactly 5 seconds before movement starts.

---

## 7. Tuning Guide

All constants are in `arduino_nano/config.h`:

| Constant | Value | Description |
|----------|-------|-------------|
| `START_DELAY_MS` | `5000UL` | 5-second countdown timer |
| `ATTACK_DIST_CM` | `60` | Ultrasonic attack trigger range |
| `CHARGE_SPEED_CM_S` | `55` | Speed drop threshold for Matador dodge |
| `EDGE_REVERSE_MS` | `280UL` | Boundary avoidance reverse duration |
| `EDGE_SPIN_MS` | `380UL` | Boundary avoidance recovery turn |
| `MATADOR_MAX_DODGES` | `4` | Number of Matador feints before bulldozer mode |

---

## 8. Troubleshooting

- **`avrdude: stk500_getsync()` error:** Go to **Tools → Processor** and switch to **ATmega328P (Old Bootloader)**. Ensure CH340 USB drivers are installed on Windows.
- **Robot spins instead of moving straight:** In `motors.cpp`, verify `motorForward()` sets `LEFT_IN1=HIGH, LEFT_IN2=LOW` and `RIGHT_IN3=HIGH, RIGHT_IN4=LOW`. If one wheel runs backward, invert that motor's wires in the L298N terminal.
- **Brownout / Random Reset on Motor Start:** Add a 470µF–1000µF 16V electrolytic capacitor across Nano 5V and GND.
