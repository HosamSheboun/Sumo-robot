# Dev Log — Autonomous Sumo Robot
**Terra Hack Club**

---

## What I Built

A fully autonomous heavyweight sumo robot that competes without any remote control.
You flip a switch, put it on the ring, and it fights entirely on its own.

The robot uses a Raspberry Pi Zero 2 W as its brain, two GA25-370 gear motors,
an HC-SR04 ultrasonic sensor to find opponents, two IR sensors to detect the ring
boundary, and a 3S 18650 lithium battery pack for power. The chassis is a custom
box design I made in OpenSCAD and 3D printed.

The software is a full adaptive Finite State Machine written in Python — 7 states,
3 fighting tactics that switch automatically based on what's happening in the match,
and an opponent tracker that remembers where the enemy probably is even when the
sensor loses sight of it.

I also built two extra versions: an Arduino Uno version with a 2-minute technique
timeline (Matador → Bulldozer → Flanker → Berserker), and a manual Bluetooth
control mode using the Dabble app on my phone.

---

## Entry 1 — The Idea

I found out about a sumo robot competition and immediately wanted to build something
that could actually win, not just drive forward and hope for the best.

The rules are simple: two robots on a black circular ring, first one to push the
other off wins. Weight limit 3kg, size limit 20×20cm. No remote control allowed
in the autonomous class — once the match starts, the robot is completely on its own.

My goal from day one was to build something with real strategy. Not just "spin and
charge" but something that actually adapts to what the opponent is doing.

I picked the Raspberry Pi Zero 2 W because it runs Python, has enough GPIO pins
for everything I need, and is tiny enough to fit inside a compact chassis. The
GA25-370 motors came with built-in encoders which means the robot can measure
exactly how far each wheel has turned — useful for keeping straight during a charge.

---

## Entry 2 — Power Architecture Problem

The first real challenge was power. I had three 18650 cells in series giving ~11.1V
for the motors, but the Pi needs exactly 5V. Too much and it dies instantly.

My first idea was to use the L298N's built-in 5V regulator output. That was wrong —
the L298N's 5V pin is extremely noisy from the motor switching and would have caused
the Pi to crash constantly.

I then looked at the MT3608 boost converter. The problem: it's boost-only, meaning
it can only go UP in voltage. You can't feed it 11.1V and get 5V out.

The solution I came up with was a single-cell tap. Instead of using the full pack,
I tap off just one cell (~3.7V) from inside the battery holder and feed that into
the MT3608. It boosts 3.7V up to exactly 5.0V. The other two cells still power the
motors at full voltage through the L298N.

This was the most important wiring decision in the whole project. Getting it wrong
would have destroyed the Pi.

I also learned that some MT3608 boards can only deliver 0.5A continuously despite
saying "2A" on the label. When the motors start they draw a spike of current that
can momentarily drop the Pi's supply voltage and cause it to reset mid-match. The
fix is a 1000µF capacitor across the MT3608 output to absorb the spike.

---

## Entry 3 — Building the Code Architecture

I decided early on to split the code into 5 separate files with strict separation
of responsibilities:

- `config.py` — every single number in one place. Pin numbers, speeds, distances,
  timings. If I want to change how fast the robot spins during search, I change
  one number in one file.

- `motors.py` — talks to the L298N and reads the encoders. Nothing else.

- `sensors.py` — reads the ultrasonic sensor and IR sensors. Nothing else.

- `strategy.py` — the brain. Makes all decisions. Never touches hardware directly.

- `main.py` — starts everything, runs the loop, handles the switch and LED.

This structure meant I could test each part independently. I could test the motors
without the sensors. I could test the sensors without the strategy. When something
broke I knew exactly which file to look in.

---

## Entry 4 — The Finite State Machine

The strategy is built as a priority FSM — 7 states checked in order every tick,
highest priority first. The first state that applies wins.

```
1. EDGE_AVOID  — white line under IR sensor → escape immediately (always wins)
2. RAM         — opponent < 25cm → burst push
3. ATTACK      — opponent < 80cm → full charge
4. STALK       — opponent 80–150cm → arc toward it
5. ORBIT       — flanking arc (FLANKER tactic only)
6. SEARCH      — nothing detected → spin scan
7. START       — 5-second pre-match pause
```

The priority ordering was the key design decision. EDGE_AVOID always wins because
staying in the ring is more important than anything else — even if the opponent is
right in front of you, if you're about to fall off the edge you reverse first.

The ATTACK state uses encoder feedback to keep the robot driving straight. If the
left wheel is turning faster than the right, the robot is drifting right, so it
slightly reduces the left motor speed to correct. This is called heading correction
and it makes a huge difference in how accurately the robot charges.

---

## Entry 5 — The Opponent Tracker

One problem I ran into: when the robot is spinning to search, the ultrasonic sensor
loses track of where the opponent was. Every time it finishes a spin step it has to
start the search from scratch.

I built an OpponentTracker class to solve this. It keeps a confidence score from
0.0 to 1.0 for how sure it is about the opponent's position. When the sensor
detects the opponent, confidence goes to 1.0. When it loses sight, confidence
decays over time. If confidence drops below 0.1, it switches to full SEARCH mode.

The clever part: it also tracks the robot's own rotation. When the robot spins,
the tracker rotates its estimate of where the opponent is by the same amount.
So when the robot finishes spinning, it already has a rough idea of which direction
to look first instead of starting from zero.

---

## Entry 6 — The Adaptive Tactic System

I wanted the robot to change its fighting style based on what's happening in the
match. I built a TacticAdaptor that monitors two things:

1. How many times the robot has been pushed to the edge (edge_hits counter)
2. How many times a direct charge made the opponent move back (push_successes counter)

Based on these it picks one of three tactics:

- **RUSHER** — charge hard and fast. Default style. Also used if direct charges
  have been working.
- **STALKER** — approach carefully using curved paths. Used if being pushed to edge.
- **FLANKER** — orbit wide around the opponent and attack from the side. Used if
  pushed to the edge 3+ times, meaning the opponent is stronger head-on.

In the final 30 seconds of the match, it forces RUSHER regardless of history.
Time is running out — no more subtlety.

---

## Entry 7 — The Hardware Watchdog

One thing I was worried about: what if the code crashes mid-match? The robot would
just sit there doing nothing.

The Raspberry Pi Zero 2 W has a built-in hardware watchdog timer. The code must
"pet" it every few seconds by writing to `/dev/watchdog`. If the code crashes or
freezes and stops petting it, the watchdog fires and automatically reboots the Pi.

After a reboot, systemd automatically restarts the sumo code because of the
`Restart=on-failure` setting in the service file. The robot recovers by itself
within about 10 seconds.

I also set the CPU governor to "performance" mode so the Pi never slows down its
clock speed to save power. In a competition you want maximum processing speed
at all times.

---

## Entry 8 — The Arduino Version

Partway through the project I wanted a simpler version that didn't need a Pi at all.
An Arduino Uno is cheaper, simpler to wire, and doesn't need an operating system.

The Arduino version uses a completely different approach: instead of a reactive FSM,
it uses a timed technique timeline. The match is divided into four 30-second phases
and the robot automatically switches fighting style at each boundary.

The four phases are:
- **Matador** (0–30s): bait the opponent's charge, dodge sideways, counter-attack
- **Bulldozer** (30–60s): wander the ring, charge anything found
- **Flanker** (60–90s): arc wide around the opponent, hit from the side
- **Berserker** (90–120s): fake retreat then charge, unpredictable aggression

The Matador phase was the most interesting to design. The robot detects if the
opponent is charging by measuring how fast the distance is closing (cm/s). If it's
closing faster than 55 cm/s and within 80cm, the robot sidesteps right to let the
charge run past, then spins to face where the opponent ended up, then slams into
its side. Like a matador letting the bull run past and then striking.

---

## Entry 9 — The 3D Chassis

I designed the chassis in OpenSCAD — a code-based 3D modelling tool where you
describe shapes with code instead of clicking. This meant I could parametrically
adjust dimensions and re-export STL files instantly.

The design is a box shape with:
- 5mm walls for impact resistance
- A 15mm lower perimeter guard to prevent opponents from getting under the sides
- A separate front blade/wedge that can be replaced if it gets damaged
- A removable lid with 6× M3 fastener positions
- An internal electronics tray to keep components organised
- Underside windows for the IR sensors to see the ring surface

Target envelope: 194 × 184 × 86mm — leaving only 6mm margin under the 200mm limit.

I chose PETG for printing because it's tougher than PLA and handles impacts better.
The front blade is printed at 80% infill to make it as solid as possible.

---

## Entry 10 — Manual Control Mode

I also built a manual control mode using the Dabble app on my phone over Bluetooth.
This was useful for testing — I could drive the robot around and verify the motors
were working correctly before trusting the autonomous code.

The Pi Zero 2 W has built-in Bluetooth. The Dabble app connects over Bluetooth
Classic using the Serial Port Profile (SPP). It sends 7-byte packets containing
joystick X and Y values.

I wrote a packet parser that reads these packets from an RFCOMM socket in a
background thread. The main loop reads the latest joystick values and converts
them to motor speeds using tank-drive mixing:

```
left_speed  = Y + X
right_speed = Y - X
```

Where Y is forward/backward and X is left/right. This gives smooth proportional
steering — push the joystick diagonally and you get a smooth arc turn.

---

## Entry 11 — Bugs I Hit

**Bug: motors not spinning at all**
The ENA and ENB jumpers on the L298N were removed from a previous PWM experiment.
Without the jumpers, the enable pins float and the motors don't run. Putting the
jumpers back fixed it instantly.

**Bug: robot spinning in circles**
One motor was wired with red and black reversed on the L298N output terminals.
The fix is always a physical wire swap — never change code for motor direction.

**Bug: Pi resets when motors start**
The MT3608 couldn't supply enough current during motor startup. Added a 1000µF
capacitor across the output. Problem gone.

**Bug: Arduino dodgeCount never reset**
The dodgeCount variable was declared as `static` inside `loop()`. Static local
variables in Arduino persist forever — even when the switch is turned off and back
on. Moved it to global scope and reset it explicitly on restart.

**Bug: motorArcLeft and motorArcRight were identical to spin functions**
Without PWM, you can't arc by running one motor slower. The correct approach is
to stop one wheel entirely while the other drives. Fixed by using `LOW/LOW` on
the stopped side instead of reversing it.

**Bug: motorsInit calling motorStop before pinMode**
`motorStop()` writes to pins before they're configured as OUTPUT. The write does
nothing and the motor driver starts in an undefined state. Fixed by calling
`pinMode` for all pins first, then `motorStop()`.

---

## What I Learned

**Hardware:**
- Power architecture is the most critical part of any robot build. Get it wrong
  and you destroy expensive components.
- Always verify voltage with a multimeter before connecting sensitive electronics.
- Motor direction is always fixed with physical wire swaps, never code changes.
- Capacitors across power rails absorb transient spikes that would otherwise
  crash microcontrollers.

**Software:**
- Separating concerns into files makes debugging dramatically faster.
- A priority FSM is a clean way to handle multiple competing behaviours.
- Static variables in Arduino persist across function calls — be careful with them.
- Hardware watchdogs are essential for any robot that needs to recover from crashes.

**Design:**
- Parametric CAD (OpenSCAD) is much better than manual modelling for mechanical
  parts that need to be adjusted repeatedly.
- Leave margin in your size calculations — 6mm is not much.
- Print structural parts at high infill. The front blade takes direct impacts.

---

## What's Next

- Add a second ultrasonic sensor facing backward so the robot can detect opponents
  approaching from behind
- Implement PID control for the heading correction instead of the simple proportional
  correction currently used
- Add a gyroscope (MPU-6050) for more accurate spin angle measurement in the
  OpponentTracker
- Test the Matador dodge timing against different opponent speeds and tune
  MATADOR_SIDESTEP_MS and MATADOR_REFACE_MS for the actual competition ring size
- Print the chassis in ASA for better UV and heat resistance

---

## Project Stats

| Metric | Value |
|--------|-------|
| Total lines of Python code | ~800 |
| Total lines of C++ code | ~350 |
| Number of FSM states | 7 |
| Number of fighting tactics | 3 (+ forced RUSHER) |
| Number of technique phases (Arduino) | 4 |
| Sensor reading rate | 50 Hz |
| Hardware watchdog timeout | 60 seconds |
| 3D printed parts | 4 (chassis, lid, tray, blade) |
| Bugs fixed | 6 documented + many more |

---

*Built for Terra Hack Club*
