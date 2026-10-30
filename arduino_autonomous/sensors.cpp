// sensors.cpp — HC-SR04 ultrasonic distance and IR edge sensor implementation.
//
// HC-SR04 on Arduino Nano:
//   Nano runs at 5V — HC-SR04 ECHO outputs 5V — safe to connect directly.
//   No resistor voltage divider needed (unlike the Raspberry Pi version).
//
// IR sensors:
//   Active-LOW: GPIO reads LOW when white boundary line is detected.
//   Internal pull-up resistors enabled on both IR pins.

#include "sensors.h"

// ── Ultrasonic median filter ──────────────────────────────────────────────────
#define HIST_SIZE 4
static float _history[HIST_SIZE];
static uint8_t _histIdx = 0;
static bool _histFull   = false;

static float _rawDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // pulseIn timeout = 25000µs ≈ 4.25m max range
    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000UL);
    if (duration == 0) return 999.0f;
    return duration * 0.01715f;   // cm = µs × 0.01715
}

static float _median() {
    // Copy history into a temp array and sort (insertion sort, tiny array)
    float tmp[HIST_SIZE];
    uint8_t count = _histFull ? HIST_SIZE : _histIdx;
    for (uint8_t i = 0; i < count; i++) tmp[i] = _history[i];
    for (uint8_t i = 1; i < count; i++) {
        float key = tmp[i];
        int8_t j  = i - 1;
        while (j >= 0 && tmp[j] > key) { tmp[j+1] = tmp[j]; j--; }
        tmp[j+1] = key;
    }
    if (count < 2) return tmp[0];
    return (tmp[count/2 - 1] + tmp[count/2]) / 2.0f;
}

// ── Opponent tracker state ────────────────────────────────────────────────────
static float _confidence   = 0.0f;
static float _bearing      = 0.0f;   // degrees
static float _lastDist     = 999.0f;
static unsigned long _lastDetectMs = 0;

static float _wrapBearing(float a) {
    while (a >  180.0f) a -= 360.0f;
    while (a <= -180.0f) a += 360.0f;
    return a;
}

// ── Public functions ──────────────────────────────────────────────────────────
void sensorsInit() {
    pinMode(TRIG_PIN,     OUTPUT);
    pinMode(ECHO_PIN,     INPUT);
    pinMode(IR_FRONT_PIN, INPUT_PULLUP);
    pinMode(IR_BACK_PIN,  INPUT_PULLUP);
    digitalWrite(TRIG_PIN, LOW);

    // Pre-fill history with out-of-range values
    for (uint8_t i = 0; i < HIST_SIZE; i++) _history[i] = 999.0f;
    _histFull = true;
}

float distanceCm() {
    float raw = _rawDistance();
    _history[_histIdx] = raw;
    _histIdx = (_histIdx + 1) % HIST_SIZE;
    if (_histIdx == 0) _histFull = true;
    return _median();
}

bool edgeFront() { return digitalRead(IR_FRONT_PIN) == LOW; }
bool edgeBack()  { return digitalRead(IR_BACK_PIN)  == LOW; }
bool anyEdge()   { return edgeFront() || edgeBack(); }

void trackerUpdate(float dist_cm, float spin_delta_deg) {
    // Rotate stored bearing by how much the robot turned this tick
    _bearing = _wrapBearing(_bearing - spin_delta_deg);

    if (dist_cm < STALK_DISTANCE_CM) {
        _lastDist       = dist_cm;
        _bearing        = 0.0f;   // ultrasonic is front-facing → bearing 0
        _lastDetectMs   = millis();
        _confidence     = 1.0f;
    } else {
        unsigned long elapsed = millis() - _lastDetectMs;
        float decay = (float)elapsed / (float)LOST_TIMEOUT_MS;
        _confidence = max(0.0f, 1.0f - decay);
    }
}

bool  trackerDetected() { return _confidence > 0.5f; }
bool  trackerLost()     { return _confidence < 0.1f; }
float trackerBearing()  { return _bearing; }
float trackerDistance() { return _lastDist; }
