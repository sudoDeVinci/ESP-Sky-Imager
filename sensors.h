#pragma once
#ifndef SENSORS_H
#define SENSORS_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp_camera.h"
#include "Adafruit_Sensor.h"
#include "Adafruit_SHT31.h"
#include "Adafruit_BMP3XX.h"
#include "io.h"

#define MAGNUS_A 17.625
#define MAGNUS_B 243.04
#define CAMERA_CLK 5000000
#define CAMERA_MODEL_ESP32S3_EYE
#define UNDEFINED -69420.00
#define SAMPLES 100
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define SLEEP_MINS 20
#define BUTTON_PIN 47

#include "camera_pins.h"


/**
 * Struct to hold sensor details and functionality.
 */
struct Sensors {
    TwoWire *wire;
    Adafruit_BMP3XX BMP;
    Adafruit_SHT31 SHT;
    Adafruit_SSD1306 SCREEN;
    camera_fb_t *CAM;

    struct Status {
        bool CAM = false;
        bool SHT = false;
        bool BMP = false;
        bool WIFI = false;
        bool SCREEN = false;
    } status;
};

/**
 * A singular timestamped reading taken by all the sensors using char arrays.
 */
struct Reading : public Printable {
    char timestamp[30] = "None";     // Fixed size for timestamp
    char temperature[15] = "None";   // Fixed size for temperature
    char humidity[15] = "None";      // Fixed size for humidity
    char pressure[15] = "None";      // Fixed size for pressure
    char altitude[15] = "None";      // Fixed size for altitude
    char dewpoint[15] = "None";      // Fixed size for dewpoint

    // Override the printTo method to make this struct printable
    size_t printTo(Print& p) const override {
        size_t n = 0;
        n += p.print("Timestamp: ");
        n += p.println(timestamp);

        n += p.print("Temperature: ");
        n += p.println(temperature);

        n += p.print("Humidity: ");
        n += p.println(humidity);

        n += p.print("Pressure: ");
        n += p.println(pressure);

        n += p.print("Altitude: ");
        n += p.println(altitude);

        n += p.print("Dewpoint: ");
        n += p.println(dewpoint);

        return n;  // Return the total number of bytes printed
    }
};

extern double SEALEVELPRESSURE_HPA;
extern unsigned long lastPressed;
extern bool PROD;

#endif