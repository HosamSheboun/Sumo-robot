# Arduino Uno Sumo Robot — Spin Search + Ultrasonic Attack

This folder contains the Arduino Uno code for the sumo robot.

**Behaviour:**
- Flip the rocker switch ON
- Robot waits 5 seconds (mandatory competition start delay)
- Robot spins in place to scan for the opponent
- When the ultrasonic sensor detects something within 80cm — charge at full speed
- If the opponent disappears — go back to spinning and searching
- Flip the switch OFF at any time to stop instantly

---

## Table of Contents

1. [Parts List](#1-parts-list)
2. [How the Power Works](#2-how-the-power-works)
3. [Complete Wiring Guide](#3-complete-wiring-guide)
4. [The Rocker Switch and LED](#4-the-rocker-switch-and-led)
5. [How the Strategy Works](#5-how-the-strategy-works)
6. [What the Code Files Do](#6-what-the-code-files-do)
7. [Installing Arduino IDE and Uploading](#7-installing-arduino-ide-and-uploading)
8. [Testing Before a Match](#8-testing-before-a-match)
9. [Tuning](#9-tuning)
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

> No MT3608 converter needed. No IR sensors needed. No resistor divider needed for the HC-SR04
> because the Arduino Uno runs at 5V logic — same as the sensor.

---

## 2. How the Power Works

```
3× 18650 in series (~11.1V)
         │
         ├──────────────────────────► L298N 12V terminal
         │                            └──► Both GA25-370 motors
         │
         └──────────────────────────► Arduino Uno VIN pin
                                       └──► Uno onboard regulator → 5V
                                            ├──► Uno logic
                                            ├──► HC-SR04 VCC
                                            └──► Encoder green wires (VCC)

Common GND rail:
  Battery (−) ── L298N GND ── Uno GND ── HC-SR04 GND ── Both encoder dark blue wires
```

### Why the Uno VIN Pin Works Here

The Arduino Uno has a built-in voltage regulator on its VIN pin that
accepts 7V to 12V and outputs a clean 5V. Your 11.1V battery pack is
within this range so no extra converter is needed.

> ⚠️ Connect the battery to the **VIN pin** only — never to the 5V pin.
> The 5V pin bypasses the regulator and will instantly destroy the Uno.

> ⚠️ The Uno's onboard regulator may get warm under load. This is normal.
> If it gets very hot, use a separate LM2596 buck converter for 5V instead.

---

## 3. Complete Wiring Guide

### Arduino Uno Pin Assignments

```
Uno Pin  │ Connected To
─────────┼──────────────────────────────────────────────────────
VIN      │ Battery pack (+) ~11.1V
GND      │ Common GND rail
5V       │ HC-SR04 VCC + both encoder green wires
D3  (~)  │ L298N ENB  (Right motor PWM speed)
D4       │ L298N IN3  (Right motor direction A)
D5       │ L298N IN4  (Right motor direction B)
D7       │ L298N IN1  (Left motor direction A)
D8       │ L298N IN2  (Left motor direction B)
D9  (~)  │ L298N ENA  (Left motor PWM speed)
D10      │ Rocker switch (other pin → GND)
D11      │ HC-SR04 TRIG
D12      │ HC-SR04 ECHO
D13      │ LED anode via 330Ω resistor → GND (also lights the Uno built-in LED)
```

> (~) means the pin is PWM-capable. ENA and ENB must use PWM pins or speed control will not work.

### HC-SR04 Ultrasonic Sensor

Mount the sensor at the front of the robot, facing forward.

```
HC-SR04 Pin │ Connected To
────────────┼──────────────────────────────────────────────────
VCC         │ Uno 5V
GND         │ Common GND rail
TRIG        │ Uno D11
ECHO        │ Uno D12
```

> No resistor divider needed. The Arduino Uno runs at 5V logic —
> same as the HC-SR04 — so you connect ECHO directly.

### GA25-370 Motor — All 6 Wires

Each motor has 6 wires. Here is exactly where each one goes:

```
Wire Colour │ What It Is        │ Where It Goes
────────────┼───────────────────┼──────────────────────────────────────
Red         │ Motor power +     │ L298N OUT1 (left) / OUT3 (right)
Black       │ Motor power −     │ L298N OUT2 (left) / OUT4 (right)
Green       │ Encoder VCC 5V    │ Uno 5V pin
Dark Blue   │ Encoder GND       │ Common GND rail
Yellow      │ Encoder Channel A │ Leave unconnected (not used)
White       │ Encoder Channel B │ Leave unconnected (not used)
```

> The green and dark blue encoder wires MUST still be connected to 5V
> and GND even though the encoder signals are not used. If you leave
> them floating the encoder board may draw unpredictable current and
> cause noise on the power rail.

### L298N Motor Driver — All Terminals

```
L298N Terminal │ Connected To                     │ Why
───────────────┼──────────────────────────────────┼──────────────────────
12V            │ Battery pack (+) ~11.1V           │ Power for the motors
GND            │ Common GND rail                   │ Ground reference
5V output      │ NOT USED — leave unconnected      │ Too noisy for logic
IN1            │ Uno D7                            │ Left motor direction A
IN2            │ Uno D8                            │ Left motor direction B
IN3            │ Uno D4                            │ Right motor direction A
IN4            │ Uno D5                            │ Right motor direction B
ENA            │ Uno D9  (REMOVE the ENA jumper)   │ Left motor PWM speed
ENB            │ Uno D3  (REMOVE the ENB jumper)   │ Right motor PWM speed
OUT1 / OUT2    │ Left  motor red / black wires     │ Power to left motor
OUT3 / OUT4    │ Right motor red / black wires     │ Power to right motor
```

> ⚠️ REMOVE the two small yellow jumpers on ENA and ENB from the L298N
> board before wiring. If you leave them in, the motors always run at
> full speed and the Uno cannot control them at all.

### Rocker Switch

```
Uno D10 ──── [ROCKER SWITCH] ──── GND
```

One pin of the switch goes to D10. The other pin goes to GND.
No resistors needed — the Uno has an internal pull-up.

### LED

```
Uno D13 ──── 330Ω resistor ──── LED (+) long leg ──── LED (−) short leg ──── GND
```

The 330Ω resistor MUST be in series with the LED or it will burn out instantly.

> D13 also drives the Uno's built-in LED on the board, so you can test the LED behaviour
> without wiring an external LED at all.

---

## 4. The Rocker Switch and LED

| LED State | What It Means |
|-----------|---------------|
| OFF | Switch is OFF — robot is safe to handle |
| SLOW BLINK (200ms on / 800ms off) | Waiting for switch to be turned ON |
| FAST BLINK (100ms on / 100ms off) | Switch is ON — 5-second countdown |
| SOLID ON | Robot is armed and actively searching / attacking |

### Step-by-Step at a Competition

1. Place the robot on the ring — keep your hand on it
2. Flip the rocker switch to ON
3. LED blinks fast for 5 seconds — mandatory countdown
4. When LED goes solid ON — release the robot immediately
5. Robot begins spinning to search for the opponent
6. When it finds the opponent it charges at full speed
7. Flip switch OFF at any time to stop instantly
8. Flip switch back ON to restart the 5-second delay and go again

---

## 5. How the Strategy Works

The robot has two modes it switches between automatically:

```
┌─────────────────────────────────────────────────────────┐
│  SEARCH                                                 │
│  Ultrasonic sees nothing within 80cm.                   │
│  Robot spins right in place for 500ms, then checks      │
│  the sensor again. Keeps spinning until it finds        │
│  something.                                             │
├─────────────────────────────────────────────────────────┤
│  ATTACK                                                 │
│  Ultrasonic detects something within 80cm.              │
│  Robot charges forward at full speed.                   │
│  If the opponent moves out of range, robot goes back    │
│  to SEARCH immediately on the next loop.                │
└─────────────────────────────────────────────────────────┘
```

The sensor is checked every loop. When charging, if the opponent
disappears (pushed out, moved sideways, or sensor loses it), the robot
automatically switches back to spinning and searching.

---

## 6. What the Code Files Do

```
arduino_autonomous.ino  ← Main sketch. Switch, LED, 5-second delay, search/attack loop.
config.h                ← All pin numbers and constants. Edit only this file to tune.
motors.h                ← Motor function declarations.
motors.cpp              ← L298N motor control — forward, spin left, spin right, stop.
```

---

## 7. Installing Arduino IDE and Uploading

### Step 1 — Install Arduino IDE

1. Go to https://www.arduino.cc/en/software
2. Download **Arduino IDE 2** for your operating system
3. Install it

### Step 2 — Connect the Uno to Your Laptop

Plug the Arduino Uno into your laptop using a **USB Type-B cable** (the square-ish connector).

> Some cheap USB cables are charge-only and carry no data.
> If the Uno does not appear in the port list, try a different cable.

### Step 3 — Configure Arduino IDE

1. Open Arduino IDE
2. Go to **Tools → Board → Arduino AVR Boards → Arduino Uno**
3. Go to **Tools → Port** → select the port that appeared (e.g. COM3 on Windows)

### Step 4 — Open the Project

1. Go to **File → Open**
2. Navigate to the `arduino_autonomous` folder
3. Open `arduino_autonomous.ino`
4. Arduino IDE will automatically load config.h, motors.h, and motors.cpp

### Step 5 — Upload

1. Click the **Upload** button (→ arrow) or press **Ctrl+U**
2. Wait for "Done uploading" at the bottom of the screen
3. The Uno restarts automatically and runs the code immediately

> If upload fails, try a different USB cable or check the correct COM port is selected.

---

## 8. Testing Before a Match

### Test 1 — Switch and LED

1. Power on the robot
2. LED should blink slowly — waiting for switch
3. Flip switch ON — LED blinks fast for 5 seconds
4. LED goes solid ON — robot starts spinning
5. Flip switch OFF — robot stops, LED goes off

### Test 2 — Ultrasonic Sensor

1. Power on and flip switch ON, wait for LED to go solid
2. Robot should be spinning
3. Hold your hand in front of the sensor within 80cm
4. Robot should stop spinning and drive forward toward your hand
5. Move your hand away — robot should go back to spinning

### Test 3 — Motor Directions

1. Lift the robot off the ground so wheels spin freely
2. Flip switch ON and wait for LED to go solid
3. Both wheels should spin in opposite directions during the spin search
   (one forward, one backward — this is correct for spinning in place)
4. Hold your hand in front — both wheels should spin forward together
5. If a wheel spins the wrong way — swap that motor's red and black wires
   on the L298N output terminals (OUT1/OUT2 or OUT3/OUT4)

---

## 9. Tuning

All tuning is done by editing **config.h only**. Never change numbers in any other file.

| Setting | Default | What Changing It Does |
|---------|---------|----------------------|
| `ATTACK_DIST_CM` | 80 | How close the opponent must be before charging. Increase to charge from further away |
| `SPEED_FULL` | 255 | Attack charge speed. 255 = maximum |
| `SPEED_SPIN` | 180 | How fast the robot spins while searching. Lower if it spins past the opponent too fast |
| `SPIN_STEP_MS` | 500 | How long it spins before checking the sensor again. Lower for more frequent checks |
| `START_DELAY_MS` | 5000 | Mandatory 5-second pre-match pause. Do not change for competition |

---

## 10. Troubleshooting

### Robot does not move at all
- Check rocker switch is ON (LED should be blinking or solid)
- Check battery voltage — 3S pack should read ~11.1V fully charged
- Check L298N ENA and ENB jumpers are removed
- Check all GND wires share a common ground rail

### Robot just drives forward and does not spin
- Check HC-SR04 wiring — TRIG to D11, ECHO to D12
- The sensor may be reading a false close distance on startup
- Increase `ATTACK_DIST_CM` temporarily to confirm the sensor is working

### Robot spins but never attacks
- Hold your hand in front of the sensor — if it still does not attack,
  the sensor is not reading correctly
- Check HC-SR04 VCC is connected to Uno 5V
- Check TRIG is on D11 and ECHO is on D12
- Try increasing `ATTACK_DIST_CM` to 120 to make it trigger more easily

### Robot spins past the opponent without attacking
- Lower `SPIN_STEP_MS` in config.h (e.g. 200) so it checks the sensor more often
- Lower `SPEED_SPIN` so it rotates more slowly and the sensor has more time to catch the opponent

### One motor spins backward
- Swap the red and black wires on that motor's L298N output terminals
- Do not change any code — physical wire swap is the correct fix

### Upload fails
- Try a different USB cable
- Check the correct COM port is selected under Tools → Port
- Make sure **Arduino Uno** is selected under Tools → Board (not Nano)

### Uno gets very hot on VIN
- The onboard regulator is dissipating the voltage difference as heat
- Add a small heatsink to the regulator chip on the Uno
- Or use a separate LM2596 buck converter to supply 7–8V to VIN instead of 11.1V

---

## License
MIT — free to use, modify, and distribute for educational and competition purposes.
