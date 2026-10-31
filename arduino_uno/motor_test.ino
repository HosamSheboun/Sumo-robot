// =============================================================================
// motor_test.ino — Standalone L298N motor driver test sketch
//
// This sketch tests all 7 motor functions independently with Serial output.
// Useful for hardware diagnosis without running the full match code.
//
// Hardware: Arduino Uno + L298N dual H-bridge
// Motors: 2× GA25-370 12V DC gear motors
// Power: 3S 18650 (~11.1V) → L298N 12V terminal
//        L298N 5V output → Uno 5V pin
//
// Upload this sketch, open Serial Monitor at 9600 baud, and watch the robot
// cycle through each motor function. Verify motors spin in correct directions.
// =============================================================================

// ── L298N Pin Definitions ─────────────────────────────────────────────────────
// ENA and ENB jumpers must be ON for motors to run.
#define LEFT_IN1    7   // D7  → L298N IN1  (left motor direction bit A)
#define LEFT_IN2    8   // D8  → L298N IN2  (left motor direction bit B)
#define RIGHT_IN3   4   // D4  → L298N IN3  (right motor direction bit A)
#define RIGHT_IN4   5   // D5  → L298N IN4  (right motor direction bit B)

// ── Motor Control Functions ───────────────────────────────────────────────────
void motorForward() {
    digitalWrite(LEFT_IN1, HIGH);  digitalWrite(LEFT_IN2, LOW);
    digitalWrite(RIGHT_IN3, HIGH); digitalWrite(RIGHT_IN4, LOW);
}

void motorReverse() {
    digitalWrite(LEFT_IN1, LOW);   digitalWrite(LEFT_IN2, HIGH);
    digitalWrite(RIGHT_IN3, LOW);  digitalWrite(RIGHT_IN4, HIGH);
}

void motorSpinLeft() {
    digitalWrite(LEFT_IN1, LOW);   digitalWrite(LEFT_IN2, HIGH);
    digitalWrite(RIGHT_IN3, HIGH); digitalWrite(RIGHT_IN4, LOW);
}

void motorSpinRight() {
    digitalWrite(LEFT_IN1, HIGH);  digitalWrite(LEFT_IN2, LOW);
    digitalWrite(RIGHT_IN3, LOW);  digitalWrite(RIGHT_IN4, HIGH);
}

void motorArcLeft() {
    digitalWrite(LEFT_IN1, LOW);   digitalWrite(LEFT_IN2, LOW);   // left stopped
    digitalWrite(RIGHT_IN3, HIGH); digitalWrite(RIGHT_IN4, LOW);  // right forward
}

void motorArcRight() {
    digitalWrite(LEFT_IN1, HIGH);  digitalWrite(LEFT_IN2, LOW);   // left forward
    digitalWrite(RIGHT_IN3, LOW);  digitalWrite(RIGHT_IN4, LOW);  // right stopped
}

void motorStop() {
    digitalWrite(LEFT_IN1, LOW);   digitalWrite(LEFT_IN2, LOW);
    digitalWrite(RIGHT_IN3, LOW);  digitalWrite(RIGHT_IN4, LOW);
}

// =============================================================================
// SETUP AND MAIN LOOP
// =============================================================================
void setup() {
    Serial.begin(9600);
    
    // Configure all motor pins as outputs
    pinMode(LEFT_IN1,  OUTPUT);
    pinMode(LEFT_IN2,  OUTPUT);
    pinMode(RIGHT_IN3, OUTPUT);
    pinMode(RIGHT_IN4, OUTPUT);
    
    // Start with motors stopped
    motorStop();
    
    Serial.println("==============================================");
    Serial.println("  L298N Motor Test — Arduino Uno");
    Serial.println("==============================================");
    Serial.println("Testing all 7 motor functions in sequence.");
    Serial.println("Each function runs for 1 second.");
    Serial.println("Cycle repeats forever.\n");
    
    delay(2000);  // Give user time to read header
}

void loop() {
    Serial.println("Testing: FORWARD");
    motorForward();
    delay(1000);
    
    Serial.println("Testing: STOP");
    motorStop();
    delay(500);
    
    Serial.println("Testing: REVERSE");
    motorReverse();
    delay(1000);
    
    Serial.println("Testing: STOP");
    motorStop();
    delay(500);
    
    Serial.println("Testing: SPIN LEFT");
    motorSpinLeft();
    delay(800);
    
    Serial.println("Testing: STOP");
    motorStop();
    delay(500);
    
    Serial.println("Testing: SPIN RIGHT");
    motorSpinRight();
    delay(800);
    
    Serial.println("Testing: STOP");
    motorStop();
    delay(500);
    
    Serial.println("Testing: ARC LEFT");
    motorArcLeft();
    delay(800);
    
    Serial.println("Testing: STOP");
    motorStop();
    delay(500);
    
    Serial.println("Testing: ARC RIGHT");
    motorArcRight();
    delay(800);
    
    Serial.println("Testing: STOP");
    motorStop();
    delay(2000);
    
    Serial.println("\n--- Cycle complete. Repeating... ---\n");
}
