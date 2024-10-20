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

struct Reading : public Printable {
    const char* timestamp;        // Pre-formatted timestamp as a string (char*)
    double temperature;           // Temperature in degrees Celsius
    double humidity;              // Humidity as percentage
    double pressure;              // Pressure in Pascals
    double dewpoint;              // Dew point in degrees Celsius

    // Constructor with default values for all fields
    Reading(const char* ts = "None", double temp = 0.0, double hum = 0.0, double pres = 0.0, double dew = 0.0)
        : timestamp(ts), temperature(temp), humidity(hum), pressure(pres), dewpoint(dew) {}

    /**
     * Helper function to format a double value to a string with 5 decimal places
     */
    const char* formatDouble(double value, const char* unit) const {
        String result = String(value, 5);  // Format with 5 decimal places
        result.concat(" ");
        result.concat(unit);
        return result.c_str();
    }

    /**
     * Override the printTo method to make this struct printable
     */
    size_t printTo(Print& p) const override {
        size_t n = 0;

        // Print timestamp
        n += p.print("Timestamp: ");
        n += p.print(timestamp);
        n += p.print(" | ");
        
        // Print each sensor reading with the appropriate unit
        n += p.print(formatDouble(temperature, "deg C"));
        n += p.print(" | ");

        n += p.print(formatDouble(humidity, "%"));
        n += p.print(" | ");

        n += p.print(formatDouble(pressure, "Pa"));
        n += p.print(" | ");

        n += p.print(formatDouble(dewpoint, "deg C"));
        n += p.print(" | ");

        // Return the total number of bytes printed
        return n;
    }
};

/**
 * Struct to hold a log of readings.
 */
struct ReadingLog{
    size_t size;
    Reading *readings;
};


extern double SEALEVELPRESSURE_HPA;
extern unsigned long lastPressed;
extern bool PROD;

#endif