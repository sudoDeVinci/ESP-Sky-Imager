#pragma once

#include "io.hpp"
#include "drivers/SHT31D.hpp"
#include "drivers/BMP3xx.hpp"
#include "camera_pins.h"
#include <string>
#include <array>
#include <vector>
#include <cmath>
#include <cstdio>


extern unsigned long LASTPRESSED;

struct EnvironmentalReading {
    const std::string timestamp;
    double humidity;
    double temperature;
    double pressure;
    double altitude;
    double dewpoint;

    EnvironmentalReading(
        const std::string& ts = "N/A",
        double temp = NAN,
        double hum = NAN,
        double pres = NAN,
        double alt = NAN,
        double dew = NAN
    ): timestamp(ts), 
       humidity(hum), 
       temperature(temp), 
       pressure(pres), 
       altitude(alt), 
       dewpoint(dew) {}

    /**
     * Convert the Reading object to a string representation.
     * This function formats the reading data into a human-readable string.
     * Each double value is formatted to 4 decimal places.
     * * @return A string representation of the Reading object.
     */
    const std::string toString(void) const {

        /**
         * An ISO 8601 timestamp is at most a 30 character string.
         * Each double is at most 9 characters long, including the decimal point.
         * Add a comma and space after each value, except the last one.
         * The format is: "timestamp, humidity, temperature, pressure, altitude, dewpoint".
         * Let's add another 5 for miscellaneous characters (e.g., spaces, commas).
         * 30 + (5 * 9) + 10 + 5 = 85 chars.
         * This is a worst-case estimate in an attempt to over-allocate the string.
         * 
         * While this isn't great for our heap, the device restarts after every duty-cycle,
         * so the heap is cleared at that point.
         */
        int strlen = 85;
        char buffer[strlen]  = "";

        snprintf(buffer, strlen,
                 "%s, %.4f, %.4f, %.4f, %.4f, %.4f",
                 timestamp.c_str(),
                 humidity,
                 temperature,
                 pressure,
                 altitude,
                 dewpoint
        );

        return std::string(buffer);
    }

    /**
     * Convert the Reading object to a JSON string representation.
     * This function formats the reading data into a JSON string.
     * @return A JSON string representation of the Reading object.
     */
    const std::string toJson(void) const {
        /**
         * The format is: "{\"timestamp\":\"2023-10-01T12:00:00Z\",\"humidity\":50.0,\"temperature\":20.0,\"pressure\":1013.25,\"altitude\":100.0,\"dewpoint\":15.0}"
         * The maximum length of the string is 150 characters.
         * We allocate 200 characters to be safe.
         */
        const int strlen = 200;
        char buffer[strlen] = "";
        snprintf(buffer, strlen,
                 "{\"timestamp\":\"%s\",\"humidity\":%.4f,\"temperature\":%.4f,\"pressure\":%.4f,\"altitude\":%.4f,\"dewpoint\":%.4f}",
                 timestamp.c_str(),
                 humidity,
                 temperature,
                 pressure,
                 altitude,
                 dewpoint
        );

        return std::string(buffer);
    }
};

/**
 * Calculate the dew point from a Reading object.
 * The formula used is the Magnus-Tetens approximation.
 * @param reading The EnvironmentalReading object containing temperature and humidity.
 * @return The calculated dew point in degrees Celsius.
 */
double calculateDewPoint(EnvironmentalReading& reading);



struct SensorContainer {
    public:
        
        struct Status {
            bool SHT = false;
            bool BMP = false;
            bool WIFI = false;

            /**
             * Convert the Status object to a string representation.
             * This function formats the status data into a human-readable string.
             * @return A string representation of the Status object.
             */
            const std::string toJson() const {
                /**
                 * The format is "{\"SHT\":true,\"BMP\":true,\"WIFI\":true}".
                 * The maximum length of the string is 45 characters
                 * We allocate 50 characters to be safe.
                 */
                const int strlen = 50;
                char buffer[strlen] = "";
                snprintf(buffer, strlen,
                         "{\"SHT\":%s,\"BMP\":%s,\"WIFI\":%s}",
                         SHT ? "true" : "false",
                         BMP ? "true" : "false",
                         WIFI ? "true" : "false"
                );

                return std::string(buffer);
            }
        } status;

        SensorContainer(void){;}

        /**
         * Check if all sensors are down.
         * This function checks the status of the SHT, BMP, and WIFI sensors.
         * It returns true if all sensors are not operational.
         * 
         * @return true if all sensors are down, false otherwise.
         */
        bool allDown(void) const {
            return !status.SHT && !status.WIFI;
        }


        /**
         * Check if all sensors are operational.
         * This function checks the status of the SHT, BMP, and WIFI sensors.
         * It returns true if all sensors are up and running.
         * 
         * @return true if all sensors are operational, false otherwise.
         */
        bool allUp(void) const {
            return status.SHT && status.WIFI;
        }
};

/**
 * Convert a JsonArray to a vector of EnvironmentalReading objects.
 * @param jsonarray The JsonArray to convert.
 * @return A vector of EnvironmentalReading objects.
 */
std::vector<EnvironmentalReading> arrayFromJson(const JsonArray jsonarray);

/**
 * @brief Write a new log entry to the log file.
 * @param fs The file system to use for the log.
 * @param reading The EnvironmentalReading object containing sensor data.
 * @return True if the log was successfully updated, false otherwise.
 */
[[nodiscard]]
bool writeLog(fs::FS& fs, const EnvironmentalReading& reading);