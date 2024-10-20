#include "comm.h"

#define FIRMWARE_VERSION "0.0.5.0"

TwoWire wire = TwoWire(0);

// Struct holding sensor object pointers and statuses.
Sensors sensors;

// Struct of network information.
NetworkInfo network;

// BMP sensor object. 
Adafruit_BMP3XX bmp;

// SHT sensor object.
Adafruit_SHT31 sht;

// SSD1306 i2c OLED object.
Adafruit_SSD1306 display;

void setup() {
    lastPressed = millis();

    if (DEBUG == 1) { 
    Serial.begin(115200);
    debugln();
    debugln("Setting up...");
    }

    sdmmcInit();

    SEALEVELPRESSURE_HPA = 1017;
    /**
     * wire.begin(sda, scl)
     * 32,33 for ESP32 "S1" WROVER
     * 41,42 for ESP32 S3
     */
    sensors.wire = &wire;
    sensors.wire -> begin(41,42);

    // Set up the display.
    display = Adafruit_SSD1306(DISPLAY_WIDTH, DISPLAY_HEIGHT, sensors.wire, -1);
    sensors.SCREEN = display;
    setup(&sensors.SCREEN);

    // Set up the SHT31-D.
    sht = Adafruit_SHT31(sensors.wire);
    sensors.SHT = sht;
    setup(&sensors.status, &sensors.SHT);

    // Set up the BMP380.
    sensors.BMP = bmp;
    setup(sensors.wire, &sensors.status, &sensors.BMP);

    // Set up the OV5640.
    setup(&sensors.status);
    debugln();

    
}