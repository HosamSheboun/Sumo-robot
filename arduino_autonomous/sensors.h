// sensors.h — HC-SR04 ultrasonic and IR edge sensor declarations.

#pragma once
#include <Arduino.h>
#include "config.h"

void sensorsInit();
float distanceCm();       // HC-SR04 median-filtered distance in cm (999.0 = no echo)
bool  edgeFront();        // true when front IR sees white boundary line
bool  edgeBack();         // true when back  IR sees white boundary line
bool  anyEdge();          // true if either IR sensor triggered

// Opponent tracker — maintains confidence and estimated bearing
void  trackerUpdate(float dist_cm, float spin_delta_deg);
bool  trackerDetected();  // confidence > 0.5
bool  trackerLost();      // confidence < 0.1
float trackerBearing();   // estimated bearing degrees (0=ahead, +ve=right, -ve=left)
float trackerDistance();  // last known distance cm
