# Pi Zero 2 W Autonomous Heavyweight Sumo Robot

---

## What Is This?

This is a **fully autonomous robot** that competes in sumo wrestling matches.
It has no remote control — once you flip the switch, it thinks and acts entirely
on its own. It uses sensors to find its opponent, drives toward it at full speed,
and tries to push it out of the ring — all by itself.

The robot is built around a **Raspberry Pi Zero 2 W**, which is a tiny computer
about the size of a stick of gum. It runs Python code that makes all the
decisions: where to go, how fast, when to attack, and when to back away from
the edge of the ring.

**Weight limit:** ≤ 3 kg
**Size limit:** ≤ 20 × 20 cm
**Arena:** Black circular dohyo with a white boundary ring on the edge

---

## Table of Contents

1. [Parts List — Everything You Need](#1-parts-list--everything-you-need)
2. [How the Power Works](#2-how-the-power-works)
3. [Wiring Guide — Step by Step](#3-wiring-guide--step-by-step)
4. [The Rocker Switch & LED](#4-the-rocker-switch--led)
5. [Protecting the Pi from the Ultrasonic Sensor](#5-protecting-the-pi-from-the-ultrasonic-sensor)
6. [How the Software Works](#6-how-the-software-works)
7. [How the Brain (FSM) Makes Decisions](#7-how-the-brain-fsm-makes-decisions)
8. [Setting Up the Raspberry Pi](#8-setting-up-the-raspberry-pi)
9. [Installing the Code](#9-installing-the-code)
10. [Making the Robot Start Automatically on Power-Up](#10-making-the-robot-start-automatically-on-power-up)
11. [Keeping the Pi Awake Forever](#11-keeping-the-pi-awake-forever)
12. [Testing Everything Before a Match](#12-testing-everything-before-a-match)
13. [Tuning Guide — Making the Robot Better](#13-tuning-guide--making-the-robot-better)
14. [Troubleshooting](#14-troubleshooting)

---

## 1. Parts List — Everything You Need

Think of this like a shopping list. You need every single item on this list
for the robot to work correctly.

| # | Part Name | How Many | What It Does |
|---|-----------|----------|--------------|
| 1 | Raspberry Pi Zero 2 W | 1 | The brain — a tiny computer that runs all the code |
| 2 | GA25-370 DC Gear Motor with Encoder (12V, 130 RPM) | 2 | The muscles — spins the wheels to move the robot |
| 3 | Robot Wheel 65mm × 26mm + 4mm Metal Coupler | 2 | The feet — attach to the motors and touch the ground |
| 4 | Metal Caster Wheel | 1 | A small free-spinning support wheel at the front |
| 5 | L298N Dual H-Bridge Motor Driver Module | 1 | Tells the motors how fast and which direction to spin |
| 6 | 18650 3.7V 1200mAh Button-Top Li-ion Battery | 3 | The energy — powers everything |
| 7 | 3-Slot 18650 Series Battery Holder | 1 | Holds the 3 batteries together in a line |
| 8 | MT3608 DC-DC Boost Converter (HW-045) | 1 | Steps up 3.7V from one battery to a safe 5V for the Pi |
| 9 | HC-SR04 Ultrasonic Sensor | 1 | The eyes — sends sound waves to detect the opponent |
| 10 | Downward IR Line Sensor (3-pin digital) | 2 | Looks at the floor to detect the white boundary line |
| 11 | Black Rocker Switch (2-pin, ON/OFF) | 1 | The on/off switch — arms and disarms the robot |
| 12 | LED (any colour, 3mm or 5mm) | 1 | A small light that shows if the robot is on and ready |
| 13 | 330 Ω resistor | 1 | Protects the LED from burning out |
| 14 | 1 kΩ resistor | 1 | Part of the voltage divider for the ultrasonic sensor |
| 15 | 2 kΩ resistor (or two 1kΩ in series) | 1 | Part of the voltage divider for the ultrasonic sensor |

---

## 2. How the Power Works

### The Big Picture

Your 3 batteries are connected in a line (called "in series"). This means their
voltages add up:

```
3.7V + 3.7V + 3.7V = 11.1V total
```

But different parts of the robot need different voltages:

- The **motors** are happy with the full **11.1V** — they are strong and need it
- The **Raspberry Pi** needs exactly **5V** — give it more and it dies instantly
- The **sensors** also need **5V**

So we need to split the power cleverly.

### The Problem with the MT3608

The MT3608 boost converter can only go **UP** in voltage, never down.
So you **cannot** feed it 11.1V and get 5V out — it would just make it higher.

### The Solution — Single Cell Tap

Instead of using all 3 batteries for the MT3608, we tap off just **one battery**
(3.7V) from inside the pack and feed that into the MT3608. It then boosts
3.7V up to 5V. Perfect.

```
Your 3-slot battery holder has 4 metal contact points:

[−] Battery 1 [+]───[−] Battery 2 [+]───[−] Battery 3 [+]
 │                        │                              │
 │                        │                              │
GND (0V)             ~3.7V tap                     ~11.1V (+)
(common ground)    ↓                              ↓
                MT3608 VIN+                   L298N 12V pin
                (boosts to 5V)               (powers motors)
```

### What Connects to What

```
Battery pack full (+) 11.1V ──────────────────► L298N 12V terminal
                                                  └──► Both motors

Battery 1 (+) tap ~3.7V ──► MT3608 VIN+
Battery pack (−) GND    ──► MT3608 VIN−
                                │
                         MT3608 VOUT+ = 5.0V (after adjustment)
                                │
               ┌────────────────┼──────────────────────┐
               ▼                ▼                       ▼
        Pi Zero 2 W       HC-SR04 VCC          Both IR sensors VCC
        Pin 2 (5V)                             Both encoder green wires

Common GND rail (everything shares this):
  Battery (−) ── L298N GND ── MT3608 VIN− ── MT3608 VOUT− ── Pi GND ── All sensor GNDs
```

### ⚠️ CRITICAL — Set the MT3608 Output BEFORE Connecting the Pi

The MT3608 has a tiny screw (trimmer potentiometer) on top that controls
the output voltage. You MUST set it to exactly 5.00V before connecting
anything to it. If you give the Pi more than 5.1V it will be permanently
destroyed — there is no fixing it.

**How to set it safely:**
1. Connect ONLY the single battery tap to MT3608 VIN+ and VIN−
2. Connect a multimeter to VOUT+ and VOUT−
3. Turn the tiny screw **clockwise very slowly**
4. Stop exactly when the multimeter reads **5.00V**
5. Double-check it. Read it again. Make sure.
6. Only NOW connect VOUT+ to Pi Pin 2 and all sensor VCC wires

> ⚠️ The single battery powering the Pi and sensors will drain faster than
> the other two because it works harder. Check its charge before every match.
> Optionally add a 1N5819 Schottky diode in series with MT3608 VIN+ to
> protect against reverse current.

> ⚠️ Some MT3608 HW-045 boards can only deliver ~0.5A continuously even
> though they say "2A" on the label. If the Pi resets when the motors start,
> solder a 1000µF / 10V electrolytic capacitor across VOUT+ and VOUT− to
> absorb the sudden power spike when motors turn on.

---

## 3. Wiring Guide — Step by Step

### Understanding BCM Pin Numbers

The Raspberry Pi has two ways to refer to its pins:
- **Physical pin number** — the actual position on the board (1 to 40)
- **BCM number** — the internal name the software uses

The code always uses BCM numbers. The table below shows both so you can
find the right pin on the board.

### Complete Pi Zero 2 W Pinout

```
Physical Pin │ BCM  │ Wire Goes To
─────────────┼──────┼──────────────────────────────────────────────────────
Pin  2 (5V)  │  —   │ MT3608 VOUT+ (5.0V regulated power IN to Pi)
Pin  6 (GND) │  —   │ Common GND rail
Pin 11       │ BCM17│ L298N IN1   — tells left motor: go forward or backward (A)
Pin 12       │ BCM18│ L298N ENA   — controls left motor SPEED via PWM
Pin 13       │ BCM27│ L298N IN2   — tells left motor: go forward or backward (B)
Pin 15       │ BCM22│ L298N IN3   — tells right motor: go forward or backward (A)
Pin 16       │ BCM23│ L298N IN4   — tells right motor: go forward or backward (B)
Pin 18       │ BCM24│ HC-SR04 TRIG — Pi sends a pulse to start a distance reading
Pin 22       │ BCM25│ HC-SR04 ECHO — Pi receives the echo (via resistor divider!)
Pin 29       │ BCM 5│ Left motor encoder — yellow wire  (Channel A)
Pin 31       │ BCM 6│ Left motor encoder — white wire   (Channel B)
Pin 32       │ BCM12│ Front IR sensor OUT — detects white line at front
Pin 33       │ BCM13│ L298N ENB   — controls right motor SPEED via PWM
Pin 35       │ BCM19│ Right motor encoder — yellow wire (Channel A)
Pin 36       │ BCM16│ Back IR sensor OUT  — detects white line at back
Pin 37       │ BCM26│ Right motor encoder — white wire  (Channel B)
Pin 38       │ BCM20│ LED anode (+ leg) via 330Ω resistor → GND
Pin 40       │ BCM21│ Rocker switch (one pin) — other pin goes to GND
```

### GA25-370 Motor — All 6 Wires Explained

Each motor has 6 wires coming out of it. 2 wires power the motor itself,
and 4 wires go to the small encoder board on the back of the motor.

```
Wire Colour │ What It Is          │ Where It Goes
────────────┼─────────────────────┼────────────────────────────────────────
Red         │ Motor power +       │ L298N OUT1 (left motor) / OUT3 (right motor)
Black       │ Motor power −       │ L298N OUT2 (left motor) / OUT4 (right motor)
Green       │ Encoder power 5V    │ MT3608 5V rail (same rail as Pi power)
Dark Blue   │ Encoder GND         │ Common GND rail
Yellow      │ Encoder Channel A   │ Pi BCM5 (left motor) / BCM19 (right motor)
White       │ Encoder Channel B   │ Pi BCM6 (left motor) / BCM26 (right motor)
```

> The encoder is like a speedometer — it counts how many times the wheel
> has turned so the robot knows if it is going straight or drifting sideways.

### L298N Motor Driver — All Terminals

```
L298N Terminal │ Where It Goes                    │ Why
───────────────┼──────────────────────────────────┼──────────────────────────
12V            │ Battery pack (+) ~11.1V           │ Power for the motors
GND            │ Common GND rail                   │ Ground reference
5V output      │ DO NOT USE — leave unconnected    │ Too noisy for the Pi
IN1            │ Pi BCM17                          │ Left motor direction bit A
IN2            │ Pi BCM27                          │ Left motor direction bit B
IN3            │ Pi BCM22                          │ Right motor direction bit A
IN4            │ Pi BCM23                          │ Right motor direction bit B
ENA            │ Pi BCM18  (REMOVE the ENA jumper) │ Left motor speed (PWM)
ENB            │ Pi BCM13  (REMOVE the ENB jumper) │ Right motor speed (PWM)
OUT1 / OUT2    │ Left motor red / black wires      │ Power to left motor
OUT3 / OUT4    │ Right motor red / black wires     │ Power to right motor
```

> ⚠️ IMPORTANT: The L298N board comes with two small yellow jumpers on ENA
> and ENB. You MUST remove both of them. If you leave them in, the motors
> will always run at 100% speed and the Pi cannot control them at all.

---

## 4. The Rocker Switch & LED

### How the Rocker Switch Works

The rocker switch is a simple 2-pin on/off switch. One pin connects to
Pi BCM21 (Physical Pin 40) and the other pin connects to GND.

```
Pi BCM21 (Pin 40) ──── [ROCKER SWITCH] ──── GND
```

The Pi has an internal "pull-up resistor" on BCM21. This means:
- Switch **ON**  (closed) → pin is pulled to GND → Pi reads **LOW**  → robot runs
- Switch **OFF** (open)   → pin floats HIGH      → Pi reads **HIGH** → robot stops

### What the LED Tells You

The LED is connected between BCM20 (Physical Pin 38) and GND, with a
330Ω resistor in series to protect it from burning out.

```
Pi BCM20 (Pin 38) ──── 330Ω resistor ──── LED (+) ──── LED (−) ──── GND
```

The LED has 3 different behaviours so you always know what the robot is doing:

| LED State | What It Means |
|-----------|---------------|
| OFF (not lit) | Switch is OFF — robot is disarmed and safe to handle |
| SLOW BLINK | Waiting for switch to be turned ON |
| FAST BLINK | Switch is ON — 5-second start delay counting down |
| SOLID ON | Robot is fully armed and actively competing |

> At a competition, wait until the LED goes SOLID ON before placing the
> robot on the ring. That means the 5-second delay has finished and the
> robot is about to move.

### Step-by-Step Switch Procedure at a Competition

1. Place the robot on the ring but keep your hand on it
2. Flip the rocker switch to ON
3. LED will blink fast — this is the mandatory 5-second countdown
4. When LED goes solid ON — let go of the robot immediately
5. The robot is now fully autonomous and competing
6. To stop the robot at any time — flip the switch to OFF
7. Motors stop instantly. Pi stays on. You can flip it back ON to restart.

---

## 5. Protecting the Pi from the Ultrasonic Sensor

### The Problem

The HC-SR04 ultrasonic sensor sends back a signal on its ECHO pin at **5V**.
The Raspberry Pi GPIO pins can only handle **3.3V maximum**.
If 5V goes directly into the Pi GPIO, it will permanently damage the pin
(and possibly the whole Pi).

### The Solution — Resistor Voltage Divider

We use two resistors to reduce the 5V signal down to 3.33V before it
reaches the Pi. This is called a voltage divider.

```
HC-SR04 ECHO pin (5V)
        │
       [1kΩ resistor]
        │
        ├──────────────────► Pi BCM25 (Pin 22) — sees ~3.33V ✓
        │
       [2kΩ resistor]
        │
       GND

Maths: 5V × (2kΩ ÷ (1kΩ + 2kΩ)) = 5V × 0.667 = 3.33V ✓
```

The TRIG pin on the HC-SR04 is fine to connect directly — the Pi sends
a 3.3V signal and the HC-SR04 accepts it without any problem.

> Use 1% tolerance resistors for accuracy. For extra safety you can add
> a 3.3V Zener diode in parallel with the 2kΩ resistor — it will clamp
> any voltage spike above 3.3V before it reaches the Pi.

---

## 6. How the Software Works

### The 5 Files and What Each One Does

Think of the code like a team of workers, each with one specific job:

```
main.py ──── The manager. Starts everything, watches the switch,
    │         controls the LED, keeps the loop running.
    │
    ├── config.py ──── The settings book. Every pin number, speed,
    │                  and timing value lives here. Change behaviour
    │                  by editing ONLY this file.
    │
    ├── motors.py ──── The driver. Tells the L298N how fast and which
    │                  direction to spin each motor. Also reads the
    │                  encoders to know if the robot is going straight.
    │
    ├── sensors.py ─── The senses. Reads the ultrasonic distance sensor
    │                  and both IR floor sensors. Also tracks where the
    │                  opponent probably is even when not directly visible.
    │
    └── strategy.py ── The brain. Decides what the robot should do next
                       based on what the sensors are reporting.
```

### How Fast Does It Think?

The main loop runs **50 times per second** (set by `LOOP_HZ = 50` in config.py).
Every 20 milliseconds the robot reads all sensors, makes a decision, and
updates the motors. This is fast enough to react to anything in a match.

---

## 7. How the Brain (FSM) Makes Decisions

### What is an FSM?

FSM stands for Finite State Machine. It just means the robot is always in
exactly one "state" (mode) at a time, and it switches between states based
on what the sensors detect.

Think of it like a person's mood — you can only be in one mood at a time,
but something can happen that changes your mood instantly.

### The 7 States — From Most Important to Least Important

The robot checks these in order every single tick. The first one that applies wins.

```
┌─────────────────────────────────────────────────────────────────┐
│  1. EDGE_AVOID  ← ⚠️ HIGHEST PRIORITY — always wins            │
│     White line detected under front or back IR sensor.          │
│     Robot reverses away then spins to face the ring centre.     │
│     Nothing overrides this — not even an opponent right in      │
│     front of it. Staying in the ring comes first.               │
├─────────────────────────────────────────────────────────────────┤
│  2. RAM                                                         │
│     Opponent is extremely close (< 25cm directly ahead).        │
│     Robot fires a short full-speed burst to push it out.        │
├─────────────────────────────────────────────────────────────────┤
│  3. ATTACK                                                      │
│     Opponent detected within 80cm.                              │
│     Robot charges at full speed. Uses encoder feedback to       │
│     keep driving in a straight line (corrects drift).           │
├─────────────────────────────────────────────────────────────────┤
│  4. STALK                                                       │
│     Opponent visible but far away (80–150cm).                   │
│     Robot arcs toward it carefully, adjusting direction         │
│     based on the estimated bearing to the opponent.             │
├─────────────────────────────────────────────────────────────────┤
│  5. ORBIT                                                       │
│     Only used in FLANKER tactic mode.                           │
│     Robot arcs wide around the opponent to approach             │
│     from the side instead of head-on.                           │
├─────────────────────────────────────────────────────────────────┤
│  6. SEARCH                                                      │
│     No opponent detected anywhere.                              │
│     Robot spins in place to scan the full 360° ring.            │
│     Reverses spin direction after each full rotation to         │
│     avoid tangling wires.                                       │
├─────────────────────────────────────────────────────────────────┤
│  7. START  ← LOWEST PRIORITY — only at the beginning           │
│     Mandatory 5-second pause required by competition rules.     │
│     Robot sits completely still. LED blinks.                    │
└─────────────────────────────────────────────────────────────────┘
```

### The Adaptive Tactic System

The robot does not just do the same thing every match. It watches what is
happening and changes its fighting style automatically:

| Style | What It Does | When It Switches To This |
|-------|-------------|--------------------------|
| RUSHER | Charges straight at the opponent immediately at full speed | Default style at match start. Also if direct charges have worked 2+ times |
| STALKER | Approaches carefully using curved paths | If being pushed to the edge |
| FLANKER | Circles wide around the opponent to attack from the side | If pushed to the edge 3+ times — opponent is stronger head-on |
| RUSHER (forced) | Ignores everything and charges regardless | Last 30 seconds of the match — time is running out |

### The Opponent Tracker

Even when the ultrasonic sensor loses sight of the opponent (because the
robot is spinning), the code remembers roughly where it last was and
rotates that estimate as the robot turns. This means when the robot
finishes spinning it already knows which direction to look first —
instead of starting the search from scratch every time.

The tracker uses a **confidence score** from 0.0 to 1.0:
- 1.0 = opponent definitely detected right now
- 0.5 = probably still there, last seen recently
- 0.0 = completely lost, switch to SEARCH

### Edge Avoidance — Which Way to Spin?

When the robot hits the white boundary line, it needs to spin back toward
the centre of the ring. The direction it spins depends on which sensor fired:

- **Front IR triggered** → robot reverses backward, then spins right
- **Back IR triggered**  → robot drives forward, then spins right
- **Both triggered**     → robot reverses, then spins right (safest default)

---

## 8. Setting Up the Raspberry Pi

### Step 1 — Flash the Operating System

1. Download **Raspberry Pi Imager** from https://www.raspberrypi.com/software/
2. Insert a microSD card (8GB minimum) into your computer
3. Open Raspberry Pi Imager
4. Click "Choose OS" → "Raspberry Pi OS (other)" → **"Raspberry Pi OS Lite (64-bit)"**
   - We use Lite because it has no desktop — the robot does not need a screen
5. Click the gear icon ⚙️ (Advanced Options) and fill in:
   - Hostname: `sumobot` (or anything you like)
   - Enable SSH: tick this box
   - Username: `pi`  Password: choose something you will remember
   - Wi-Fi SSID and password: your home Wi-Fi so you can connect to it
6. Click Save, then Write
7. Wait for it to finish, then put the microSD card into the Pi

### Step 2 — First Boot and Connect

Power on the Pi and wait 60 seconds for it to boot. Then from your computer:

```bash
ssh pi@sumobot.local
```

If that does not work, find the Pi's IP address from your router and use:

```bash
ssh pi@192.168.x.x
```

### Step 3 — Update Everything

```bash
sudo apt update && sudo apt full-upgrade -y
sudo apt install -y python3-pip python3-venv git
```

This downloads all the latest software. It may take a few minutes.

### Step 4 — Enable GPIO Interfaces

```bash
sudo raspi-config
```

Navigate through the menus:
- **Interface Options → I2C → Enable**
- **Interface Options → SPI → Enable**
- **Performance Options → GPU Memory → type 16 → OK**
  (The robot has no screen so the GPU needs almost no memory)
- **Finish → Yes to reboot**

Wait for the Pi to reboot, then SSH back in.

---

## 9. Installing the Code

```bash
# Download the project
git clone https://github.com/<your-username>/sumo-robot.git
cd sumo-robot

# Create an isolated Python environment (keeps things clean)
python3 -m venv .venv
source .venv/bin/activate

# Install the only dependency needed
pip install RPi.GPIO
```

### Run the Pi Setup Script (do this once)

This script enables the hardware watchdog and prevents the Pi from sleeping:

```bash
bash setup_pi.sh
sudo reboot
```

After the reboot, SSH back in and activate the environment again:

```bash
cd sumo-robot
source .venv/bin/activate
```

---

## 10. Making the Robot Start Automatically on Power-Up

For competition use, you want the robot to start its code the moment it
is powered on — no laptop needed, no SSH, nothing. This is done using
**systemd**, which is the Pi's built-in service manager.

### Create the Service File

```bash
sudo nano /etc/systemd/system/sumo.service
```

Copy and paste exactly this (adjust the path if your folder is different):

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

Press **Ctrl+X**, then **Y**, then **Enter** to save.

### Enable and Start It

```bash
sudo systemctl daemon-reload
sudo systemctl enable sumo.service
sudo systemctl start  sumo.service
```

### Check It Is Running

```bash
sudo journalctl -u sumo.service -f
```

You should see log messages appearing. Press Ctrl+C to stop watching.

### Useful Commands

```bash
sudo systemctl stop    sumo.service   # Stop the robot code
sudo systemctl start   sumo.service   # Start it again
sudo systemctl restart sumo.service   # Restart it
sudo systemctl disable sumo.service   # Stop it from starting on boot
sudo systemctl enable  sumo.service   # Re-enable starting on boot
```

---

## 11. Keeping the Pi Awake Forever

By default the Raspberry Pi can slow down its CPU or enter idle states
to save power. For a competition robot this is dangerous — we need it
running at full speed at all times.

We use two layers of protection:

### Layer 1 — No Sleep (setup_pi.sh handles this)

The setup script:
- Sets the CPU to **performance mode** — never slows down
- Disables **console blanking** — screen never goes dark
- Disables **Wi-Fi power saving** — connection stays active
- Disables **swap** — no slow disk access during a match

### Layer 2 — Hardware Watchdog

The Pi Zero 2 W has a built-in hardware watchdog timer. Think of it like
an alarm clock that goes off every 60 seconds. The code must "pet" the
watchdog (reset the timer) before it goes off. If the code crashes or
freezes and stops petting it, the watchdog fires and **automatically
reboots the Pi**.

The setup script enables this. The code in main.py pets it every 5 seconds
from a background thread.

```
Code running normally:
  Pet watchdog → 5s → Pet watchdog → 5s → Pet watchdog → ...  (watchdog never fires)

Code crashes or freezes:
  Pet watchdog → 5s → [CRASH — no more pets] → 60s → WATCHDOG REBOOTS PI
```

After a reboot, systemd automatically restarts the sumo code (because of
`Restart=on-failure` in the service file). The robot recovers by itself.

---

## 12. Testing Everything Before a Match

Always test each part separately before a competition. Run these tests
on the Pi via SSH.

```bash
cd sumo-robot
source .venv/bin/activate
```

### Test 1 — Motors Forward

```bash
python3 - <<'EOF'
import time
from motors import MotorController
m = MotorController()
print("Driving forward for 2 seconds...")
m.forward(50)
time.sleep(2)
m.stop()
m.cleanup()
print("Done.")
EOF
```

Both wheels should spin forward. If one goes backward, swap that motor's
red and black wires on the L298N output terminals.

### Test 2 — Encoders

```bash
python3 - <<'EOF'
import time
from motors import MotorController
m = MotorController()
m.reset_encoders()
m.forward(50)
time.sleep(1)
m.stop()
print(f"Left ticks:  {m.left_enc.get_ticks()}")
print(f"Right ticks: {m.right_enc.get_ticks()}")
m.cleanup()
EOF
```

Both numbers should be positive and roughly equal (within 10%). If one
is negative, swap that motor's red and black wires on the L298N.

### Test 3 — Ultrasonic Sensor

```bash
python3 - <<'EOF'
import time
from sensors import UltrasonicSensor
us = UltrasonicSensor()
for i in range(10):
    print(f"Distance: {us.distance_cm():.1f} cm")
    time.sleep(0.2)
EOF
```

Hold your hand in front of the sensor at different distances. The numbers
should change smoothly. 999.0 means nothing was detected (out of range).

### Test 4 — IR Edge Sensors

```bash
python3 - <<'EOF'
import time
from sensors import IRSensors
ir = IRSensors()
print("Move the robot over white and black surfaces. Press Ctrl+C to stop.")
while True:
    print(f"Front: {'WHITE LINE' if ir.edge_front() else 'black - ok'} | "
          f"Back: {'WHITE LINE' if ir.edge_back() else 'black - ok'}")
    time.sleep(0.1)
EOF
```

Over the black ring surface: both should say "black - ok"
Over the white boundary line: the relevant sensor should say "WHITE LINE"

Each IR sensor has a small potentiometer (tiny screw) on the board.
Adjust it until the LED on the sensor board:
- Turns **ON** over the black dohyo surface
- Turns **OFF** over the white boundary ring

### Test 5 — Rocker Switch and LED

```bash
python3 - <<'EOF'
import RPi.GPIO as gpio
import time, config
gpio.setmode(gpio.BCM)
gpio.setup(config.SWITCH_PIN, gpio.IN, pull_up_down=gpio.PUD_UP)
gpio.setup(config.LED_PIN, gpio.OUT)
print("Flip the switch ON and OFF. Press Ctrl+C to stop.")
while True:
    if gpio.input(config.SWITCH_PIN) == gpio.LOW:
        gpio.output(config.LED_PIN, gpio.HIGH)
        print("Switch: ON  | LED: ON")
    else:
        gpio.output(config.LED_PIN, gpio.LOW)
        print("Switch: OFF | LED: OFF")
    time.sleep(0.2)
EOF
```

---

## 13. Tuning Guide — Making the Robot Better

All tuning is done by editing numbers in **config.py only**.
Never change numbers in any other file.

### Speed Settings

| Setting | Default | What Changing It Does |
|---------|---------|----------------------|
| `SPEED_FULL` | 100 | Attack charge speed. 100 = maximum. Lower if motors overheat |
| `SPEED_SEARCH` | 55 | How fast it spins while looking for the opponent |
| `SPEED_STALK` | 70 | How fast it approaches a far opponent |
| `SPEED_REVERSE` | 85 | How fast it reverses away from the edge |
| `SPEED_TURN` | 80 | How fast it spins during edge avoidance |
| `SPEED_CORRECT` | 40 | How aggressively it corrects straight-line drift |

### Distance Settings

| Setting | Default | What Changing It Does |
|---------|---------|----------------------|
| `ATTACK_DISTANCE_CM` | 80 | How close opponent must be before full charge. Increase to charge from further away |
| `STALK_DISTANCE_CM` | 150 | How far away opponent can be and still be tracked |
| `CLOSE_DISTANCE_CM` | 25 | Distance that triggers the RAM burst push |

### Timing Settings

| Setting | Default | What Changing It Does |
|---------|---------|----------------------|
| `REVERSE_DURATION` | 0.30s | How long to reverse when edge detected. Increase if robot falls off |
| `SPIN_DURATION` | 0.45s | How long to spin after reversing. Increase if robot still faces edge |
| `SCAN_STEP_TIME` | 0.55s | How long each scan step lasts. Increase for slower but more thorough scan |
| `LOST_TIMEOUT` | 1.2s | How long to remember opponent after losing sight. Increase to track longer |
| `RAM_BURST_DURATION` | 0.18s | Length of the close-range burst push |
| `LOOP_HZ` | 50 | How many times per second the brain runs. Higher = faster reactions |

### Motor Direction Is Wrong?

If a motor spins the wrong direction (robot goes backward when it should
go forward, or turns the wrong way):

**DO NOT change the code.** Just physically swap the red and black wires
on that motor's L298N output terminals (OUT1/OUT2 or OUT3/OUT4).

---

## 14. Troubleshooting

### Robot does not move at all
- Check the rocker switch is ON (LED should be blinking or solid)
- Check the 5-second start delay has finished (LED should be solid ON)
- Check all power connections
- Run the motor test in Section 12

### Robot moves but goes in circles
- One motor is spinning faster than the other, or one is going backward
- Run the encoder test — both tick counts should be positive and similar
- If one is negative: swap that motor's red/black wires on L298N

### Robot falls off the ring
- IR sensors need calibration — adjust the potentiometer on each sensor
- Increase `REVERSE_DURATION` in config.py (reverses further from edge)
- Increase `SPIN_DURATION` in config.py (spins more before resuming)

### Robot does not detect the opponent
- Check HC-SR04 wiring — TRIG to BCM24, ECHO to BCM25 via resistor divider
- Run the ultrasonic test in Section 12
- Make sure the 1kΩ and 2kΩ resistors are correctly placed on the ECHO line

### Pi resets when motors start
- The MT3608 cannot supply enough current during motor startup
- Solder a 1000µF / 10V electrolytic capacitor across MT3608 VOUT+ and VOUT−
- Make sure the capacitor + leg goes to VOUT+ and − leg goes to VOUT−

### SSH connection refused
- Wait longer after power-on (Pi takes ~60 seconds to boot)
- Check Wi-Fi credentials were set correctly during imaging
- Try `ssh pi@<ip-address>` instead of `ssh pi@sumobot.local`

### Code crashes during match
- Check `sudo journalctl -u sumo.service -f` for error messages
- The watchdog will auto-reboot the Pi if it freezes completely
- systemd will restart the code automatically after a crash

---

## License

MIT — free to use, modify, and distribute for educational and competition purposes.
