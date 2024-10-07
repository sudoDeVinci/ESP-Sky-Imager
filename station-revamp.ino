#include "sensors.h"

void setup() {
    if (DEBUG) { 
        Serial.begin(115200);
        debugln();
        debugln("Setting up...");
    }
}

void loop() {
    Serial.println("OUT");
    // Enter deep sleep mode for the calculated duration
    esp_sleep_enable_timer_wakeup(100 * 1000000LL);  // Convert to microseconds
    esp_deep_sleep_start();
}
