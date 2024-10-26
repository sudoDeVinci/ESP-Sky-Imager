#include "wrapper.h"

#define FIRMWARE_VERSION "0.0.5.0"

TwoWire wire = TwoWire(0);

// Struct holding sensor object pointers and statuses.
Sensors sensors;

// Struct of network information.
NetworkInfo network;

void setup() {
    if (DEBUG == 1) { 
      Serial.begin(115200);
      debugln("Setting up...");
    }

    // Initialize file system reuirements.
    sdmmcInit();
    initLogFile(SD_MMC);
    initCacheFile(SD_MMC);
    /**
     * wire.begin(sda, scl)
     * 32,33 for ESP32 "S1" WROVER
     * 41,42 for ESP32 S3
     */
    sensors = Sensors(&wire);
    sensors.wire -> begin(41,42);
    fetchCurrentTime(SD_MMC, &network.TIMEINFO, &sensors.status);
}

void loop() {
  serverInterop(SD_MMC, &network.TIMEINFO, &sensors, &network);
  debugln("Going to sleep...");
  delay(100);
  deepSleepMins(SLEEP_MINS);
}