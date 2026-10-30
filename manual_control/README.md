# Manual Bluetooth Joystick Control

This folder lets you **drive the robot yourself** using your phone.
You use a free app called **Dabble** which connects to the robot over
Bluetooth and gives you a joystick on your screen to steer it.

No wires. No remote control hardware. Just your phone and the Pi's
built-in Bluetooth.

---

## Table of Contents

1. [What Is In This Folder](#1-what-is-in-this-folder)
2. [How It Works — Simple Explanation](#2-how-it-works--simple-explanation)
3. [Parts You Need](#3-parts-you-need)
4. [Wiring — Exactly the Same as Autonomous Mode](#4-wiring--exactly-the-same-as-autonomous-mode)
5. [Installing the Dabble App on Your Phone](#5-installing-the-dabble-app-on-your-phone)
6. [Setting Up the Pi for Bluetooth](#6-setting-up-the-pi-for-bluetooth)
7. [Installing and Running the Code](#7-installing-and-running-the-code)
8. [How to Drive the Robot](#8-how-to-drive-the-robot)
9. [The Rocker Switch and LED](#9-the-rocker-switch-and-led)
10. [Run on Boot (Optional)](#10-run-on-boot-optional)
11. [Troubleshooting](#11-troubleshooting)

---

## 1. What Is In This Folder

```
manual_control/
  ├── config.py     ← All pin numbers and settings (edit only this file)
  ├── motors.py     ← Controls the motors and reads the encoders
  ├── bluetooth.py  ← Handles the Bluetooth connection to the Dabble app
  ├── main.py       ← The main program — ties everything together
  └── README.md     ← This file
```

This folder is **completely standalone** — it has everything it needs
inside it. You do not need any files from the autonomous robot folder.

---

## 2. How It Works — Simple Explanation

Think of it like a remote-controlled car, but instead of a physical
remote, your phone IS the remote.

```
Your Phone                  Raspberry Pi Zero 2 W
┌──────────────┐            ┌─────────────────────┐
│  Dabble App  │            │                     │
│              │            │   bluetooth.py      │
│  [Joystick]  │──Bluetooth─►   reads joystick    │
│   ↑ ↓ ← →   │            │   values            │
│              │            │        │            │
└──────────────┘            │        ▼            │
                            │   main.py           │
                            │   decides speed     │
                            │        │            │
                            │        ▼            │
                            │   motors.py         │
                            │   spins the wheels  │
                            └─────────────────────┘
```

1. You move the joystick on your phone
2. The Dabble app sends the joystick position to the Pi over Bluetooth
3. The Pi reads the joystick values and calculates how fast each motor
   should spin
4. The motors spin and the robot moves

The joystick works like a tank:
- **Push forward** → both motors go forward → robot drives straight ahead
- **Push backward** → both motors go backward → robot drives straight back
- **Push left** → left motor slows down, right motor stays fast → robot turns left
- **Push right** → right motor slows down, left motor stays fast → robot turns right
- **Push diagonally** → robot makes a smooth curved arc

---

## 3. Parts You Need

Everything is the same as the autonomous robot. No extra parts needed.

| Part | Qty | Notes |
|------|-----|-------|
| Raspberry Pi Zero 2 W | 1 | Must have Bluetooth — Pi Zero 2 W has it built in |
| GA25-370 Motor with Encoder | 2 | Same motors as autonomous mode |
| L298N Motor Driver | 1 | Same driver |
| 18650 Battery Pack (3S) | 1 | Same power setup |
| MT3608 Boost Converter | 1 | Same power setup |
| Black Rocker Switch (2-pin) | 1 | Same switch |
| LED + 330Ω resistor | 1 | Same LED |
| Your phone (iOS or Android) | 1 | Needs the Dabble app installed |

---

## 4. Wiring — Exactly the Same as Autonomous Mode

The wiring for this manual control mode is **100% identical** to the
autonomous robot wiring. Every wire goes to the same place.

Refer to the main README.md in the parent folder for the full wiring
guide. The pin assignments are:

```
Physical Pin │ BCM  │ Connected To
─────────────┼──────┼──────────────────────────────────────────────
Pin  2 (5V)  │  —   │ MT3608 VOUT+ (5.0V regulated)
Pin  6 (GND) │  —   │ Common GND rail
Pin 11       │ BCM17│ L298N IN1   (Left motor direction A)
Pin 12       │ BCM18│ L298N ENA   (Left motor PWM speed)
Pin 13       │ BCM27│ L298N IN2   (Left motor direction B)
Pin 15       │ BCM22│ L298N IN3   (Right motor direction A)
Pin 16       │ BCM23│ L298N IN4   (Right motor direction B)
Pin 29       │ BCM 5│ Left encoder  Channel A (yellow wire)
Pin 31       │ BCM 6│ Left encoder  Channel B (white wire)
Pin 33       │ BCM13│ L298N ENB   (Right motor PWM speed)
Pin 35       │ BCM19│ Right encoder Channel A (yellow wire)
Pin 37       │ BCM26│ Right encoder Channel B (white wire)
Pin 38       │ BCM20│ LED anode via 330Ω resistor → GND
Pin 40       │ BCM21│ Rocker switch → GND
```

> The HC-SR04 ultrasonic sensor and IR sensors are NOT used in manual
> mode. You do not need to connect them for this folder to work.
> But if they are already wired up from the autonomous build, that is
> fine — they will just be ignored.

---

## 5. Installing the Dabble App on Your Phone

### Android
1. Open the **Google Play Store**
2. Search for **"Dabble"** by STEMpedia
3. Install it (it is free)

### iPhone (iOS)
1. Open the **App Store**
2. Search for **"Dabble"** by STEMpedia
3. Install it (it is free)

### Inside the App
Once installed, open Dabble. You will see a list of modules.
You need the **"Gamepad"** module — it has a joystick on the left side.

---

## 6. Setting Up the Pi for Bluetooth

### Step 1 — Install Required Packages

SSH into the Pi and run:

```bash
sudo apt update
sudo apt install -y bluetooth bluez python3-dev libbluetooth-dev
pip install pybluez
```

### Step 2 — Enable Bluetooth Serial Port Profile

The Dabble app uses Bluetooth Classic (not BLE). We need to enable
the Serial Port Profile (SPP) on the Pi:

```bash
sudo nano /etc/systemd/system/bluetooth.service
```

Find the line that says:

```
ExecStart=/usr/lib/bluetooth/bluetoothd
```

Change it to:

```
ExecStart=/usr/lib/bluetooth/bluetoothd --compat
```

Save with Ctrl+X → Y → Enter, then:

```bash
sudo systemctl daemon-reload
sudo systemctl restart bluetooth
sudo sdptool add SP
```

### Step 3 — Make the Pi Discoverable

```bash
sudo hciconfig hci0 up
sudo hciconfig hci0 piscan
sudo hciconfig hci0 name 'SumoBot'
```

Your phone should now be able to see "SumoBot" in its Bluetooth device list.

### Step 4 — Pair Your Phone with the Pi

1. On your phone, go to **Settings → Bluetooth**
2. Make sure Bluetooth is ON
3. Tap **"SumoBot"** in the list of available devices
4. If it asks for a PIN, type **1234** or **0000**
5. The pairing is now saved — you only need to do this once

> You do NOT need to connect from the phone's Bluetooth settings every
> time. The Dabble app handles the connection itself when you open it.

---

## 7. Installing and Running the Code

### Navigate to the Manual Control Folder

```bash
cd ~/sumo-robot/manual_control
```

If you have not set up the Python virtual environment yet:

```bash
cd ~/sumo-robot
python3 -m venv .venv
source .venv/bin/activate
pip install RPi.GPIO pybluez
```

### Run the Manual Control Code

```bash
source ~/sumo-robot/.venv/bin/activate
cd ~/sumo-robot/manual_control
python3 main.py
```

You should see:

```
[INFO] Starting Bluetooth. Open Dabble app and connect to 'SumoBot'.
[INFO] Waiting for Dabble app to connect...
```

The LED will blink slowly while waiting for your phone to connect.

---

## 8. How to Drive the Robot

### Step 1 — Start the Code
Run `python3 main.py` on the Pi (or let it auto-start on boot).
The LED will blink slowly — this means it is waiting for your phone.

### Step 2 — Connect in Dabble
1. Open the Dabble app on your phone
2. Tap the Bluetooth icon in the top-right corner of the app
3. Select **"SumoBot"** from the list
4. Wait for it to say "Connected"
5. The LED on the robot will go **SOLID ON** — robot is ready

### Step 3 — Open the Gamepad Module
1. In Dabble, scroll through the modules
2. Tap **"Gamepad"**
3. You will see a joystick on the left side of the screen

### Step 4 — Drive!

```
Joystick position → What the robot does
─────────────────────────────────────────────────────
Centre (not touching)  → Robot stops completely
Push straight up       → Drive forward at full speed
Push straight down     → Drive backward at full speed
Push straight left     → Spin left in place
Push straight right    → Spin right in place
Push up-left diagonal  → Arc forward-left (curve)
Push up-right diagonal → Arc forward-right (curve)
Push slightly up       → Drive forward slowly
Push fully up          → Drive forward at full speed
```

The joystick is **proportional** — the further you push it from the
centre, the faster the robot goes. A tiny push = slow movement.
A full push = maximum speed.

### Step 5 — Stop the Robot
- **Let go of the joystick** → robot stops immediately
- **Flip the rocker switch to OFF** → motors cut instantly
- **Close the Dabble app** → robot stops (connection lost = stop)

---

## 9. The Rocker Switch and LED

The rocker switch and LED work the same way as in autonomous mode:

```
Pi BCM21 (Pin 40) ──── [ROCKER SWITCH] ──── GND
Pi BCM20 (Pin 38) ──── 330Ω ──── LED (+) ──── LED (−) ──── GND
```

| LED State | What It Means |
|-----------|---------------|
| OFF | Switch is OFF, or code is not running |
| SLOW BLINK (0.15s on / 0.85s off) | Waiting for Dabble app to connect |
| SOLID ON | Dabble connected — robot ready to drive |

| Switch State | What Happens |
|-------------|--------------|
| ON | Robot responds to joystick normally |
| OFF | Motors stop instantly. Pi stays on. Flip back ON to resume. |

---

## 10. Run on Boot (Optional)

If you want the manual control to start automatically when the Pi
powers on (without needing to SSH in):

```bash
sudo nano /etc/systemd/system/sumo-manual.service
```

Paste this:

```ini
[Unit]
Description=Sumo Robot Manual Bluetooth Control
After=bluetooth.target multi-user.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/sumo-robot/manual_control
ExecStart=/home/pi/sumo-robot/.venv/bin/python main.py
Restart=on-failure
RestartSec=3

[Install]
WantedBy=multi-user.target
```

Save with Ctrl+X → Y → Enter, then:

```bash
sudo systemctl daemon-reload
sudo systemctl enable sumo-manual.service
sudo systemctl start  sumo-manual.service
```

> ⚠️ Do NOT enable both sumo.service (autonomous) and sumo-manual.service
> at the same time. They both try to control the motors and will conflict.
> Only enable one at a time.

To switch between autonomous and manual:

```bash
# Switch to autonomous mode
sudo systemctl disable sumo-manual.service
sudo systemctl enable  sumo.service

# Switch to manual mode
sudo systemctl disable sumo.service
sudo systemctl enable  sumo-manual.service

sudo reboot
```

---

## 11. Troubleshooting

### Phone cannot find "SumoBot" in Bluetooth list
- Make sure you ran `sudo hciconfig hci0 piscan` on the Pi
- Make sure Bluetooth is ON on your phone
- Try running `sudo hciconfig hci0 up` again on the Pi
- Restart the Bluetooth service: `sudo systemctl restart bluetooth`

### Dabble connects but robot does not move
- Check the rocker switch is ON (LED should be solid)
- Make sure you are in the **Gamepad** module in Dabble, not another module
- Check the joystick is actually moving (watch the values on screen)
- Run the motor test: `python3 -c "from motors import MotorController; m=MotorController(); m.drive(50,50); import time; time.sleep(2); m.cleanup()"`

### Robot moves in wrong direction
- Swap the red and black wires on that motor's L298N output terminals
- Do not change any code — physical wire swap is the correct fix

### Bluetooth keeps disconnecting
- Keep your phone within 5–10 metres of the robot
- Make sure nothing is blocking the signal (metal chassis can reduce range)
- The Pi Zero 2 W antenna is small — avoid placing it inside a fully
  enclosed metal box

### LED never goes solid (stays blinking)
- The Dabble app is not connected
- Check the Bluetooth pairing was done correctly (Section 6, Step 4)
- Try unpairing and re-pairing from your phone's Bluetooth settings

### `pybluez` install fails
- Run: `sudo apt install -y python3-dev libbluetooth-dev`
- Then try again: `pip install pybluez`

### Permission denied on /dev/rfcomm or Bluetooth socket
- Run the code with sudo: `sudo python3 main.py`
- Or add the pi user to the bluetooth group:
  `sudo usermod -a -G bluetooth pi` then log out and back in

---

## License
MIT — free to use, modify, and distribute for educational and competition purposes.
