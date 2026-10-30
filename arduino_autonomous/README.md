# Arduino Nano Autonomous Sumo Robot

This folder contains the **Arduino Nano version** of the autonomous sumo robot.
It has the exact same brain (FSM) as the Raspberry Pi version but runs on an
Arduino Nano instead. No Linux, no Python, no SSH needed — just upload the
code and it runs.

---

## Table of Contents

1. [What Is Different From the Pi Version](#1-what-is-different-from-the-pi-version)
2. [Parts List](#2-parts-list)
3. [How the Power Works](#3-how-the-power-works)
4. [Complete Wiring Guide](#4-complete-wiring-guide)
5. [The Rocker Switch and LED](#5-the-rocker-switch-and-led)
6. [Protecting Nothing — No Voltage Divider Needed](#6-protecting-nothing--no-voltage-divider-needed)
7. [What the Code Files Do](#7-what-the-code-files-do)
8. [How the Brain Works](#8-how-the-brain-works)
9. [Installing Arduino IDE and Uploading the Code](#9-installing-arduino-ide-and-uploading-the-code)
10. [Testing Everything Before a Match](#10-testing-everything-before-a-match)
11. [Tuning Guide](#11-tuning-guide)
12. [Troubleshooting](#12-troubleshooting)

---

## 1. What Is Different From the Pi Version

| Thing | Raspberry Pi Version | Arduino Nano Version |
|-------|---------------------|---------------------|
| Language | Python | C++ (Arduino) |
| Operating system | Raspberry Pi OS Linux | None — bare metal |
| Boot time | ~60 seconds | ~1 second |
| Upload method | SSH over Wi-Fi | USB cable to laptop |
| Voltage divider on HC-SR04 ECHO | YES (Pi GPIO is 3.3V) | NO (Nano GPIO is 5V) |
| Power for logic | MT3608 boost converter | Direct from battery pack via Nano VIN pin |
| Bluetooth | Built-in | Not used in this folder |
| Sleep prevention | setup_pi.sh script | Not needed (Nano never sleeps) |
| Watchdog | Hardware watchdog daemon | Built-in hardware watchdog via wdt.h |

The **algorithm is identical** — same 7 states, same adaptive tactic system,
same opponent tracker, same edge avoidance logic.

---

## 2. Parts List

| # | Part | Qty | Notes |
|---|------|-----|-------|
| 1 | Arduino Nano (ATmega328P) | 1 | The brain |
| 2 | GA25-370 DC Gear Motor with Encoder (12V, 130RPM) | 2 | Left and right drive motors |
| 3 | Robot Wheel 65mm × 26mm + 4mm metal coupler | 2 | Drive wheels |
| 4 | Metal caster wheel | 1 | Front support wheel |
| 5 | L298N Dual H-Bridge Motor Driver Module | 1 | Motor driver |
| 6 | 18650 3.7V 1200mAh button-top Li-ion battery | 3 | In series → ~11.1V |
| 7 | 3-slot 18650 series battery holder | 1 | Battery pack |
| 8 | HC-SR04 Ultrasonic Sensor | 1 | Front opponent detection |
| 9 | Downward IR Line Sensor (3-pin digital) | 2 | Front and back boundary detection |
| 10 | Black rocker switch (2-pin ON/OFF) | 1 | Arms and disarms the robot |
| 11 | LED (any colour, 3mm or 5mm) | 1 | Status indicator |
| 12 | 330Ω resistor | 1 | Protects the LED |

> No MT3608 boost converter needed for this version.
> The Arduino Nano's onboard regulator accepts 7–12V on its VIN pin
> and outputs a clean 5V for all logic and sensors.

---

## 3. How the Power Works

This is simpler than the Pi version because the Arduino Nano has its own
built-in voltage regulator.

```
3× 18650 in series (~11.1V pack)
         │
         ├──────────────────────────────► L298N 12V terminal
         │                                └──► Both GA25-370 motors
         │
         └──────────────────────────────► Arduino Nano VIN pin
                                          └──► Nano onboard regulator
                                               └──► 5V output
                                                    ├──► HC-SR04 VCC
                                                    ├──► Both IR sensors VCC
                                                    └──► Both encoder VCC (green wire)

Common GND rail:
  Battery (−) ── L298N GND ── Nano GND ── All sensor GNDs
```

### Why This Is Safe

The Arduino Nano's onboard regulator (AMS1117 or similar) accepts up to
**12V on VIN** and outputs a stable **5V**. Your 11.1V pack is within spec.

> ⚠️ The Nano's onboard regulator can get warm under load. This is normal.
> If it gets very hot, add a small heatsink or use a separate 5V buck
> converter (like LM2596) instead of the VIN pin.

> ⚠️ Do NOT connect the 11.1V battery pack to the Nano's 5V pin directly.
> Only connect it to the VIN pin. The 5V pin bypasses the regulator and
> will instantly destroy the Nano.

---

## 4. Complete Wiring Guide

### Arduino Nano Pin Assignments

```
Nano Pin │ Label │ Connected To
─────────┼───────┼──────────────────────────────────────────────────────
VIN      │  —    │ Battery pack (+) ~11.1V
GND      │  —    │ Common GND rail
5V       │  —    │ HC-SR04 VCC, both IR sensors VCC, both encoder VCC (green)
D2       │  —    │ Left  encoder Channel A — yellow wire  (interrupt INT0)
D3       │  —    │ Right encoder Channel A — yellow wire  (interrupt INT1)
D4       │  —    │ L298N IN3  (Right motor direction A)
D5       │  —    │ L298N IN4  (Right motor direction B)
D6       │  —    │ L298N ENB  (Right motor PWM speed)
D7       │  —    │ L298N IN1  (Left motor direction A)
D8       │  —    │ L298N IN2  (Left motor direction B)
D9       │  —    │ L298N ENA  (Left motor PWM speed)
D10      │  —    │ Rocker switch (other pin → GND)
D11      │  —    │ LED anode via 330Ω resistor → GND
A0       │  —    │ Left  encoder Channel B — white wire
A1       │  —    │ Right encoder Channel B — white wire
A2       │  —    │ HC-SR04 TRIG
A3       │  —    │ HC-SR04 ECHO  (connect directly — no resistor divider needed)
A4       │  —    │ Front downward IR sensor OUT
A5       │  —    │ Back  downward IR sensor OUT
```

### GA25-370 Motor — All 6 Wires

```
Wire Colour │ What It Is        │ Where It Goes
────────────┼───────────────────┼──────────────────────────────────────
Red         │ Motor power +     │ L298N OUT1 (left) / OUT3 (right)
Black       │ Motor power −     │ L298N OUT2 (left) / OUT4 (right)
Green       │ Encoder VCC 5V    │ Nano 5V pin
Dark Blue   │ Encoder GND       │ Common GND rail
Yellow      │ Encoder Channel A │ Nano D2 (left motor) / D3 (right motor)
White       │ Encoder Channel B │ Nano A0 (left motor) / A1 (right motor)
```

> The yellow wires MUST go to D2 and D3. These are the only two
> interrupt-capable pins on the Arduino Nano. The encoder counting
> will not work correctly on any other pins.

### L298N Motor Driver

```
L298N Terminal │ Connected To
───────────────┼──────────────────────────────────────────
12V            │ Battery pack (+) ~11.1V
GND            │ Common GND rail
5V output      │ NOT USED — leave unconnected
IN1            │ Nano D7
IN2            │ Nano D8
IN3            │ Nano D4
IN4            │ Nano D5
ENA            │ Nano D9  (REMOVE the ENA jumper first)
ENB            │ Nano D6  (REMOVE the ENB jumper first)
OUT1 / OUT2    │ Left  motor red / black wires
OUT3 / OUT4    │ Right motor red / black wires
```

> ⚠️ REMOVE the yellow ENA and ENB jumpers from the L298N board before
> wiring. If you leave them in, the motors always run at full speed and
> the Nano cannot control their speed at all.

### HC-SR04 Ultrasonic Sensor

```
HC-SR04 Pin │ Connected To
────────────┼──────────────────────────────
VCC         │ Nano 5V
GND         │ Common GND rail
TRIG        │ Nano A2
ECHO        │ Nano A3  (direct — no resistor divider needed!)
```

> Unlike the Raspberry Pi version, you do NOT need a resistor voltage
> divider on the ECHO pin. The Pi's GPIO is 3.3V max so it needed
> protection. The Nano's GPIO is 5V — same as the HC-SR04 output.
> Connect ECHO directly to A3.

### IR Edge Sensors (×2)

```
IR Sensor   │ Pin  │ Connected To
────────────┼──────┼──────────────────────────────
Front sensor│ VCC  │ Nano 5V
            │ GND  │ Common GND rail
            │ OUT  │ Nano A4
────────────┼──────┼──────────────────────────────
Back sensor │ VCC  │ Nano 5V
            │ GND  │ Common GND rail
            │ OUT  │ Nano A5
```

Both sensors face **downward** toward the floor. They detect the white
boundary ring of the sumo arena.

### Rocker Switch and LED

```
Component      │ Connection
───────────────┼──────────────────────────────────────────────
Rocker switch  │ One pin → Nano D10
               │ Other pin → GND
LED (+) anode  │ Nano D11 → 330Ω resistor → LED (+)
LED (−) cathode│ LED (−) → GND
```

---

## 5. The Rocker Switch and LED

The rocker switch works exactly the same as the Pi version:

```
Nano D10 ──── [ROCKER SWITCH] ──── GND
```

- Switch **ON**  → D10 reads LOW  (closed to GND, internal pull-up active)
- Switch **OFF** → D10 reads HIGH (open circuit, pull-up holds HIGH)

| LED State | What It Means |
|-----------|---------------|
| OFF | Switch is OFF — robot is safe to handle |
| SLOW BLINK (200ms on / 800ms off) | Waiting for switch to be turned ON |
| FAST BLINK (100ms on / 100ms off) | Switch ON — 5-second start delay |
| SOLID ON | Robot is fully armed and competing |

### Competition Procedure

1. Place robot on the ring, keep your hand on it
2. Flip rocker switch ON
3. LED blinks fast — 5-second countdown
4. LED goes solid ON — release the robot immediately
5. Robot is now autonomous and competing
6. Flip switch OFF at any time to stop instantly

---

## 6. Protecting Nothing — No Voltage Divider Needed

In the Raspberry Pi version, the HC-SR04 ECHO pin needed a resistor
voltage divider because the Pi's GPIO pins are 3.3V maximum and the
ECHO signal is 5V.

The Arduino Nano runs at **5V logic** — the same voltage as the HC-SR04.
So you connect the ECHO pin **directly** to Nano A3. No resistors needed
on the ECHO line.

```
Pi version:    HC-SR04 ECHO → 1kΩ → junction → Pi GPIO  (needed!)
                                         │
                                        2kΩ
                                         │
                                        GND

Nano version:  HC-SR04 ECHO ──────────► Nano A3  (direct, no divider)
```

---

## 7. What the Code Files Do

```
arduino_autonomous.ino  ← Main sketch. Setup, loop, switch, LED.
config.h                ← All pin numbers and constants. Edit only this file.
motors.h / motors.cpp   ← L298N control + quadrature encoder ISRs.
sensors.h / sensors.cpp ← HC-SR04 distance + IR edge detection + opponent tracker.
strategy.h / strategy.cpp ← Full adaptive FSM (7 states, 3 tactics).
```

### Why Multiple Files?

Arduino sketches can be split into .h (header) and .cpp (implementation)
files. This keeps the code organised — each file has one job, just like
the Python version. The Arduino IDE automatically compiles all files in
the same folder together.

---

## 8. How the Brain Works

The algorithm is identical to the Raspberry Pi version. Here is a quick
summary:

### 7 States (highest priority first)

```
┌──────────────────────────────────────────────────────────────────┐
│ 1. EDGE_AVOID  ← ALWAYS wins — white line detected              │
│    Reverse away from boundary, spin to face ring centre.         │
├──────────────────────────────────────────────────────────────────┤
│ 2. RAM                                                           │
│    Opponent closer than 25cm — short full-speed burst push.      │
├──────────────────────────────────────────────────────────────────┤
│ 3. ATTACK                                                        │
│    Opponent within 80cm — full speed charge with encoder-based   │
│    heading correction to keep driving straight.                  │
├──────────────────────────────────────────────────────────────────┤
│ 4. STALK                                                         │
│    Opponent visible at 80–150cm — arc toward it carefully.       │
├──────────────────────────────────────────────────────────────────┤
│ 5. ORBIT                                                         │
│    FLANKER tactic only — arc wide to approach from the side.     │
├──────────────────────────────────────────────────────────────────┤
│ 6. SEARCH                                                        │
│    No opponent — spin 360° to scan. Reverses direction after     │
│    each full rotation to avoid tangling wires.                   │
├──────────────────────────────────────────────────────────────────┤
│ 7. START  ← lowest priority — only at match start               │
│    Mandatory 5-second pause. Robot sits still. LED blinks.       │
└──────────────────────────────────────────────────────────────────┘
```

### Adaptive Tactic System

| Style | Behaviour | Switches To This When |
|-------|-----------|----------------------|
| RUSHER | Charge immediately at full speed | Default. Also when pushes succeed 2+ times |
| STALKER | Approach carefully with arcs | Moderate edge pressure |
| FLANKER | Orbit wide, attack from side | Pushed to edge 3+ times |
| RUSHER (forced) | Charge regardless | Match time > 150 seconds |

---

## 9. Installing Arduino IDE and Uploading the Code

### Step 1 — Install Arduino IDE

1. Go to https://www.arduino.cc/en/software
2. Download **Arduino IDE 2** for your operating system
3. Install it

### Step 2 — Connect the Nano

1. Plug the Arduino Nano into your laptop with a **USB Mini-B cable**
2. Open Arduino IDE
3. Go to **Tools → Board → Arduino AVR Boards → Arduino Nano**
4. Go to **Tools → Processor → ATmega328P**
   - If upload fails, try **ATmega328P (Old Bootloader)**
5. Go to **Tools → Port** and select the COM port that appeared
   (on Windows it will be something like COM3 or COM4)

### Step 3 — Open the Project

1. In Arduino IDE, go to **File → Open**
2. Navigate to the `arduino_autonomous` folder
3. Open `arduino_autonomous.ino`
4. Arduino IDE will automatically load all the other files
   (config.h, motors.h, motors.cpp, sensors.h, sensors.cpp,
   strategy.h, strategy.cpp) because they are in the same folder

### Step 4 — Upload

1. Click the **Upload** button (right arrow icon) or press Ctrl+U
2. Wait for "Done uploading" message at the bottom
3. The Nano will restart automatically and run the code

> The robot will immediately wait for the rocker switch to be turned ON.
> The LED will blink slowly. This is normal.

---

## 10. Testing Everything Before a Match

### Test 1 — Switch and LED

Power on the robot. LED should blink slowly (waiting for switch).
Flip the switch ON. LED should blink fast (5-second delay).
After 5 seconds, LED goes solid. Flip switch OFF — LED goes off immediately.

### Test 2 — Motors

With the robot lifted off the ground, flip the switch ON.
After the 5-second delay, the robot should start spinning (SEARCH state).
Both wheels should spin. If one goes backward, swap that motor's red and
black wires on the L298N output terminals.

### Test 3 — Ultrasonic Sensor

With the robot in SEARCH state (spinning), hold your hand in front of the
HC-SR04 sensor. The robot should immediately stop spinning and charge
toward your hand (ATTACK state). Remove your hand — robot should go back
to spinning (SEARCH state).

### Test 4 — IR Edge Sensors

Place the robot near the white boundary line of the arena. When the front
IR sensor crosses the white line, the robot should reverse and spin away.
When the back IR sensor crosses the white line, the robot should drive
forward and spin away.

Each IR sensor has a small potentiometer (tiny screw) on the board.
Adjust it until:
- LED on sensor board turns **ON** over the black dohyo surface
- LED on sensor board turns **OFF** over the white boundary ring

### Test 5 — Encoders

During ATTACK state (charging at your hand), the robot should drive in a
straight line. If it curves left or right, the encoders are correcting it.
If it curves badly in one direction, swap that motor's red/black wires on
the L298N to reverse its direction.

---

## 11. Tuning Guide

All tuning is done in **config.h only**. Never change numbers in other files.

### Speed Values (0–255, where 255 = full speed)

| Constant | Default | What It Does |
|----------|---------|-------------|
| `SPEED_FULL` | 255 | Attack charge speed |
| `SPEED_SEARCH` | 140 | Spin speed while scanning |
| `SPEED_STALK` | 178 | Approach speed for far opponent |
| `SPEED_REVERSE` | 217 | Reverse speed during edge avoidance |
| `SPEED_TURN` | 204 | Spin speed during edge avoidance |
| `SPEED_CORRECT` | 102 | Heading correction strength |

### Distance Values (cm)

| Constant | Default | What It Does |
|----------|---------|-------------|
| `ATTACK_DISTANCE_CM` | 80 | Charge when opponent closer than this |
| `STALK_DISTANCE_CM` | 150 | Track opponent when closer than this |
| `CLOSE_DISTANCE_CM` | 25 | Trigger RAM burst when closer than this |

### Timing Values (milliseconds)

| Constant | Default | What It Does |
|----------|---------|-------------|
| `START_DELAY_MS` | 5000 | Pre-match pause (5 seconds) |
| `REVERSE_DURATION_MS` | 300 | How long to reverse on edge detection |
| `SPIN_DURATION_MS` | 450 | How long to spin after reversing |
| `SCAN_STEP_MS` | 550 | Time per scan step |
| `LOST_TIMEOUT_MS` | 1200 | How long to remember opponent after losing sight |
| `RAM_BURST_MS` | 180 | Duration of close-range burst push |
| `LOOP_MS` | 20 | Main loop period (20ms = 50Hz) |

---

## 12. Troubleshooting

### Robot does not move at all
- Check rocker switch is ON (LED should be blinking or solid)
- Check 5-second delay has finished (LED solid)
- Check battery voltage — 3S pack should read ~11.1V
- Check L298N ENA/ENB jumpers are removed

### Robot spins but does not charge at opponent
- HC-SR04 may not be detecting — check TRIG→A2 and ECHO→A3 wiring
- Make sure ECHO is connected directly (no resistor divider on Nano version)
- Hold your hand 30cm in front of the sensor — robot should charge

### Robot falls off the ring
- IR sensors need calibration — adjust the potentiometer on each sensor board
- Increase `REVERSE_DURATION_MS` in config.h
- Increase `SPIN_DURATION_MS` in config.h

### Robot drives in circles instead of straight
- Encoder wiring issue — yellow wires MUST be on D2 and D3
- Check white (Channel B) wires are on A0 and A1
- If still curving, swap red/black wires on the weaker motor's L298N terminals

### Upload fails in Arduino IDE
- Try **Tools → Processor → ATmega328P (Old Bootloader)**
- Try a different USB cable (some cables are charge-only, no data)
- Check the correct COM port is selected under Tools → Port

### Nano gets very hot
- The onboard regulator is working hard — this is normal under load
- If it shuts down from heat, add a small heatsink to the regulator chip
- Or replace with a separate LM2596 buck converter for the 5V supply

---

## License
MIT — free to use, modify, and distribute for educational and competition purposes.
