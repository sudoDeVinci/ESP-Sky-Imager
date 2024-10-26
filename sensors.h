#pragma once
#ifndef SENSORS_H
#define SENSORS_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
 * A singular timestamped reading from the sensors.
 */
struct Reading : public Printable {
    const char* timestamp;        // Pre-formatted timestamp as a string (char*)
    double temperature;           // Temperature in degrees Celsius
    double humidity;              // Humidity as percentage
    double pressure;              // Pressure in Pascals
    double dewpoint;              // Dew point in degrees Celsius
    double altitude;              // Altitude in meters

    // Constructor with default values for all fields
    Reading(const char* ts = "None", 
            double temp = UNDEFINED, 
            double hum = UNDEFINED, 
            double pres = UNDEFINED, 
            double dew = UNDEFINED,
            double alt = UNDEFINED)
        : timestamp(ts), temperature(temp), humidity(hum), pressure(pres), dewpoint(dew), altitude(alt) {}

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
 * Struct to hold sensor details and functionality.
 */
struct Sensors {

    struct Status {
        bool CAM = false;
        bool SHT = false;
        bool BMP = false;
        bool WIFI = false;
        bool SCREEN = false;
    } status;

    TwoWire *wire;
    camera_fb_t *frame;
    camera_config_t config;
    Adafruit_BMP3XX BMP;
    Adafruit_SHT31 SHT;
    Adafruit_SSD1306 SCREEN;

    Sensors(){}

    Sensors(TwoWire *w) : wire(w) {
        SHT = Adafruit_SHT31();
        BMP = Adafruit_BMP3XX();
        SCREEN = Adafruit_SSD1306();
        status.BMP = initBMP();
        status.SHT = initSHT();
        status.CAM = initCAM();
    }

    bool all_down() {
        return !status.CAM && !status.SHT && !status.BMP && !status.SCREEN;
    }

    bool initSHT() {
        if (!SHT.begin()) {
            debugln("Couldn't find SHT31.");
            return false;
        }

        debugln("SHT31-D found and initialized");
        return true;
    }

    bool initBMP() {
        if (!BMP.begin_I2C(0x77, wire)) {
            debugln("Couldn't find BMP3XX.");
            return false;
        }

        debugln("BMP3XX found and initialized");
        return true;
    }

    esp_err_t cameraTeardown() {
        esp_err_t err = ESP_OK;

        // Deinitialize camera
        err = esp_camera_deinit();

        // Display any errors associated with camera deinitialization
        if (err != ESP_OK) debugf("Error deinitializing camera: %s\n", esp_err_to_name(err));
        else debugf("Camera deinitialized successfully\n");
        
        debugln();
        return err;
    }

    bool initCAM() {
        debugln("Setting up camera...");
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer = LEDC_TIMER_0;
        config.pin_d0 = Y2_GPIO_NUM;
        config.pin_d1 = Y3_GPIO_NUM;
        config.pin_d2 = Y4_GPIO_NUM;
        config.pin_d3 = Y5_GPIO_NUM;
        config.pin_d4 = Y6_GPIO_NUM;
        config.pin_d5 = Y7_GPIO_NUM;
        config.pin_d6 = Y8_GPIO_NUM;
        config.pin_d7 = Y9_GPIO_NUM;
        config.pin_xclk = XCLK_GPIO_NUM;
        config.pin_pclk = PCLK_GPIO_NUM;
        config.pin_vsync = VSYNC_GPIO_NUM;
        config.pin_href = HREF_GPIO_NUM;
        config.pin_sscb_sda = SIOD_GPIO_NUM;
        config.pin_sscb_scl = SIOC_GPIO_NUM;
        config.pin_pwdn = PWDN_GPIO_NUM;
        config.pin_reset = RESET_GPIO_NUM;
        config.xclk_freq_hz = CAMERA_CLK;
        config.frame_size = FRAMESIZE_QHD;
        config.pixel_format = PIXFORMAT_JPEG;
        config.grab_mode = CAMERA_GRAB_LATEST; // Needs to be "CAMERA_GRAB_LATEST" for camera to capture.
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.jpeg_quality = 10;
        config.fb_count = 1;

        /** if PSRAM keep res and jpeg quality.
        * Limit the frame size & quality when PSRAM is not available
        */
        if(!psramFound()) {
            debugln("Couldn't find PSRAM on the board!");
            config.frame_size = FRAMESIZE_SVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
            config.jpeg_quality = 30;
        }

        /**
        * Initialize the camera
        */
        esp_err_t initErr = esp_camera_init(&config);
        if (initErr != ESP_OK) {
            debugf("Camera init failed with error 0x%x", initErr);
            if (initErr == ESP_ERR_NOT_FOUND) debugln("Camera not found");
            esp_err_t deinitErr = cameraTeardown();
            if (deinitErr != ESP_OK) debugf("Camera de-init failed with error 0x%x", deinitErr);
            debugln();
            return false;
        }

        sensor_t * s = esp_camera_sensor_get();
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 1);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
        s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
        s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
        s->set_wb_mode(s, 2);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
        s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
        s->set_aec2(s, 0);           // 0 = disable , 1 = enable
        s->set_ae_level(s, 0);       // -2 to 2
        s->set_aec_value(s, 300);    // 0 to 1200
        s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
        s->set_agc_gain(s, 0);       // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)6);  // 0 to 6 (6 from "timelapse" example sjr, OK)
        s->set_bpc(s, 0);            // 0 = disable , 1 = enable
        s->set_wpc(s, 1);            // 0 = disable , 1 = enable
        s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
        s->set_lenc(s, 1);           // 0 = disable , 1 = enable
        s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
        s->set_vflip(s, 1);          // 0 = disable , 1 = enable
        s->set_dcw(s, 1);            // 0 = disable , 1 = enable
        s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

        delay(100);

        /**
        * Attempt to capture an image with the given settings.
        * If no capture, set camera status to false and de-init camera.
        */
        frame = esp_camera_fb_get();
        if(!frame) {
            debugln("Camera capture failed");
            esp_err_t deinitErr = cameraTeardown();
            if (deinitErr != ESP_OK) debugf("Camera de-init failed with error 0x%x", deinitErr);
            debugln();
            esp_camera_fb_return(frame);
            return false;
        }

        esp_camera_fb_return(frame);
        debugln("Camera configuration complete!");
        return true;
    }

    camera_fb_t* read_cam() {
        debugln("Taking image...");
        for(int i = 0; i < 3; i++) {
            frame = esp_camera_fb_get();
            delay(20);
            esp_camera_fb_return(fb);
            delay(20);
        }
        frame = esp_camera_fb_get();
        delay(20);

        if (!frame) {
            debugln("Camera capture failed");
            return nullptr;
        }

        debugln("Image captured!");
        return frame;
    }

    void read_bmp(Reading *reading, double QNH) {
        if (BMP.performReading()) return;

        double presses[SAMPLES];
        double altids[SAMPLES];
        double tempers_bmp[SAMPLES];
        uint8_t errors = 0;
        uint8_t valid = 0;

        while ( valid < SAMPLES && errors < 5 ) {
            presses[valid] = BMP.readPressure();
            altids[valid] = BMP.readAltitude(QNH);
            tempers_bmp[valid] = BMP.readTemperature();
            if (isnan(presses[valid]) || isnan(altids[valid]) || isnan(tempers_bmp[valid])) {
                errors++;
                continue;
            }
            else valid++;
            delay(50);
        }

        if (errors < 5) {
            reading -> altitude = removeOutliersandGetMean(altids, valid);
            reading -> temperature = removeOutliersandGetMean(tempers_bmp, valid);
            reading -> pressure = removeOutliersandGetMean(presses, valid);
        }
    }

    void read_sht(Reading *reading) {
        double h[SAMPLES];
        double t[SAMPLES];
        uint8_t errors = 0;
        uint8_t valid = 0;

        while (valid < SAMPLES && errors < 5) {
            h[valid] = sht -> readHumidity();
            t[valid] = sht -> readTemperature();
            if (isnan(h[valid]) || isnan(t[valid])) {
                errors++;
                continue;
            }
            else valid++;
            delay(50);
        }

        if( errors < 5 ) { 
            reading -> humidity = removeOutliersandGetMean(h, valid);
            reading -> temperature = removeOutliersandGetMean(t, valid);
        }
    }

    void read(Reading *reading, double QNH) {
        if (status.BMP) read_bmp(reading, QNH);
        if (status.SHT) read_sht(reading);

        if (reading -> temperature != UNDEFINED &&
            reading -> humidity != UNDEFINED &&
            reading -> pressure != UNDEFINED &&
            reading -> altitude != UNDEFINED
            ) {
                reading -> dewpoint = calcDP(reading -> temperature, 
                                             reading -> humidity, 
                                             reading -> pressure, 
                                             reading -> altitude);
        }
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


/**
 * Append a reading object to the log file.
 */
void appendReading(fs::FS &fs, Reading* reading);

/**
 * Read the log file and return an array of readings.
 * WARNING: DYNAMICALLY ALLOCATED HEAP ARRAY.
 */
ReadingLog readLog(fs::FS &fs);

#endif