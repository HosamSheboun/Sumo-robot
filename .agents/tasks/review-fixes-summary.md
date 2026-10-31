# Review Fixes Summary — Closing Speed Reset Mechanisms

This document describes the fixes applied to resolve the two HIGH severity findings from the code review.

## Issues Fixed

### 1. Match Restart Detection Gap in getClosingSpeed()

**Problem:** When the switch is toggled mid-session (match restart), the static variables in `getClosingSpeed()` persisted from the previous match. On the first call after restart, `prevT` might be 45000ms (from the previous match) and `millis()` might be 48000ms (3 seconds into the new match), producing `dt = 3 seconds` which passes validation but calculates meaningless speed.

**Solution:** Added explicit reset mechanism:
- Added global flag `g_needSpeedReset` to signal match restart
- Added `resetClosingSpeed()` function that sets the flag
- Modified `getClosingSpeed()` to check the flag at the start and zero all static state if set
- Called `resetClosingSpeed()` in the switch restart block in `loop()`

**Code Changes:**
```cpp
// arduino_uno.ino
static bool g_needSpeedReset = false;

static float getClosingSpeed(float dist) {
    static float         prev  = NO_DETECT_CM;
    static unsigned long prevT = 0;
    unsigned long now = millis();
    
    // Check for explicit reset request (match restart)
    if (g_needSpeedReset) {
        prev  = NO_DETECT_CM;
        prevT = 0;
        g_needSpeedReset = false;
    }
    
    // ... rest of function
}

static void resetClosingSpeed() {
    g_needSpeedReset = true;
}
```

### 2. Fragile Rolling Speed Restart Detection

**Problem:** `getRollingClosingSpeed()` tried to detect match restarts by checking if `prevT > now + 1000` (time went backwards). This only works if the Arduino hard reboots and `millis()` resets to 0, which doesn't happen when the switch is toggled — the Uno keeps running and `millis()` keeps counting up. The existing `resetRollingSpeed()` function just called `getRollingClosingSpeed(NO_DETECT_CM)` hoping the time-backwards check would fire, but it wouldn't.

**Solution:** Replaced fragile heuristic with explicit reset mechanism:
- Added global flag `g_needRollingReset` to signal match restart
- Modified `resetRollingSpeed()` to set the flag instead of calling the function
- Modified `getRollingClosingSpeed()` to check the flag at the start and zero all static state if set
- Removed the unreliable `prevT > now + 1000` heuristic entirely

**Code Changes:**
```cpp
// advanced_features.h
static bool g_needRollingReset = false;

float getRollingClosingSpeed(float dist) {
    static float speedBuffer[ROLLING_SAMPLES] = {0, 0, 0};
    static uint8_t bufferIndex = 0;
    static float   prevDist    = NO_DETECT_CM;
    static unsigned long prevT = 0;
    
    // Check for explicit reset request (match restart)
    if (g_needRollingReset) {
        for (uint8_t i = 0; i < ROLLING_SAMPLES; i++) speedBuffer[i] = 0;
        bufferIndex = 0;
        prevDist = NO_DETECT_CM;
        prevT = 0;
        g_needRollingReset = false;
    }
    
    // ... rest of function (removed time-went-backwards check)
}

void resetRollingSpeed() {
    g_needRollingReset = true;
}
```

## Testing Verification

Both fixes follow the same pattern:
1. Global flag signals when reset is needed
2. Function checks flag at the start of execution
3. If flag is set, all static state is zeroed and flag is cleared
4. Reset function is called from the switch restart block in `loop()`

This ensures that every match restart (switch toggle) properly resets the closing speed calculations, preventing stale data from the previous match from contaminating the new match.

## Commit

All changes committed in commit `fix: add explicit reset mechanisms for closing speed calculations` with detailed message describing both fixes.
