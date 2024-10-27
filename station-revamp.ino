#include "wrapper.h"

#define FIRMWARE_VERSION "0.0.5.0"

TwoWire wire = TwoWire(0);

// Struct holding sensor object pointers and statuses.
Sensors sensors;

// Struct of network information.
NetworkInfo network;

FS* fileSystem;

void setup() {
    if (DEBUG == 1) { 
      Serial.begin(115200);
      debugln("Setting up...");
    }

    // Initialize file system reuirements.
    fileSystem = DetermineFileSystem();
    if (!fileSystem) {
      debugln("Failed to mount any file system");
      delay(100);
      deepSleepMins(SLEEP_MINS);
    }

    initLogFile(*fileSystem);
    initCacheFile(*fileSystem);

    /**
     * wire.begin(sda, scl)
     * 32,33 for ESP32 "S1" WROVER
     * 41,42 for ESP32 S3
     */
    sensors = Sensors(&wire);
    sensors.wire -> begin(41,42);

    wifiSetup(&network, &sensors.status);

    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", "CET-1-CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
    fetchCurrentTime(*fileSystem, &network.TIMEINFO, &sensors.status);
    fetchQNH(*fileSystem, &network.TIMEINFO, &network);
}

void loop() {
  //serverInterop(*fileSystem, &network.TIMEINFO, &sensors, &network);
  debugln("Going to sleep...");
  delay(100);
  deepSleepMins(SLEEP_MINS);
}