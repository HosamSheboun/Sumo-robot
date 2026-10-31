# Arduino Uno Sumo Robot — 2-Minute Technique System

This folder contains the complete Arduino Uno code for the autonomous sumo robot.

The robot uses a **timed technique system** — it automatically switches fighting
style every 30 seconds throughout the 2-minute match. Each phase is designed to
beat a different type of opponent.

---

## Table of Contents

1. [Parts List](#1-parts-list)
2. [How the Power Works](#2-how-the-power-works)
3. [Complete Wiring Guide](#3-complete-wiring-guide)
4. [The Rocker Switch and LED](#4-the-rocker-switch-and-led)
5. [The 2-Minute Technique System](#5-the-2-minute-technique-system)
6. [What Each Code File Does](#6-what-each-code-file-does)
7. [Installing Arduino IDE and Uploading](#7-installing-arduino-ide-and-uploading)
8. [Testing Before a Match](#8-testing-before-a-match)
9. [Tuning Guide](#9-tuning-guide)
10. [Troubleshooting](#10-troubleshooting)

---

## 1. Parts List

| # | Part | Qty | Notes |
|---|------|-----|-------|
| 1 | Arduino Uno (ATmega328P) | 1 | The brain |
| 2 | GA25-370 DC Gear Motor with Encoder (12V, 130RPM) | 2 | Left and right drive |
| 3 | Robot Wheel 65mm × 26mm + 4mm metal coupler | 2 | Drive wheels |
| 4 | Metal caster wheel | 1 | Front support |
| 5 | L298N Dual H-Bridge Motor Driver Module | 1 | Motor driver |
| 6 | HC-SR04 Ultrasonic Sensor | 1 | Detects the opponent |
| 7 | 18650 3.7V 1200mAh button-top Li-ion battery | 3 | In series → ~11.1V |
| 8 | 3-slot 18650 series battery holder | 1 | Battery pack |
| 9 | Black rocker switch (2-pin ON/OFF) | 1 | Arms and disarms the robot |
| 10 | LED (any colour, 3mm or 5mm) | 1 | Status indicator |
| 11 | 330Ω resistor | 1 | Protects the LED from burning out |

> No MT3608 converter needed. No IR sensors needed. No resistor divider needed
> for the HC-SR04 — the Arduino Uno is 5V logic, same as the sensor.

---

## 2. How the Power Works

```
3× 18650 in series (~11.1V)
         │
         ├──────────────────────────► L298N 12V terminal
         │                            └──► Both GA25-370 motors (full 11.1V)
         │
         └──────────────────────────► Arduino Uno VIN pin
                                       └──► Uno onboard regulator → 5V
                                            ├──► Uno logic
                                            ├──► HC-SR04 VCC
                                            └──► Both encoder green wires (VCC)

Common GND rail:
  Battery (−) ── L298N GND ── Uno GND ── HC-SR04 GND ── Both encoder dark blue wires
```

### Why the Uno VIN Pin Works Here

The Arduino Uno has a built-in voltage regulator on its VIN pin that accepts
7V to 12V and outputs a clean 5V. Your 11.1V battery pack is within this range.

> ⚠️ Connect the battery to the **VIN pin** only — never to the 5V pin.
> The 5V pin bypasses the regulator and will instantly destroy the Uno.

> ⚠️ The Uno's onboard regulator will get warm under load. This is normal.
> If it gets very hot, use a separate LM2596 buck converter set to 7–8V into VIN.

---

## 3. Complete Wiring Guide

### Arduino Uno Pin Assignments

```
Uno Pin  │ Connected To
─────────┼──────────────────────────────────────────────────────────────
VIN      │ Battery pack (+) ~11.1V
GND      │ Common GND rail
5V       │ HC-SR04 VCC + both encoder green wires
D4       │ L298N IN3  (right motor direction bit A)
D5       │ L298N IN4  (right motor direction bit B)
D7       │ L298N IN1  (left motor direction bit A)
D8       │ L298N IN2  (left motor direction bit B)
D10      │ Rocker switch (other pin → GND)
D11      │ HC-SR04 TRIG
D12      │ HC-SR04 ECHO
D13      │ LED anode via 330Ω resistor → GND
```

> D3, D6, D9 are now free — ENA and ENB jumpers on the L298N are ON,
> so no PWM pins are needed for motor speed control.

### HC-SR04 Ultrasonic Sensor

Mount the sensor at the front of the robot, facing forward.

```
HC-SR04 Pin │ Connected To
────────────┼──────────────────────────────────────────────────────────
VCC         │ Uno 5V
GND         │ Common GND rail
TRIG        │ Uno D11
ECHO        │ Uno D12
```

> No resistor divider needed. Uno and HC-SR04 are both 5V logic.

### GA25-370 Motor — All 6 Wires

```
Wire Colour │ What It Is        │ Where It Goes
────────────┼───────────────────┼────────────────────────────────────────
Red         │ Motor power +     │ L298N OUT1 (left) / OUT3 (right)
Black       │ Motor power −     │ L298N OUT2 (left) / OUT4 (right)
Green       │ Encoder VCC 5V    │ Uno 5V pin
Dark Blue   │ Encoder GND       │ Common GND rail
Yellow      │ Encoder Channel A │ Leave unconnected (not used)
White       │ Encoder Channel B │ Leave unconnected (not used)
```

> Green and dark blue MUST be connected even though the encoder signals
> are unused. If left floating, the encoder board can cause power rail noise.

### L298N Motor Driver — All Terminals

```
L298N Terminal │ Connected To                      │ Notes
───────────────┼───────────────────────────────────┼──────────────────────────
12V            │ Battery pack (+) ~11.1V            │ Motor power supply
GND            │ Common GND rail                    │ Ground reference
5V output      │ NOT USED — leave unconnected       │ Too noisy for logic
IN1            │ Uno D7                             │ Left motor direction A
IN2            │ Uno D8                             │ Left motor direction B
IN3            │ Uno D4                             │ Right motor direction A
IN4            │ Uno D5                             │ Right motor direction B
ENA            │ JUMPER ON — no wire needed         │ Left motor always enabled
ENB            │ JUMPER ON — no wire needed         │ Right motor always enabled
OUT1 / OUT2    │ Left motor red / black wires       │ Power to left motor
OUT3 / OUT4    │ Right motor red / black wires      │ Power to right motor
```

> ⚠️ The ENA and ENB jumpers must be **ON** (installed) for this build.
> This permanently enables both motors at full power.
> If you previously removed them for PWM control, put them back in now.

### Rocker Switch

```
Uno D10 ──── [ROCKER SWITCH] ──── GND
```

### LED

```
Uno D13 ──── 330Ω resistor ──── LED (+) long leg ──── LED (−) short leg ──── GND
```

> D13 also drives the Uno's built-in LED — you can test without an external LED.

---

## 4. The Rocker Switch and LED

| LED State | What It Means |
|-----------|---------------|
| OFF | Switch is OFF — robot is safe to handle |
| SLOW BLINK (200ms on / 800ms off) | Waiting for switch to be turned ON |
| FAST BLINK (100ms on / 100ms off) | Start delay counting down |
| SOLID ON | Robot is active and competing |

### Step-by-Step at a Competition

1. Place the robot on the ring — keep your hand on it
2. Flip the rocker switch to ON
3. LED blinks fast — start delay counting down
4. When LED goes solid ON — release the robot immediately
5. Robot begins Phase 1 (Matador) automatically
6. Flip switch OFF at any time to stop instantly
7. Flip switch back ON to restart from Phase 1 with a fresh timer

---

## 5. The 2-Minute Technique System

The robot automatically switches fighting style every 30 seconds.
You do not need to do anything — it changes on its own.

### Phase 1 — MATADOR (0:00 – 0:30)

```
Goal: make the opponent charge at you, then dodge and hit it from the side.

No opponent seen  → spin right to search
Opponent far away → creep forward slowly to bait a charge
Opponent charges  → sidestep right (let it run past), spin to face it,
                    slam into its side at full speed
Opponent close
but not charging  → attack directly

After 4 successful dodges, switches to direct attacks for the rest of this phase.
```

This works well against aggressive bots that charge straight at you.
You use their own momentum against them.

### Phase 2 — BULLDOZER (0:30 – 1:00)

```
Goal: cover the whole ring and push anything you find.

Opponent found    → charge and hold — keep pushing until it goes over the edge
No opponent       → wander in a 6-step pattern to cover the ring:
                    forward → spin right → forward → spin left → forward → spin right
                    (repeats)
```

This works well against passive bots that sit still or hide near the edge.
The wander pattern ensures the robot covers the whole ring instead of
spinning in one spot.

### Phase 3 — FLANKER (1:00 – 1:30)

```
Goal: approach from the side instead of head-on.

No opponent seen  → spin right to search
Opponent visible  → arc wide to the right around the opponent,
                    re-read the sensor after the arc,
                    if opponent still there → slam into its side
Opponent far away → drive forward to close the gap
```

This works well against strong bots with powerful front pushers.
Hitting the side bypasses their strongest pushing direction.

### Phase 4 — BERSERKER (1:30 – 2:00)

```
Goal: pure aggression with unpredictability. Last 30 seconds — no time for subtlety.

No opponent seen  → spin right to search
Opponent found    → every 3rd attack: reverse briefly then charge immediately
                    (fake retreat — confuses bots that try to dodge or counter)
                    other attacks: direct full-speed charge
Opponent far away → drive forward fast to close the gap
```

This works well against any opponent. The fake retreat tricks bots that
react to your movement — they start to move away and you charge into them.

---

## 6. What Each Code File Does

```
arduino_autonomous.ino  ← Main sketch. All strategy logic lives here.
                          Organised into 9 clearly labelled sections.

config.h                ← Every pin number, distance threshold, and timing
                          constant. This is the ONLY file you edit to tune
                          the robot. Nothing is hardcoded in other files.

motors.h                ← Declarations for all 7 motor functions.

motors.cpp              ← L298N implementation:
                          motorForward, motorReverse, motorSpinLeft,
                          motorSpinRight, motorArcLeft, motorArcRight,
                          motorStop.
```

### The 7 Motor Movements

| Function | Left Wheel | Right Wheel | Result |
|----------|-----------|-------------|--------|
| motorForward() | Forward | Forward | Drive straight ahead |
| motorReverse() | Backward | Backward | Drive straight back |
| motorSpinLeft() | Backward | Forward | Pivot left in place |
| motorSpinRight() | Forward | Backward | Pivot right in place |
| motorArcLeft() | Stop | Forward | Curve left (wide turn) |
| motorArcRight() | Forward | Stop | Curve right (wide turn) |
| motorStop() | Stop | Stop | Coast to a halt |

---

## 7. Installing Arduino IDE and Uploading

### Step 1 — Install Arduino IDE

1. Go to https://www.arduino.cc/en/software
2. Download **Arduino IDE 2** for your operating system
3. Install it

### Step 2 — Connect the Uno

Plug the Arduino Uno into your laptop using a **USB Type-B cable**
(the square-ish connector, not Micro-USB).

> If the Uno does not appear in the port list, try a different cable —
> some cheap cables carry power only and no data.

### Step 3 — Configure Arduino IDE

1. Open Arduino IDE
2. Go to **Tools → Board → Arduino AVR Boards → Arduino Uno**
3. Go to **Tools → Port** → select the COM port that appeared

### Step 4 — Open and Upload

1. Go to **File → Open**
2. Navigate to the `arduino_autonomous` folder
3. Open `arduino_autonomous.ino`
4. Arduino IDE automatically loads config.h, motors.h, and motors.cpp
5. Click the **Upload** button (→) or press **Ctrl+U**
6. Wait for "Done uploading"

> If upload fails, try a different USB cable or check the correct port is selected.

---

## 8. Testing Before a Match

Always test each part separately before competing.

### Test 1 — Switch and LED

1. Power on the robot (connect battery)
2. LED blinks slowly — waiting for switch
3. Flip switch ON — LED blinks fast for 1 second (or 5 seconds for competition)
4. LED goes solid ON — robot is now active
5. Flip switch OFF — LED goes off, motors stop immediately

### Test 2 — Motors (lift robot off ground first)

1. Flip switch ON, wait for LED to go solid
2. Robot should start spinning right (Phase 1 search behaviour)
3. Hold your hand in front of the sensor within 80cm
4. Robot should stop spinning and start the matador sequence
5. If a wheel spins the wrong direction — swap that motor's red and black
   wires on the L298N output terminals (OUT1/OUT2 or OUT3/OUT4)
6. Do NOT change any code — physical wire swap is always the correct fix

### Test 3 — Ultrasonic Sensor

1. Flip switch ON, wait for LED to go solid
2. Robot spins — hold your hand in front within 80cm
3. Robot should react (stop spinning, start matador sequence)
4. Move hand away — robot should go back to spinning
5. If no reaction: check TRIG on D11 and ECHO on D12, check HC-SR04 VCC on Uno 5V

### Test 4 — All 7 Motor Movements

Lift the robot off the ground. Upload this quick test sketch temporarily
to verify every movement direction is correct:

```cpp
#include "motors.h"
void setup() {
    motorsInit();
    motorForward();   delay(600); motorStop(); delay(400);
    motorReverse();   delay(600); motorStop(); delay(400);
    motorSpinLeft();  delay(600); motorStop(); delay(400);
    motorSpinRight(); delay(600); motorStop(); delay(400);
    motorArcLeft();   delay(600); motorStop(); delay(400);
    motorArcRight();  delay(600); motorStop(); delay(400);
}
void loop() {}
```

Expected behaviour for each:
- Forward: both wheels spin forward
- Reverse: both wheels spin backward
- SpinLeft: left wheel backward, right wheel forward
- SpinRight: left wheel forward, right wheel backward
- ArcLeft: left wheel stops, right wheel forward
- ArcRight: left wheel forward, right wheel stops

---

## 9. Tuning Guide

All tuning is done by editing **config.h only**.

### Sensor Thresholds

| Constant | Default | What Changing It Does |
|----------|---------|----------------------|
| `ATTACK_DIST_CM` | 80 | Distance to trigger attack. Increase to attack from further away |
| `NO_DETECT_CM` | 200 | Max detection range. Leave at 200 for a standard ring |
| `CHARGE_SPEED_CM_S` | 55 | How fast opponent must close to trigger a matador dodge. Lower = dodges more easily |

### Timing Constants

| Constant | Default | What Changing It Does |
|----------|---------|----------------------|
| `START_DELAY_MS` | 1000 | Pre-match pause. Set to 5000 for competition rules |
| `SPIN_STEP_MS` | 500 | Search spin duration. Lower if it spins past the opponent |
| `MATADOR_SIDESTEP_MS` | 380 | How far the robot sidesteps during a dodge |
| `MATADOR_REFACE_MS` | 280 | How long it spins to face the opponent after dodging |
| `MATADOR_SLAM_MS` | 550 | How long the counter-attack charge lasts |
| `MATADOR_BAIT_MS` | 200 | How long it creeps forward to bait a charge |
| `MATADOR_MAX_DODGES` | 4 | Max dodges before switching to direct attacks |
| `FLANKER_ARC_MS` | 600 | How wide the flanker arc is. Increase for a wider approach angle |
| `FLANKER_SLAM_MS` | 500 | How long the flanker slam lasts |
| `BERSERKER_RETREAT_MS` | 180 | How long the fake retreat reversal lasts |
| `BERSERKER_CHARGE_MS` | 600 | How long the charge after fake retreat lasts |

### Phase Boundaries

| Constant | Default | What Changing It Does |
|----------|---------|----------------------|
| `PHASE1_END_MS` | 30000 | When Matador ends and Bulldozer begins |
| `PHASE2_END_MS` | 60000 | When Bulldozer ends and Flanker begins |
| `PHASE3_END_MS` | 90000 | When Flanker ends and Berserker begins |

---

## 10. Troubleshooting

### Robot does not move at all
- Check rocker switch is ON (LED should be blinking or solid)
- Check battery voltage — 3S pack should read ~11.1V fully charged
- Check ENA and ENB **jumpers are ON** (installed) on the L298N
- Check all GND wires share a common ground rail
- Check D4, D5, D7, D8 are connected to the correct L298N IN terminals

### Robot spins but never attacks
- Hold your hand in front of the sensor within 80cm — if no reaction:
- Check HC-SR04 VCC is on Uno 5V (not 3.3V — Uno has no 3.3V pin anyway)
- Check TRIG is on D11 and ECHO is on D12
- Try increasing `ATTACK_DIST_CM` to 120 in config.h

### Robot never dodges in Phase 1
- The opponent may not be closing fast enough to trigger the dodge
- Lower `CHARGE_SPEED_CM_S` in config.h (e.g. 30)
- Make sure the sensor is getting stable readings

### Robot dodges when it shouldn't
- Raise `CHARGE_SPEED_CM_S` (e.g. 80) so it only dodges a real fast charge

### One motor spins the wrong direction
- Swap that motor's red and black wires on the L298N output terminals
- Do not change any code — physical wire swap is always the correct fix

### Robot curves instead of going straight
- One motor is slightly stronger than the other — normal for L298N
- Physically adjust the motor mounting or add a small shim to one wheel

### Upload fails
- Try a different USB cable
- Check the correct COM port is selected under Tools → Port
- Make sure **Arduino Uno** is selected under Tools → Board

### Uno gets very hot on VIN
- The onboard regulator dissipates the voltage difference as heat
- Use a separate LM2596 buck converter set to 7–8V into VIN instead of 11.1V

---

## License
MIT — free to use, modify, and distribute for educational and competition purposes.
