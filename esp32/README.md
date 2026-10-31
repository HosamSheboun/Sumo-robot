# ESP32 Autonomous Sumo Robot — Dual-Core Advanced Combat System

This folder contains the most capable version of the firmware — written for the **ESP32** (dual-core 240MHz, 3.3V logic). The ESP32 runs the ultrasonic sensor on **Core 0** and the entire combat strategy on **Core 1** simultaneously using FreeRTOS, achieving dramatically faster reaction times than any single-core Arduino build.

---

## Table of Contents

1. [Why ESP32 Wins Sumo Matches](#1-why-esp32-wins-sumo-matches)
2. [Hardware Bill of Materials](#2-hardware-bill-of-materials)
3. [Critical Voltage Warning (3.3V Logic)](#3-critical-voltage-warning-33v-logic)
4. [Power Architecture](#4-power-architecture)
5. [Complete Wiring Guide](#5-complete-wiring-guide)
6. [Strategy: Priority FSM + 2-Minute Timeline](#6-strategy-priority-fsm--2-minute-timeline)
7. [Code Structure](#7-code-structure)
8. [Installing ESP32 Board Support & Uploading](#8-installing-esp32-board-support--uploading)
9. [Pre-Match Checkout](#9-pre-match-checkout)
10. [Tuning Guide](#10-tuning-guide)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. Why ESP32 Wins Sumo Matches

| Feature | Arduino Uno/Nano | ESP32 |
|---------|-----------------|-------|
| Clock speed | 16 MHz | 240 MHz |
| Cores | 1 | **2 (dual-core)** |
| Sensor polling | Blocks motor loop | **Runs simultaneously on Core 0** |
| Motor control | Noisy `analogWrite` PWM | **LEDC hardware PWM — silent, smooth** |
| Ultrasonic resolution | Shared with loop timing | **Dedicated FreeRTOS task** |
| Reaction time | ~10–25ms loop | **~2–5ms sensor → response** |
| Speed control | Full speed only (jumpers) | **0–255 speed fine-tuned per maneuver** |
| Connectivity | None | **WiFi (OTA updates) + Bluetooth** |

The speed advantage is decisive in competition. The ESP32 can detect a charging opponent and trigger a Matador sidestep dodge roughly 4× faster than the Uno, meaning the robot leaves the charge window before impact.

---

## 2. Hardware Bill of Materials

| # | Component | Qty | Notes |
|---|-----------|-----|-------|
| 1 | **ESP32 Dev Module** (WROOM-32 / WROVER) | 1 | 38-pin or 30-pin both work |
| 2 | **GA25-370 12V 130RPM DC Gear Motors** | 2 | Heavyweight drive motors |
| 3 | **L298N Dual H-Bridge Motor Driver** | 1 | Motor power + 5V rail source |
| 4 | **HC-SR04 Ultrasonic Sensor** | 1 | Opponent detection (front-facing) |
| 5 | **TCRT5000 / Digital IR Line Sensors** | 2 | White border detection (front & rear) |
| 6 | **3S 18650 Li-ion Battery Pack** (~11.1V – 12.6V) | 1 | Full system power |
| 7 | **1kΩ resistor** | 1 | Part of HC-SR04 ECHO voltage divider |
| 8 | **2kΩ resistor** | 1 | Part of HC-SR04 ECHO voltage divider |
| 9 | **Rocker Switch** (2-pin SPST) | 1 | Arm/disarm switch |
| 10 | **Status LED + 330Ω Resistor** | 1 | Status indicator |
| 11 | **Robot Wheels 65mm × 26mm** + 4mm couplers | 2 | Drive wheels |
| 12 | **Metal Swivel Caster** | 1 | Front support wheel |

---

## 3. Critical Voltage Warning (3.3V Logic)

> ⚠️ **ESP32 GPIO pins are 3.3V logic — NOT 5V tolerant.** Applying 5V to any GPIO pin will permanently damage or destroy the chip.

**Affected signal:** HC-SR04 `ECHO` pin outputs 5V. You **must** use a resistor voltage divider before connecting it to the ESP32.

### ECHO Voltage Divider Circuit

```
HC-SR04 ECHO (5V)
      │
     [1kΩ]
      │
      ├──────────────── GPIO19 (ESP32 ECHO_PIN)
      │
     [2kΩ]
      │
     GND

Output voltage to GPIO = 5V × 2000 / (1000 + 2000) = 3.33V ✓
```

**TRIG pin:** The HC-SR04 accepts 3.3V HIGH as a valid trigger — connect the ESP32 TRIG output directly (no divider needed).

**IR Sensors:** Most 3.3V-compatible IR modules output 3.3V signals. Check the OUT pin voltage with a multimeter. If it swings to 5V, add the same 1kΩ + 2kΩ divider for that pin too.

---

## 4. Power Architecture

```
3S 18650 Pack (~11.1V)
   │
   ├─────────────────────────────► L298N "12V" Screw Terminal
   │                                └── Drives both GA25-370 motors at 11.1V
   │
[L298N 5V-EN Jumper ON]
   │
   └─ L298N "5V" Screw Terminal ──► ESP32 Dev Board "VIN" Pin (5V in)
                                     └── ESP32 onboard LDO → 3.3V
                                          ├── ESP32 logic (3.3V)
                                          ├── HC-SR04 VCC (5V) ← from L298N 5V directly
                                          ├── Both IR Sensor VCC (5V) ← from L298N 5V directly
                                          └── Both GA25-370 Green Wires (Encoder VCC)

Common Ground:
   Battery (−) ──── L298N GND ──── ESP32 GND ──── All Sensor GNDs ──── Encoder Blue Wires
```

> Note: HC-SR04 and IR sensors require 5V VCC — wire them from the L298N 5V terminal, not the ESP32 3.3V pin. Only connect their **signal/OUT pins** to the ESP32 through the voltage divider.

---

## 5. Complete Wiring Guide

### ESP32 Pin Assignments

| ESP32 GPIO | Connected To | Notes |
|------------|--------------|-------|
| **VIN** | L298N 5V Screw Terminal | 5V power input to onboard LDO |
| **GND** | Common GND Rail | — |
| **GPIO27** | L298N `IN1` | Left motor direction A |
| **GPIO26** | L298N `IN2` | Left motor direction B |
| **GPIO14** | L298N `ENA` | Left motor PWM speed (LEDC CH0) |
| **GPIO25** | L298N `IN3` | Right motor direction A |
| **GPIO33** | L298N `IN4` | Right motor direction B |
| **GPIO32** | L298N `ENB` | Right motor PWM speed (LEDC CH1) |
| **GPIO18** | HC-SR04 `TRIG` | Ultrasonic trigger (direct) |
| **GPIO19** | HC-SR04 `ECHO` | Via 1kΩ + 2kΩ divider — **see Section 3** |
| **GPIO4** | Front IR Sensor `OUT` | Active LOW |
| **GPIO16** | Back IR Sensor `OUT` | Active LOW |
| **GPIO13** | Rocker Switch Pin 1 | (Pin 2 → GND; uses internal pull-up) |
| **GPIO2** | Status LED Anode | Via 330Ω to GND (mirrors built-in LED) |

### L298N Jumper Settings
- **ENA Jumper:** **REMOVED** — PWM speed control via `GPIO14`. Do NOT use the jumper.
- **ENB Jumper:** **REMOVED** — PWM speed control via `GPIO32`. Do NOT use the jumper.
- **5V-EN Jumper:** **ON** — Powers the 5V screw terminal from the battery.

---

## 6. Strategy: Priority FSM + 2-Minute Timeline

The ESP32 firmware combines **reactive priority checking** with **a timed combat timeline**. Every loop tick on Core 1 first checks the three top-priority states before falling into the phase-specific strategy.

### Priority Stack (Checked Every Loop)
```
Priority 1 (EDGE_AVOID) — IR sensor sees white border → Escape immediately
Priority 2 (RAM)        — Opponent < 20cm → Full-speed burst push
Priority 3 (PHASE)      — Run the current timed combat phase
```

### 2-Minute Combat Timeline (Phase 3 Detail)
```
[0:00 – 0:30] MATADOR
   Core 0 computes opponent closing speed in real time.
   Detects charging (closing speed >55 cm/s) → precision sidestep dodge at SPEED_FULL.
   Uses LEDC speed control for smooth arc with outer/inner wheel differential.

[0:30 – 1:00] BULLDOZER
   Sweeps the ring in a 6-step pattern.
   Instantly shifts to full-speed charge on lock-on.

[1:00 – 1:30] FLANKER
   Smooth outer/inner wheel speed arc (220/80) to circle wide and attack from the side.
   Re-validates distance after arc before committing to slam.

[1:30 – 2:00] BERSERKER
   Every 3rd attack: reverse feint (SPEED_FULL, 160ms) then immediate charge (550ms).
   Disrupts traction patterns on heavy pushing bots.
```

---

## 7. Code Structure

```
esp32/
├── esp32_sumo.ino   # Main sketch: FreeRTOS sensor task + combat FSM phases
├── config.h         # All pins, speeds, thresholds, timings
├── motors.h         # Speed-controlled motor function prototypes
├── motors.cpp       # LEDC PWM implementation (Core 2/3 compatible)
└── README.md        # This documentation
```

### Dual-Core Architecture
```
ESP32
├── Core 0 (sensorTask — FreeRTOS)
│   ├── Reads HC-SR04 every ~1ms
│   ├── Computes closing speed
│   ├── Reads both IR sensors
│   └── Writes g_dist, g_closingSpd, g_irFront, g_irBack (with portMUX)
│
└── Core 1 (loop())
    ├── Reads sensor globals safely
    ├── Runs handleEdge() (Priority 1)
    ├── Runs handleRam()  (Priority 2)
    └── Runs current phase (MATADOR / BULLDOZER / FLANKER / BERSERKER)
```

---

## 8. Installing ESP32 Board Support & Uploading

### Step 1 — Add ESP32 Board Support URL
1. Open Arduino IDE.
2. Go to **File → Preferences**.
3. In **Additional Boards Manager URLs**, add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Click OK.

### Step 2 — Install ESP32 Boards
1. Go to **Tools → Board → Boards Manager**.
2. Search for **esp32**.
3. Install **esp32 by Espressif Systems** (latest version).

### Step 3 — Select Board and Upload
1. Go to **Tools → Board → ESP32 Arduino → ESP32 Dev Module**.
2. Set **Upload Speed** to `921600` for fastest upload.
3. Set **Flash Size** to `4MB (32Mb)`.
4. Select your COM port under **Tools → Port**.
5. Press the **BOOT button on the ESP32** while the IDE shows `Connecting...`, then release.
6. Click **Upload**.

> Many ESP32 dev boards with built-in auto-reset circuits don't need manual boot button pressing.

---

## 9. Pre-Match Checkout

1. **Voltage Divider Check (Before Powering On):** Use a multimeter to confirm the HC-SR04 ECHO pin reads ~3.3V when held 30cm from a wall.
2. **Motor Direction Test:** Hold robot above ground, flip switch ON, wait 5s. All 4 phases should engage motors. Check that forward motion is actually forward. If one motor runs backward, swap its OUT wires in the L298N terminal.
3. **Dual Core Verification:** Monitor boot messages over USB Serial (`baud 115200`) — you should see the FreeRTOS sensor task launched: `ARDUINO_EVENT_WIFI_READY` and normal boot messages (no watchdog warnings).

---

## 10. Tuning Guide

All constants are in `esp32/config.h`. The ESP32 benefits most from:

| Constant | Default | Tuning Notes |
|----------|---------|-------------|
| `ATTACK_DIST_CM` | `60` | Increase if robot misses nearby opponents |
| `RAM_DIST_CM` | `20` | RAM burst triggers at close range |
| `CHARGE_SPEED_CM_S` | `55` | Matador dodge threshold (cm/s closing speed) |
| `SPEED_FULL` | `255` | Attack / dodge / RAM speed |
| `SPEED_SEARCH` | `160` | Spin search speed |
| `SPEED_STALK` | `180` | Approach speed during bait phase |
| `SPEED_ARC_OUT` | `220` | Outer wheel speed during arc maneuver |
| `SPEED_ARC_IN` | `80` | Inner wheel speed during arc (tighter = smaller `SPEED_ARC_IN`) |
| `MATADOR_SIDESTEP_MS` | `320UL` | Dodge duration — increase if opponent clips you |
| `FLANKER_ARC_MS` | `500UL` | Arc sweep duration — increase for wider flanking path |
| `START_DELAY_MS` | `5000UL` | 5 seconds for competition, `1000UL` for practice |

---

## 11. Troubleshooting

- **ESP32 crashes instantly on motor start (Guru Meditation Error):** Add a 1000µF 16V capacitor across the L298N 5V output and GND to prevent brownout from motor current spikes crashing the LDO.
- **`ECHO` pin causes random reboots:** Confirm the 1kΩ + 2kΩ divider is in place. GPIO pins exposed to 5V will cause intermittent resets or permanent damage.
- **Robot doesn't respond to switch:** GPIO34–39 on ESP32 are input-only and have **no internal pull-up**. This build uses GPIO13 which has internal pull-up support. If using a different GPIO, add an external 10kΩ pull-up resistor.
- **FreeRTOS sensor task crashes (stack overflow):** Increase the stack size in `esp32_sumo.ino` from `4096` to `8192` in the `xTaskCreatePinnedToCore` call.
- **Upload fails:** Hold the **BOOT** button on the ESP32 when the Arduino IDE console shows `Connecting...`. Release after upload starts.
