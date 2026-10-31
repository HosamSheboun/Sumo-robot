# Competition Day Tips — Sumo Robot

This guide covers practical preparation, tuning, and troubleshooting for autonomous sumo robot competitions. All advice is based on real competition experience and community best practices from sumo.robojogo.com, GitHub projects, and documented competition builds.

---

## Hardware Checklist (Pre-Match)

Run through this checklist before every match. Hardware failures are the #1 cause of unexpected losses in autonomous sumo.

**Battery voltage check:**
- Measure battery voltage with a multimeter before each match
- 3S LiPo/18650: must be **above 11.0V** for full motor power
- Below 11.0V: motors lose torque and the robot becomes sluggish
- Below 10.5V: risk of brownout reset during motor startup
- Charge between matches if voltage drops below 11.5V

**ENA/ENB jumpers confirmed seated:**
- The L298N enable jumpers MUST be installed for motors to run
- They can vibrate loose during impacts — press them down firmly before each match
- If motors don't start, this is the first thing to check

**IR sensor sensitivity adjustment:**
- Each IR line sensor module has a potentiometer for threshold adjustment
- Test on the actual competition ring surface (not your desk or carpet)
- Adjust sensitivity so the sensor detects white boundary line but not shiny black ring
- Too sensitive: false edge triggers from reflections
- Not sensitive enough: robot drives off ring before detecting edge
- Indicator LED on the module lights up when white line is detected

**All wires zip-tied, no loose connections:**
- Vibration and impacts will shake loose any wire that isn't secured
- Use zip-ties or hot glue to secure all Dupont jumper wires
- Pay special attention to motor output wires on the L298N screw terminals
- A single loose ground wire can cause intermittent resets that are impossible to debug

**Wheel grub screws tightened:**
- The GA25-370 motor shafts use grub screws to secure the wheels
- If a grub screw loosens, the wheel will slip on the shaft — motor spins but robot doesn't move
- Use a hex key to tighten both grub screws on each wheel
- Apply a tiny drop of threadlocker (e.g., Loctite blue) if screws keep loosening

**5V power confirmed at Uno 5V pin with multimeter:**
- Measure voltage between Uno 5V pin and GND with multimeter
- Should read **4.8V–5.2V** with motors running
- If below 4.8V: L298N 5V regulator can't supply enough current, or wiring has high resistance
- If 0V: 5V-EN jumper is not installed on L298N, or 5V wire is disconnected
- This test catches power supply issues before they cause mid-match resets

---

## Ring Strategy

**START_DELAY_MS: competition vs testing:**
- Competition rules require a 5-second delay (START_DELAY_MS = 5000) before autonomous movement
- During testing at home, set START_DELAY_MS = 1000 (1 second) to speed up iteration
- **Remember to set it back to 5000 before competition**
- The LED blinks rapidly during the delay so you know the robot is alive and counting down

**Positioning robot at match start:**
- Always face toward the center of the ring, not toward the opponent
- If you face the opponent, both robots charge and the heavier one wins by pure momentum
- If you face center, you have room to dodge and execute strategy
- Position the front blade at a slight inward angle — this gives better scooping leverage

**ATTACK_DIST_CM tuning based on ring size:**
- Standard competition ring: 77cm diameter (154cm dohyo in Japan)
- Mini ring: 38cm diameter
- ATTACK_DIST_CM default: 60cm (works for standard ring)
- For mini ring: reduce to 30cm — any larger and you're charging when opponent is on the opposite edge
- For larger custom rings: increase to 80–100cm to start charge sooner

**CHARGE_SPEED_CM_S tuning for opponent speed:**
- CHARGE_SPEED_CM_S = 55 cm/s (default) triggers Matador dodge when opponent approaches fast
- If opponent is fast/aggressive: reduce to 40 cm/s — dodge earlier
- If opponent is slow/passive: increase to 70 cm/s — don't dodge slow approaches, just push back
- If opponent never charges: set to 999 to effectively disable Matador dodge and always charge

---

## Physical Tuning Tips (From Real Competition Experience)

**Low center of gravity:**
- Mount the battery pack as low as possible inside the chassis
- Higher center of gravity = easier to flip when hit from below
- A low-mounted battery also improves traction by pressing the wheels down harder

**Blade angle: 5–10 degrees improves scooping:**
- A blade angled 5–10 degrees upward from horizontal gets under opponents better than 0 degrees
- Too steep (>15 degrees): opponent slides up and over instead of being lifted
- Too shallow (0 degrees): you can't get under low-clearance opponents
- Adjustable blade mounts are ideal — tune angle between matches based on opponent design

**Wheel traction: silicone RC car tires vs stock rubber:**
- The stock rubber wheels on most hobby motors have mediocre traction
- Replace with silicone RC car tires (1/10 scale buggy tires fit GA25-370 wheels)
- Silicone tires dramatically improve pushing power and reduce wheel slip during charges
- Clean tires with isopropyl alcohol between matches — dust and oil reduce traction

**Weight distribution: 60% front / 40% rear:**
- More weight on the front wheels improves pushing traction
- Too much weight in back: front wheels lift during push, you lose traction
- Too much weight in front: rear wheels slip during reverse escapes
- Mount heavy components (battery, motor driver) toward the front half of the chassis

---

## Common Failure Modes

**Robot drives off ring immediately:**
- **Cause:** IR sensor sensitivity too high, or sensors detecting shiny black ring as white line
- **Fix:** Adjust IR sensor potentiometers (turn clockwise to reduce sensitivity)
- **Test:** Hold robot above black ring surface — IR indicator LED should be OFF
- **Test:** Hold robot above white boundary — IR indicator LED should be ON

**Robot never attacks:**
- **Cause 1:** ATTACK_DIST_CM too small — opponent is never within trigger range
- **Fix:** Increase ATTACK_DIST_CM in config.h to 80cm and re-test
- **Cause 2:** HC-SR04 ECHO pin not connected, or TRIG pin not connected
- **Fix:** Check wiring — TRIG to D11, ECHO to D12 on Arduino Uno
- **Test:** Upload motor_test.ino — if motors work, sensor wiring is the issue

**Robot spins forever (never stops searching):**
- **Cause:** Ultrasonic sensor always reading NO_DETECT_CM (200cm) — not detecting anything
- **Fix 1:** Check HC-SR04 wiring — VCC to 5V, GND to GND, TRIG to D11, ECHO to D12
- **Fix 2:** Verify HC-SR04 is facing forward, not blocked by chassis or wires
- **Test:** Open Serial Monitor and add debug output: `Serial.println(readDistance());` in loop()

**Motors don't start:**
- **Cause 1:** ENA/ENB jumpers not installed on L298N (most common!)
- **Fix:** Install both jumpers — they enable the motor outputs
- **Cause 2:** 5V not reaching Arduino Uno
- **Fix:** Check voltage at Uno 5V pin with multimeter — should be 4.8–5.2V
- **Cause 3:** Motor power supply disconnected
- **Fix:** Check 12V battery connection to L298N "12V" screw terminal

**Robot crashes/resets mid-match:**
- **Cause 1:** Voltage drop when motors start — L298N 5V regulator can't supply enough current
- **Fix:** Add 1000µF electrolytic capacitor across L298N 5V output (positive to 5V, negative to GND)
- **Cause 2:** Loose ground wire causing intermittent connection
- **Fix:** Check all GND connections are secure, add zip-ties to prevent vibration
- **Test:** Run motor_test.ino for 5 minutes straight — if it doesn't reset, power supply is stable

**Matador dodge triggers randomly:**
- **Cause:** Ultrasonic jitter causing false high closing speed readings
- **Fix:** Enable ENABLE_ROLLING_SPEED in advanced_features.h to smooth readings
- **Alternative:** Increase CHARGE_SPEED_CM_S threshold from 55 to 70 cm/s (less sensitive)

---

## Tuning Session Protocol

Follow this sequence when tuning a sumo robot for a new ring or after hardware changes.

**Step 1: Verify motors (5 minutes)**
- Upload motor_test.ino
- Open Serial Monitor at 9600 baud
- Verify all 7 motor functions work correctly:
  - FORWARD: both motors spin, robot drives straight forward
  - REVERSE: both motors reverse, robot drives straight backward
  - SPIN LEFT: left motor reverses, right motor forward, robot spins in place
  - SPIN RIGHT: left motor forward, right motor reverses, robot spins in place
  - ARC LEFT: left motor stopped, right motor forward, robot curves left
  - ARC RIGHT: left motor forward, right motor stopped, robot curves right
  - STOP: both motors stop
- If any motor spins the wrong direction: physically swap red/black wires on L298N terminal

**Step 2: Tune IR sensitivity (10 minutes)**
- Place robot on actual competition ring (black surface, white boundary)
- Adjust front IR sensor potentiometer until LED lights up ONLY on white line
- Adjust back IR sensor potentiometer the same way
- Test edge avoidance: upload main sketch, flip switch, manually push robot toward edge
- Robot should reverse away from edge and spin before falling off

**Step 3: Tune EDGE_REVERSE_MS (5 minutes)**
- Set EDGE_REVERSE_MS = 200 and test — if robot still falls off edge, increase to 300
- Set EDGE_REVERSE_MS = 400 and test — if robot reverses too far from edge, reduce to 300
- Goal: robot should reverse just far enough to get all sensors back on black ring surface

**Step 4: Tune ATTACK_DIST_CM (10 minutes)**
- Place an opponent (or a cardboard box) 50cm in front of robot
- Upload sketch, flip switch
- Robot should detect opponent and charge immediately
- If it doesn't charge: increase ATTACK_DIST_CM from 60 to 80
- If it charges when opponent is on opposite side of ring: reduce ATTACK_DIST_CM to 40

**Step 5: Tune technique-specific timings (20 minutes)**
- **MATADOR_SIDESTEP_MS:** Adjust until sidestep dodges a charging opponent by ~10cm
- **MATADOR_REFACE_MS:** Adjust until robot spins to face where opponent ended up after charge
- **SPIN_STEP_MS:** Adjust until robot rotates ~45 degrees per search step
- **FLANKER_ARC_MS:** Adjust until robot arcs wide enough to approach from side (90-degree arc)

**Step 6: Full match simulation (10 minutes)**
- Run 3-minute timer, let robot fight against an opponent (or cardboard box)
- Watch for edge avoidance failures, missed detections, or stuck behaviors
- Note which phase (Matador, Bulldozer, Flanker, Berserker) has issues
- Re-tune constants for that phase and repeat test

---

## Final Pre-Competition Checklist

- [ ] Battery fully charged (>12.0V for 3S)
- [ ] All wires secured with zip-ties
- [ ] ENA/ENB jumpers installed and pressed down firmly
- [ ] Wheel grub screws tightened
- [ ] IR sensors adjusted for competition ring surface
- [ ] START_DELAY_MS set to 5000 (competition mode)
- [ ] Sketch uploaded and verified (no compilation warnings)
- [ ] Switch tested (flip off → robot stops, flip on → robot starts after delay)
- [ ] Full 2-minute test run completed without resets or edge falls

---

*Good luck in competition!*
