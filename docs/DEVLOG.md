# Dev Log — Autonomous Sumo Robot
**Terra Hack Club**

---

## What I Built

A fully autonomous heavyweight sumo robot that competes without any remote control.
You flip a switch, put it on the ring, and it fights entirely on its own.

The project grew into a **complete multi-controller competition kit** with working
firmware for 4 different microcontrollers:

- **Raspberry Pi Zero 2 W** — Python adaptive FSM, 7 states, quadrature encoders,
  opponent tracker with confidence scoring, adaptive tactic system (RUSHER/STALKER/FLANKER)
- **ESP32** — Dual-core FreeRTOS, hardware LEDC PWM speed control, sensors on Core 0
  and strategy on Core 1 simultaneously — fastest reaction time of all versions
- **Arduino Uno** — 4-phase timed technique timeline with both IR sensors and HC-SR04
- **Arduino Nano** — Same firmware as Uno in compact form factor

All four versions run the same sensor suite: one front-facing HC-SR04 ultrasonic
sensor for opponent detection and two downward-facing IR line sensors for white
boundary edge avoidance. Every version has the 4-phase technique system:
Matador → Bulldozer → Flanker → Berserker.

The chassis is a custom box design made in OpenSCAD and 3D printed. I also built a
manual Bluetooth control mode using the Dabble app on my phone.

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
| Total lines of C++ code | ~1,200 (across Uno, Nano, ESP32) |
| Number of microcontroller versions | 4 (Pi, ESP32, Uno, Nano) |
| Number of FSM states (Pi) | 7 |
| Number of fighting tactics (Pi) | 3 (+ forced RUSHER) |
| Number of technique phases (Arduino/ESP32) | 4 |
| Sensor reading rate (Pi) | 50 Hz |
| Sensor reading rate (ESP32 Core 0) | ~1000 Hz |
| Hardware watchdog timeout (Pi) | 60 seconds |
| 3D printed parts | 4 (chassis, lid, tray, blade) |
| Bugs fixed | 6 documented + many more |

---

*Built for Terra Hack Club*

---

## Entry 12 — Going Multi-Controller: Uno, Nano, and ESP32

After the Pi version was complete and the Arduino Uno build was running well,
I wanted to make this project genuinely useful for other teams at the competition —
not just a single-MCU project.

The problem is that not every team has the same hardware. Some have an Uno, some a Nano,
some an ESP32. The competition doesn't care which brain you use as long as it's autonomous.

So I expanded to four controllers. Here's what I learned from each one:

**Arduino Nano vs Arduino Uno:**
The pins are basically identical (D0–D13, A0–A5 map the same). The only real differences
are the Nano's physical size (18 × 45mm vs Uno's 53 × 69mm — a massive chassis footprint
difference), the Mini-USB connector instead of Type-B, and the clone boards that ship
with an old bootloader. On those clones you have to set *ATmega328P (Old Bootloader)*
in Arduino IDE or the upload fails with a cryptic `stk500_getsync()` error. I wasted
20 minutes on this before figuring it out. Now it's in the README.

**ESP32 — the real upgrade:**
The ESP32 changes everything. The ATmega328P in the Uno/Nano runs at 16 MHz and has a
single core. The ESP32 runs at 240 MHz with two cores. In practice this means:

The ultrasonic sensor polling (`pulseIn()` calls) normally blocks the main loop on
Arduino — you can't do anything else while waiting for the echo. On ESP32 I moved the
entire sensor reading job onto Core 0 as a FreeRTOS task. Core 1 runs the combat
strategy and never waits for sensor data. Both run simultaneously.

This cuts effective reaction time from ~15–25ms on the Uno to ~3–5ms on the ESP32.
In a sumo match that difference is visible — the Matador dodge triggers before the
opponent actually reaches you rather than at the moment of impact.

The ESP32 also has hardware LEDC PWM, which means you get smooth speed control (0–255)
without the audible whine that `analogWrite()` produces on the Uno. The motors run
quieter and the arc maneuvers are much smoother because outer and inner wheel speeds
can be precisely set.

**The ESP32 voltage trap:**
The biggest danger with ESP32 is that it uses 3.3V logic but the HC-SR04 ECHO pin
outputs 5V. That 5V will gradually damage the ESP32 GPIO over time, or instantly
destroy it in a power spike. You need a 1kΩ + 2kΩ resistor voltage divider on the
ECHO line to bring it down to 3.33V.

I also discovered that GPIO34–39 on the ESP32 are input-only and have **no internal
pull-up resistors** — so using GPIO34 for the rocker switch meant the pin floated
when the switch was open, causing random phantom switch triggers. Changed to GPIO13
which has proper internal pull-up support.

**Power for Arduino versions:**
For the Pi, I used an MT3608 boost converter on a single-cell tap to get 5V.
But for the Arduino Uno/Nano and ESP32, I found a simpler solution:
the L298N motor driver has a built-in 78M05 linear regulator. When the 5V-EN jumper
is installed, it puts out a regulated 5V from the motor battery — enough for the
microcontroller, HC-SR04, two IR sensors, and both encoder VCC lines, all from
one power source. No MT3608 needed.

This simplifies the wiring significantly for non-Pi builds. Less wiring = fewer things
to shake loose during a match impact.

---

*Built for Terra Hack Club*

---

## Entry 13 — Debug Audit: What We Found

Before heading into competition, I wanted to do a systematic deep-dive audit of the Arduino code. This wasn't about adding features — it was about finding bugs that could cost us a match. The kind of bugs that don't crash the robot but make it behave unpredictably under pressure.

I went through the entire codebase line by line, verifying every assumption against the hardware datasheets and the L298N truth table. Here's what I checked and what I found.

**What I verified was correct:**

The motorsInit() function calls pinMode() for all four direction pins before calling motorStop(). This seems obvious but it's easy to get wrong — if you write to pins before configuring them as OUTPUT, the L298N starts in an undefined state. I confirmed this was already fixed in an earlier iteration.

I verified every motor function against the L298N truth table. The H-bridge needs specific HIGH/LOW combinations on IN1/IN2 and IN3/IN4 to drive the motors correctly. motorForward() sets LEFT_IN1=HIGH, LEFT_IN2=LOW, RIGHT_IN3=HIGH, RIGHT_IN4=LOW — exactly right. motorReverse() flips them. motorSpinLeft() and motorSpinRight() set opposite directions on left vs right. motorArcLeft() stops the left motor (IN1=LOW, IN2=LOW) while driving right forward. motorArcRight() does the opposite. motorStop() sets all pins LOW. Every single combination matched the datasheet perfectly.

The IR sensors are configured with INPUT_PULLUP in setup(). These are active-LOW sensors — they read LOW when white line is detected. Using INPUT_PULLUP is correct because it prevents the pin from floating if a sensor gets disconnected. The pull-up is redundant (the sensor modules have their own) but it's a safety measure and costs nothing.

The phase timing boundaries use `elapsed < PHASE1_END_MS` to check if we're still in Phase 1. When elapsed equals PHASE1_END_MS exactly, the condition is false and we advance to Phase 2. This is correct — the phases are 0–30s, 30–60s, 60–90s, 90–120s with exclusive upper bounds. No off-by-one error.

Edge avoidance (handleEdge()) runs at the top of every phase function. It returns true if an edge was detected and handled, false otherwise. When it returns true, the caller immediately returns, skipping the rest of that phase's logic. This priority structure is correct — staying in the ring always wins.

The dodgeCount static variable is reset in both setup() and the switch restart block inside loop(). Each match starts with dodgeCount = 0, which is correct behavior.

**Bugs I found and fixed:**

**Bug 1 — pulseIn timeout too short**

The HC-SR04 ultrasonic sensor was configured with a 25ms timeout. At maximum detection range (NO_DETECT_CM = 200cm), the sound round-trip takes about 23.2ms. That leaves only 1.8ms of margin before the timeout fires prematurely. If the sensor is slightly out of calibration or the opponent is at exactly 200cm, the timeout would cut off a valid reading and the robot would think there's nothing there.

I increased the timeout from 25000 microseconds to 30000. Now we have 6.8ms of margin — enough to reliably cover the full detection range even with sensor variance.

**Bug 2 — getClosingSpeed first-call behavior**

The getClosingSpeed() function measures how fast the ultrasonic distance is changing (cm/s) to detect if the opponent is charging. It uses two static variables: prev (last distance reading) and prevT (timestamp of last reading). Both are initialized to 0.

On the very first call, prevT is 0 but millis() might return 3000 (if the match starts 3 seconds after boot). The code calculates dt = (3000 - 0) / 1000.0 = 3 seconds. If the opponent is actually 50cm away moving at 20 cm/s, the calculated closing speed would be completely wrong because dt is huge.

I added a first-call guard: if prevT == 0, initialize prev and prevT and immediately return 0 speed. Now the first call sets up the state without trying to calculate a meaningless speed value. The second call (and all subsequent calls) have valid prev and prevT to work with.

**Bug 3 — dt threshold too small for stable closing speed**

The original code required dt > 0.01 seconds (10ms) before calculating closing speed. Ultrasonic sensors have about ±1cm of jitter on every reading. At 10ms intervals, that jitter dominates the signal. If the true distance is 80cm but the sensor reads 79cm, then 81cm, then 80cm on consecutive calls, the calculated speed will swing wildly: +100 cm/s, -200 cm/s, +100 cm/s. None of those speeds are real — they're just noise.

I increased the threshold to dt > 0.1 seconds (100ms). Now we only calculate speed when we have a 100ms gap between samples. The ±1cm jitter is still there, but over 100ms the opponent's actual movement (several centimeters) is much larger than the noise. The closing speed reading is now stable enough to reliably trigger the Matador dodge.

I also added an upper bound: dt < 1.0 seconds. If the robot spins away from the opponent for a full second and then spins back, the old distance reading is stale. Calculating a closing speed from stale data gives meaningless results. Now we ignore data older than 1 second.

**Bug 4 — Wander step counter overflow**

The wander() function in the Bulldozer phase has a static uint8_t counter called step that cycles through 6 different movement patterns (forward, spin, forward, spin, forward, spin). The original code did step++ with no wraparound logic. After 256 iterations, uint8_t wraps to 0 silently. This isn't catastrophic (the wander pattern just repeats) but it's sloppy and could cause weird behavior if someone later changes how step is used.

I changed step++ to step = (step + 1) % 6. Now the wraparound is explicit and tied to the actual number of cases in the switch statement. If someone adds a 7th wander pattern later, they'll see the % 6 and know to change it to % 7.

**Bug 5 — Berserker attack counter overflow**

The runBerserker() function has a static uint8_t counter called n that increments every time the robot attacks. It uses n % 3 to decide when to do a fake retreat (every 3rd attack). The original code did n++ with no wraparound. Same issue as Bug 4 — after 256 attacks, n wraps to 0 and keeps going.

In this case the overflow is truly harmless because n % 3 produces the same pattern whether n is 252, 253, 254, 255, 0, 1, 2... The modulo operation makes the overflow invisible. But for code clarity I added n = (n + 1) % 255 anyway. Now it's explicit that wraparound is intentional.

**The motor test sketch**

One of the hardest problems when debugging a sumo robot is figuring out whether the issue is in the motor driver hardware or the strategy logic. You make a change to the code, upload it, flip the switch, and the robot drives off the edge immediately. Is it because a motor is wired backwards? Or because the edge avoidance logic has a bug? You don't know.

I created motor_test.ino — a minimal standalone sketch that does nothing but test the motor functions. It doesn't include config.h or motors.h or any of the strategy code. It defines the four motor pins directly (D7, D8, D4, D5) and cycles through all seven motor functions: forward, stop, reverse, stop, spin left, stop, spin right, stop, arc left, stop, arc right, stop. Each function runs for 1 second and prints to Serial so you can see what's happening.

Upload motor_test.ino, open the Serial Monitor, and watch the robot. If forward drives backward, you know you have a motor wiring issue — swap the red and black wires on the L298N output terminal. If arc left makes the robot spin instead of arc, you know the stopped wheel isn't actually stopped — check that IN1 and IN2 are both LOW for the stopped side. If the test sketch works perfectly but the match code doesn't, you know the hardware is fine and the bug is in the strategy logic.

This one file cuts motor debugging time in half. It's also useful for competition day — if the robot behaves weirdly after a hard impact, upload motor_test to verify nothing shook loose.

**What I learned from the audit:**

Static analysis is tedious but it catches bugs that testing never would. The pulseIn timeout bug only manifests at exactly 200cm range — an edge case I'd never hit during desk testing. The getClosingSpeed first-call bug only triggers once per match (the first speed reading) and it might not even cause visible problems depending on when the first opponent detection happens. The dt threshold bug causes intermittent Matador dodge failures that look like random behavior.

None of these bugs would crash the robot. They'd just make it lose matches in ways that seem like bad luck.

Verification against datasheets matters. I thought motorsInit() was correct but I went back and re-read the ATmega328P datasheet to confirm that pinMode() actually configures the Data Direction Register before digitalWrite() writes to the PORT register. It does.

Having a minimal hardware test sketch is essential for any robot project. The 30 minutes I spent writing motor_test.ino will save hours of debugging over the lifetime of this project.

All five bugs are now fixed. The code compiles without warnings. Time to test on the actual ring.

---

*Built for Terra Hack Club*

---

## Entry 14 — Advanced Competition Features

With all the bugs fixed and the core strategy solid, I wanted to add features that real competition teams use to win matches. Not flashy additions — practical techniques that solve specific problems you encounter when fighting other autonomous robots.

I researched sumo robot projects on GitHub, read papers on sensor fusion in mobile robotics, and studied match footage from Japanese mini-sumo competitions. I found four techniques that showed up repeatedly in winning robots: sensor fusion for edge detection, opponent direction memory, rolling average closing speed, and motor kickstart pulses.

The challenge is that the Arduino Uno has only 2KB of RAM. Every feature has to be memory-efficient or it's useless. A naive sensor fusion implementation could eat 50 bytes of RAM just storing a history buffer. A heavyweight opponent tracker with a Kalman filter could take 200 bytes. Neither would fit.

I designed four advanced features that are all individually enable/disable-able via #define flags at the top of advanced_features.h. Each one costs between 0 and 12 bytes of RAM. You can enable all four simultaneously and still have 98% of your RAM budget left for the core strategy code.

**Feature 1 — Sensor Fusion (Majority-Vote Edge Detection)**

In competition, the ring surface can be shiny or there might be overhead lighting that reflects off the black surface. IR line sensors sometimes see those reflections as white line and false-trigger. The robot reverses away from the "edge" when it's actually in the middle of the ring. This costs precious seconds and can throw off the entire fight.

Sensor fusion solves this by reading each IR sensor multiple times (default: 5 samples over 2.5 milliseconds) and only triggering edge avoidance if the majority of samples detect white line. If 3 out of 5 reads say "white line" and 2 say "black ring," the confidence is 60% — probably real. If 1 out of 5 says white line, that's 20% — probably a reflection, ignore it.

This technique comes from a 2007 Springer paper on fuzzy logic control for autonomous mini-sumo robots. The paper describes using fuzzy membership functions to fuse IR sensor data and improve reliability. My implementation uses simple threshold logic instead of full fuzzy rules (lighter memory footprint) but the principle is the same: multiple samples beat single samples.

Japanese competition teams document this on sumo.robojogo.com — they call it "confidence filtering" and most top teams use at least 3-sample averaging on IR sensors.

Memory cost: 0 bytes. The function reads the sensor in a tight loop using a local counter variable. Nothing persists between function calls.

**Feature 2 — Search Memory (Opponent Direction Tracking)**

When the robot loses sight of the opponent during a spin-search, it normally starts scanning 360° from scratch. If the opponent was last seen on the right side and the robot spins left, it's searching in the wrong direction first. It'll take a full 270° spin before it finds the opponent again.

Search memory remembers the last direction the opponent was seen (left or right based on which way the robot was spinning when it detected them) and biases the next search toward that direction. If the opponent was last seen while spinning right, the next search starts by spinning right. If the memory is more than 3 seconds old (MEMORY_DECAY_MS), it expires and the robot falls back to alternating left/right.

This technique shows up in reinforcement learning sumo robot projects like Robot-Sumo-RL by sebastianbrzustowicz on GitHub. The RL version uses a neural network to predict opponent position based on past observations. The Arduino version uses a much simpler heuristic — just remember left/right/center and a timestamp — but it captures the same idea: use what you saw before to inform what you do next.

Iron Reign Robotics (an FTC team) documented their "search pattern memory" strategy in a blog post. After losing visual contact with a game element, they search the quadrant where it was last detected before doing a full field scan. Same principle, different domain.

Memory cost: 6 bytes. A small struct stores lastSeenMs (4-byte uint32_t), lastSeenRight (1-byte bool), and valid (1-byte bool).

**Feature 3 — Rolling Average Closing Speed**

The Matador phase detects opponent charges by measuring how fast the distance is closing. If the distance is decreasing faster than CHARGE_SPEED_CM_S (default: 55 cm/s), the opponent is probably charging hard — time to sidestep and counter.

The problem is ultrasonic jitter. The HC-SR04 has ±1cm accuracy. On one measurement the opponent is 80cm away, on the next it's 81cm, then 79cm, then 80cm again. The calculated closing speed swings wildly: +50 cm/s, -100 cm/s, +50 cm/s. None of those are real speeds — they're just noise from the sensor.

A rolling average smooths this out. Instead of using a single speed reading, maintain a ring buffer of the last 3 speed calculations and return the average. If the buffer contains [40, 60, 50] cm/s, the rolling average is 50 cm/s — stable enough to make a dodge decision. The buffer updates every loop iteration so it adapts quickly but doesn't react to single-sample spikes.

This is temporal consistency filtering from edge device object detection. Computer vision systems use it to prevent "flickering" detections where one video frame sees an object, the next doesn't, the next does again. Filtering over time smooths it out. Same idea applies to ultrasonic range measurements.

Three samples is the sweet spot for sumo. Larger buffers would be smoother but add lag — in a fast fight, reaction time matters. Smaller buffers (2 samples) don't smooth enough. Three samples gives stable readings without sluggish response.

Memory cost: 12 bytes. The ring buffer stores 3 floats (4 bytes each).

**Feature 4 — Motor Kickstart Pulse**

DC motors have higher static friction than kinetic friction. It takes more voltage to start a motor spinning than to keep it spinning. This is called "stiction" and it's especially noticeable in heavyweight sumo robots with large motors and heavy chassis.

A kickstart pulse applies a brief high-power signal (default: 80ms at full power) before settling to the desired speed. This overcomes the static friction hump and gets the motor moving smoothly.

Arduino motor control forums document the "dead-band" problem where motors hum but don't turn at low PWM values (0–25% duty cycle). The voltage isn't enough to overcome static friction. The standard fix is to start at 100% PWM for 50–100ms, then ramp down to the target speed.

In this build, the ENA and ENB jumpers are installed so the motors always run at full power — kickstart has no effect. But the feature is included in advanced_features.h with a note that it only works in PWM builds (where ENA/ENB are wired to Arduino PWM pins for variable speed control). For anyone doing a PWM build, this feature is ready to use without modification.

Memory cost: 0 bytes. Just a timing constant, no state needed.

**How to use these features:**

All four features are disabled by default. To enable one, open advanced_features.h in Arduino IDE and change the corresponding flag from 0 to 1:

```cpp
#define ENABLE_SENSOR_FUSION    1  // Enable majority-vote edge detection
#define ENABLE_SEARCH_MEMORY    1  // Enable opponent direction tracking
#define ENABLE_ROLLING_SPEED    1  // Enable closing speed smoothing
#define ENABLE_KICKSTART        0  // Enable motor kickstart (PWM builds only)
```

Save, re-compile, and upload. The code automatically uses the advanced version of each feature when enabled, or falls back to the default behavior when disabled.

Total RAM with all features enabled: ~18 bytes. That's less than 1% of the Arduino Uno's 2KB budget. Flash usage increases by about 800 bytes (still well under the 32KB limit). You can safely enable everything and still have headroom for future additions.

**What we still want to add:**

These four features solve real problems but there's more we could do. A gyroscope (MPU-6050) would give us accurate spin angle measurement for better opponent tracking — right now we estimate spin angle from motor timings which isn't precise. A rear-facing ultrasonic sensor would let us detect opponents approaching from behind instead of only seeing them when we spin around. Encoder-based straightness correction would make charges more accurate by measuring actual wheel rotation instead of assuming both motors turn at the same speed.

But each of those additions costs RAM, GPIO pins, and complexity. For now, these four features strike the right balance: real competition techniques, minimal resource cost, individually enable-able so you only pay for what you use.

---

*Built for Terra Hack Club*
