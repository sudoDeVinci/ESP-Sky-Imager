#ifndef SHT31D_H
#define SHT31D_H

#include "sensor.hpp"
#include <Wire.h>
#include <array>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

#define SHT31_ADDRESS 0x44

/**
 * Enumeration for SHT31D command codes in hexadecimal.
 * These commands are used to interact with the SHT31D sensor.
 */
enum CMDS {
    READSTATUS = 0xF32D,
    CLEARSTATUS = 0x3041,
    SOFTRESET = 0x30A2,
    HEATER_ON = 0x306D,
    HEATER_OFF = 0x3066,
    HEATER_STATUS = 0x0d
};

/**
 * Enumeration for SHT31D measurement modes.
 * These modes determine the frequency and type of measurements taken by the sensor.
 */
enum MEASUREMENT_MODE {
    LOWREP = 0x2416,
    MEDREP = 0x2C0B,
    HIGHREP = 0x2400,
    LOWREP_STRETCH  = 0x2C10,
    MEDREP_STRETCH = 0x2C0D,
    HIGHREP_STRETCH = 0x2C06, 
};

struct SHT31 : public Sensor {
private:
    static constexpr uint16_t MAX_SAMPLES = 100;
    /**Internal humidity rep. */
    float humidity;
    /**Internal temperature rep. */
    float temperature;

public:

    SHT31(
        uint16_t address = SHT31_ADDRESS,
        uint32_t clk = 400000,
        bool autoInit = true,
        TwoWire& wireInstance = Wire
    ) : Sensor(address, clk, wireInstance) {
        if (autoInit) {
            initialize();
        }

        this -> humidity = NAN;
        this -> temperature = NAN;
    }

    /**
     * Initializes the SHT31 sensor.
     * This method sets the clock speed and begins I2C communication.
     * It also performs a soft reset to ensure the sensor is in a known state.
     * @return True if initialization was successful, false otherwise.
     */
    bool initialize(void) {
        wire.setClock(clk);
        wire.begin();
        vTaskDelay(Sensor::I2C_INIT_DELAY_MS);
        reset();
        return readStatus() != 0xFFFF;
    }

    /**
     * Resets the SHT31 sensor.
     * This method sends a soft reset command to the sensor.
     * It is recommended to call this method after initialization.
     */
    void reset(void) const {
        writeCommand(SOFTRESET);
        vTaskDelay(Sensor::I2C_INIT_DELAY_MS);
    }

    /**
     * Enables the heater of the SHT31 sensor.
     * This method sends a command to turn on the heater.
     */
    void enableHeater(void) const {
        writeCommand(HEATER_ON);
    }

    /**
     * Disables the heater of the SHT31 sensor.
     * This method sends a command to turn off the heater.
     */
    void disableHeater(void) const {
        writeCommand(HEATER_OFF);
    }

    /**
     * Returns the current humidity reading from the sensor.
     * @return The humidity as a percentage.
     */
    float getHumidity(void) const {
        return this -> humidity;
    }

    /** 
     * Returns the current temperature reading from the sensor.
     * @return The temperature in degrees Celsius.
     */
    float getTemperature(void) const {
        return this -> temperature;
    }

    /**
     * Reads the current status of the SHT31 sensor.
     * This method sends a command to read the status and returns the status code.
     * @return The status code as a 16-bit unsigned integer.
     */
    uint16_t readStatus(void) const {
        writeCommand(READSTATUS);
        uint8_t data[3] = {0, 0, 0};

        UniqueTimedMutex lock(this->i2cMutex, std::defer_lock);
        if (lock.try_lock_for(Sensor::I2C_TIMEOUT_MS)) {
            if (this->wire.requestFrom(this->address, (uint8_t)3) == 3) {
                data[0] = this->wire.read();
                data[1] = this->wire.read();
                data[2] = this->wire.read();

                if (data[2] == crc8(data, 2)) {
                    return (uint16_t)data[0] << 8 | data[1];
                }
            }
        }
        return 0xFFFF; // Indicate error
    }


    /**
     * Reads the temperature and humidity from the SHT31 sensor.
     * This method sends a command to read the temperature and humidity data,
     * processes the data, and updates the internal temperature and humidity values.
     * @return True if the read was successful, false otherwise.
     */
    bool readTempHum(void) {
        writeCommand(MEDREP);
        uint8_t readbuffer[6] = {0, 0, 0, 0, 0, 0};
        vTaskDelay(20 / portTICK_PERIOD_MS);

        {
            UniqueTimedMutex lock(this->i2cMutex, std::defer_lock);
            if (lock.try_lock_for(Sensor::I2C_TIMEOUT_MS)) {
                
                if (this->wire.requestFrom(this->address, (uint8_t)6) != 6) return false;

                for (size_t i = 0; i < 6; ++i) {
                    if (wire.available()) readbuffer[i] = this->wire.read();
                }
            } else {
                return false; // Timeout trying to grab the lock
            }
        }

        if (readbuffer[2] != crc8(readbuffer, 2) || readbuffer[5] != crc8(readbuffer + 3, 2)) {
            return false; // CRC check failed
        }

        int32_t stemp = (int32_t)(((uint32_t)readbuffer[0] << 8) | readbuffer[1]);
        stemp = ((4375 * stemp) >> 14) - 4500;
        this->temperature = (float)stemp / 100.0f;

        uint32_t shum = ((uint32_t)readbuffer[3] << 8) | readbuffer[4];
        shum = (625 * shum) >> 12;
        this->humidity = (float)shum / 100.0f;

        return true;
    }
};

#endif // SHT31D_H