# Arduino Nano Sumo Robot — Forward Drive

This folder contains the Arduino Nano code for the sumo robot.

Behaviour is simple:
- Flip the rocker switch ON
- Robot waits 5 seconds (mandatory competition start delay)
- Robot drives forward at full speed until the switch is turned OFF

No sensors. No FSM. No autonomous logic. Just forward.

---

## Table of Contents

1. [Parts List](#1-parts-list)
2. [How the Power Works](#2-how-the-power-works)
3. [Complete Wiring Guide](#3-complete-wiring-guide)
4. [The Rocker Switch and LED](#4-the-rocker-switch-and-led)
5. [What the Code Files Do](#5-what-the-code-files-do)
6. [Installing Arduino IDE and Uploading](#6-installing-arduino-ide-and-uploading)
7. [Testing Before a Match](#7-testing-before-a-match)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Parts List

| # | Part | Qty | Notes |
|---|------|-----|-------|
| 1 | Arduino Nano (ATmega328P) | 1 | The brain |
| 2 | GA25-370 DC Gear Motor with Encoder (12V, 130RPM) | 2 | Left and right drive |
| 3 | Robot Wheel 65mm × 26mm + 4mm metal coupler | 2 | Drive wheels |
| 4 | Metal caster wheel | 1 | Front support |
| 5 | L298N Dual H-Bridge Motor Driver Module | 1 | Motor driver |
| 6 | 18650 3.7V 1200mAh button-top Li-ion battery | 3 | In series → ~11.1V |
| 7 | 3-slot 18650 series battery holder | 1 | Battery pack |
| 8 | Black rocker switch (2-pin ON/OFF) | 1 | Arms and disarms the robot |
| 9 | LED (any colour, 3mm or 5mm) | 1 | Status indicator |
| 10 | 330Ω resistor | 1 | Protects the LED from burning out |

> No HC-SR04, no IR sensors, no MT3608 converter needed for this build.

---

## 2. How the Power Works

```
3× 18650 in series (~11.1V)
         │
         ├──────────────────────────► L298N 12V terminal
         │                            └──► Both GA25-370 motors
         │
         └──────────────────────────► Arduino Nano VIN pin
                                       └──► Nano onboard regulator → 5V
                                            ├──► Nano logic
                                            └──► Encoder green wires (VCC)

Common GND rail:
  Battery (−) ── L298N GND ── Nano GND ── Both encoder dark blue wires
```

### Why the Nano VIN Pin Works Here

The Arduino Nano has a built-in voltage regulator on its VIN pin that
accepts 7V to 12V and outputs a clean 5V. Your 11.1V battery pack is
within this range so no extra converter is needed.

> ⚠️ Connect the battery to the **VIN pin** only — never to the 5V pin.
> The 5V pin bypasses the regulator and will instantly destroy the Nano.

> ⚠️ The Nano's onboard regulator may get warm under load. This is normal.
> If it gets very hot, use a separate LM2596 buck converter for 5V instead.

---

## 3. Complete Wiring Guide

### Arduino Nano Pin Assignments

```
Nano Pin │ Connected To
─────────┼──────────────────────────────────────────────────────
VIN      │ Battery pack (+) ~11.1V
GND      │ Common GND rail
5V       │ Both encoder green wires (VCC)
D4       │ L298N IN3  (Right motor direction A)
D5       │ L298N IN4  (Right motor direction B)
D6       │ L298N ENB  (Right motor PWM speed)
D7       │ L298N IN1  (Left motor direction A)
D8       │ L298N IN2  (Left motor direction B)
D9       │ L298N ENA  (Left motor PWM speed)
D10      │ Rocker switch (other pin → GND)
D11      │ LED anode via 330Ω resistor → GND
```

### GA25-370 Motor — All 6 Wires

Each motor has 6 wires. Here is exactly where each one goes:

```
Wire Colour │ What It Is        │ Where It Goes
────────────┼───────────────────┼──────────────────────────────────────
Red         │ Motor power +     │ L298N OUT1 (left) / OUT3 (right)
Black       │ Motor power −     │ L298N OUT2 (left) / OUT4 (right)
Green       │ Encoder VCC 5V    │ Nano 5V pin
Dark Blue   │ Encoder GND       │ Common GND rail
Yellow      │ Encoder Channel A │ Leave unconnected (not used)
White       │ Encoder Channel B │ Leave unconnected (not used)
```

> The green and dark blue encoder wires MUST still be connected to 5V
> and GND. If you leave them floating the encoder board may draw
> unpredictable current and cause noise on the power rail.
> The yellow and white wires are safe to leave unconnected.

### L298N Motor Driver — All Terminals

```
L298N Terminal │ Connected To                      │ Why
───────────────┼───────────────────────────────────┼──────────────────────
12V            │ Battery pack (+) ~11.1V            │ Power for the motors
GND            │ Common GND rail                    │ Ground reference
5V output      │ NOT USED — leave unconnected       │ Too noisy for logic
IN1            │ Nano D7                            │ Left motor direction A
IN2            │ Nano D8                            │ Left motor direction B
IN3            │ Nano D4                            │ Right motor direction A
IN4            │ Nano D5                            │ Right motor direction B
ENA            │ Nano D9  (REMOVE the ENA jumper)   │ Left motor PWM speed
ENB            │ Nano D6  (REMOVE the ENB jumper)   │ Right motor PWM speed
OUT1 / OUT2    │ Left  motor red / black wires      │ Power to left motor
OUT3 / OUT4    │ Right motor red / black wires      │ Power to right motor
```

> ⚠️ REMOVE the two small yellow jumpers on ENA and ENB from the L298N
> board before wiring. If you leave them in, the motors always run at
> full speed and the Nano cannot control them at all.

### Rocker Switch

```
Nano D10 ──── [ROCKER SWITCH] ──── GND
```

One pin of the switch goes to D10. The other pin goes to GND.
That is all. No resistors needed — the Nano has an internal pull-up.

### LED

```
Nano D11 ──── 330Ω resistor ──── LED (+) long leg ──── LED (−) short leg ──── GND
```

The 330Ω resistor MUST be in series with the LED. Without it the LED
will burn out instantly the first time it turns on.

---

## 4. The Rocker Switch and LED

| LED State | What It Means |
|-----------|---------------|
| OFF | Switch is OFF — robot is safe to handle |
| SLOW BLINK (200ms on / 800ms off) | Waiting for switch to be turned ON |
| FAST BLINK (100ms on / 100ms off) | Switch is ON — 5-second countdown |
| SOLID ON | Robot is driving forward |

### Step-by-Step at a Competition

1. Place the robot on the ring — keep your hand on it
2. Flip the rocker switch to ON
3. LED blinks fast — this is the 5-second mandatory countdown
4. When LED goes solid ON — release the robot immediately
5. Robot drives forward at full speed
6. Flip switch OFF at any time to stop instantly
7. Flip switch back ON to restart the 5-second delay and go again

---

## 5. What the Code Files Do

```
arduino_autonomous.ino  ← Main sketch. Switch, LED, 5-second delay, forward drive.
config.h                ← All pin numbers and constants. Edit only this file.
motors.h                ← Motor function declarations.
motors.cpp              ← L298N forward and stop implementation.
```

That is all. Four files. Nothing else.

---

## 6. Installing Arduino IDE and Uploading

### Step 1 — Install Arduino IDE

1. Go to https://www.arduino.cc/en/software
2. Download **Arduino IDE 2** for your operating system
3. Install it

### Step 2 — Connect the Nano to Your Laptop

Plug the Arduino Nano into your laptop using a **USB Mini-B cable**.

> Some cheap USB cables are charge-only and carry no data.
> If the Nano does not appear in the port list, try a different cable.

### Step 3 — Configure Arduino IDE

1. Open Arduino IDE
2. Go to **Tools → Board → Arduino AVR Boards → Arduino Nano**
3. Go to **Tools → Processor → ATmega328P**
4. Go to **Tools → Port** → select the port that appeared (e.g. COM3 on Windows)

### Step 4 — Open the Project

1. Go to **File → Open**
2. Navigate to the `arduino_autonomous` folder
3. Open `arduino_autonomous.ino`
4. Arduino IDE will automatically load config.h, motors.h, and motors.cpp

### Step 5 — Upload

1. Click the **Upload** button (→ arrow) or press **Ctrl+U**
2. Wait for "Done uploading" at the bottom of the screen
3. The Nano restarts automatically and runs the code immediately

> If upload fails with an error, go to
> **Tools → Processor → ATmega328P (Old Bootloader)** and try again.

---

## 7. Testing Before a Match

### Test 1 — Switch and LED

1. Power on the robot (connect battery)
2. LED should blink slowly — this means it is waiting for the switch
3. Flip the switch ON
4. LED should blink fast for 5 seconds
5. LED goes solid ON
6. Flip switch OFF — LED goes off immediately, motors stop

### Test 2 — Motors

1. Lift the robot off the ground so the wheels spin freely
2. Flip the switch ON and wait 5 seconds
3. Both wheels should spin forward at full speed
4. If one wheel spins backward — swap that motor's red and black wires
   on the L298N output terminals (OUT1/OUT2 or OUT3/OUT4)
5. Flip switch OFF — both wheels stop immediately

### Test 3 — On the Ground

1. Place the robot on a flat surface
2. Flip the switch ON and wait 5 seconds
3. Robot should drive straight forward
4. If it curves left or right, one motor is slightly faster than the other
   — this is normal for L298N. You can reduce SPEED_FULL slightly in
   config.h or physically adjust the motor mounting angle

---

## 8. Troubleshooting

### Robot does not move at all
- Check rocker switch is ON (LED should be blinking or solid)
- Check battery voltage — 3S pack should read ~11.1V fully charged
- Check L298N ENA and ENB jumpers are removed
- Check all GND wires share a common ground rail

### One motor does not spin
- Check the red and black wires on that motor's L298N output terminals
- Check IN1/IN2 (left) or IN3/IN4 (right) are wired to the correct Nano pins
- Check ENA (D9) or ENB (D6) is connected and the jumper is removed

### One motor spins backward
- Swap the red and black wires on that motor's L298N output terminals
- Do not change any code — physical wire swap is the correct fix

### Robot drives but curves badly
- One motor is stronger than the other — this is normal
- Reduce SPEED_FULL in config.h slightly (e.g. 230 instead of 255)
  to give the L298N more headroom to balance the motors

### LED does not light up
- Check the 330Ω resistor is in series between D11 and the LED
- Check the LED is the right way around (long leg = + = toward D11)
- Check the LED cathode (short leg) goes to GND

### Upload fails
- Try **Tools → Processor → ATmega328P (Old Bootloader)**
- Try a different USB cable
- Check the correct COM port is selected under Tools → Port

### Nano gets very hot on VIN
- The onboard regulator is dissipating the voltage difference as heat
- 11.1V − 5V = 6.1V × current drawn = heat generated
- Add a small heatsink to the regulator chip on the Nano
- Or use a separate LM2596 buck converter to supply 5V to the Nano
  5V pin instead of using VIN

---

## License
MIT — free to use, modify, and distribute for educational and competition purposes.
