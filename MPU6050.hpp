#ifndef MPU6050_H
#define MPU6050_H

#include "sensor.hpp"
#include <Wire.h>
#include <array>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

#define MPU6050_ADDRESS 0x68


/**
 * Enumeration for Digital Low Pass Filter (DLPF)
 * configuration registers addresses in hexadecimal.
 * These determine the cutoff frequency for the low pass filter.
 */
enum DLPF_CFG {
    DLPF_256HZ = 0x00,
    DLPF_188HZ = 0x01,
    DLPF_98HZ  = 0x02,
    DLPF_42HZ  = 0x03,
    DLPF_20HZ  = 0x04,
    DLPF_10HZ  = 0x05,
    DLPF_5HZ   = 0x06
};

/**
 * Enumeration for LSB sensitivity settings.
 * These determine the sensitivity of the gyro readings,
 * mapped to the corresponding register addresses in hexadecimal.
 * The values are in degrees per second per LSB.
 */
enum LSB_SENSITIVITY {
    LSB_131P0 = 0x00,
    LSB_65P5 = 0x08,
    LSB_32P8 = 0x10,
    LSB_16P4 = 0x18
};

/**
 * Mapping of LSB sensitivity values to their corresponding sensitivity factors.
 * This is used to convert raw gyro readings into meaningful values.
 */
const std::unordered_map<LSB_SENSITIVITY, float> LSB_MAP = {
    {LSB_SENSITIVITY::LSB_131P0, 131.0f},
    {LSB_SENSITIVITY::LSB_65P5, 65.5f},
    {LSB_SENSITIVITY::LSB_32P8, 32.8f},
    {LSB_SENSITIVITY::LSB_16P4, 16.4f}
};

/**
 * Enumeration for MPU6050 register addresses.
 * These are used to read and write data from/to the MPU6050 sensor.
 */
enum MPU6050_REG {
    GYRO_LPF = 0x1A,
    GYRO_SENS = 0x1B,
    PWR_MGMT_1 = 0x6B,
    GYRO_XOUT_H = 0x43,
    GYRO_XOUT_L = 0x44,
    GYRO_YOUT_H = 0x45,
    GYRO_YOUT_L = 0x46,
    GYRO_ZOUT_H = 0x47,
    GYRO_ZOUT_L = 0x48
};

using MPU_XYZ = std::array<float, 3>;

struct MPU6050 : public Sensor {
private: 
    static constexpr uint16_t MAX_SAMPLES = 500;

public:
    DLPF_CFG filter;
    float sensitivity;
    MPU_XYZ lastGyro;
    MPU_XYZ gyroOffsets = {0.0f, 0.0f, 0.0f};

    MPU6050(
        uint16_t address = MPU6050_ADDRESS,
        uint32_t clk = 400000,
        DLPF_CFG filter = DLPF_256HZ,
        LSB_SENSITIVITY lsb = LSB_65P5,
        bool autoInit = true,
        TwoWire& wireInstance = Wire
    ) : Sensor(address, clk, wireInstance) {
        if (autoInit) {
            initialize(filter, lsb);
        } else {
            this->filter = filter;
            this->sensitivity = LSB_MAP.at(lsb);
        }
    }

    /**
     * Initializes the MPU6050 sensor with the specified filter and sensitivity.
     * This method sets the clock speed, begins I2C communication, and applies initial configurations.
     * @param filter The digital low pass filter configuration to apply.
     * @param lsb The sensitivity setting for the gyro.
     */
    void initialize(DLPF_CFG filter = DLPF_256HZ, LSB_SENSITIVITY lsb = LSB_65P5) {
        wire.setClock(clk);
        wire.begin();
        vTaskDelay(Sensor::I2C_INIT_DELAY_MS);

        // Turn on the device with no extra configs
        powerOn();

        // set up low pass filter
        setLPF(filter);

        // set up sensitivity
        setSensitivity(lsb);
    }

    /**
     * Powers on the MPU6050 sensor.
     * This method writes to the PWR_MGMT_1 register to wake up the sensor.
     */
    void powerOn(void) const {
        writeToReg(MPU6050_REG::PWR_MGMT_1, 0x00);
    }

    /**
     * Sets the digital low pass filter (DLPF) configuration.
     * This method updates the filter setting and writes it to the sensor's register.
     * @param newFilter The new DLPF configuration to apply.
     */
    void setLPF(DLPF_CFG newFilter = DLPF_CFG::DLPF_10HZ) {
        this->filter = newFilter;
        writeToReg(MPU6050_REG::GYRO_LPF, static_cast<uint8_t>(newFilter));                         
    }

    /**
     * Sets the sensitivity of the gyro.
     * This method updates the sensitivity value and writes it to the sensor's register.
     * @param newSensitivity The new sensitivity setting to apply.
     */
    void setSensitivity(LSB_SENSITIVITY newSensitivity = LSB_SENSITIVITY::LSB_65P5) {
        this->sensitivity = LSB_MAP.at(newSensitivity);
        writeToReg(MPU6050_REG::GYRO_SENS, static_cast<uint8_t>(newSensitivity));
    }

    /**
     * Calibrates the gyro by averaging multiple samples.
     * This method collects a specified number of samples and computes the average offsets for each axis.
     * @param samples The number of samples to average for calibration.
     */
    void calibrateGyro(uint16_t samples = MAX_SAMPLES) {
        this -> gyroOffsets = {0.0f, 0.0f, 0.0f};
        MPU_XYZ offsets = this->readGyroSampled(samples);
        this->gyroOffsets = offsets;
    }

    /**
     * Reads the gyro data from the MPU6050 sensor.
     * This method reads raw gyro data from the sensor and applies sensitivity scaling and offsets.
     * @return An MPU_XYZ containing the scaled gyro values for each axis.
     */
    MPU_XYZ readGyro() const{
        MPU_XYZ buffer = {0.0f, 0.0f, 0.0f};

        this->writeToReg(MPU6050_REG::GYRO_XOUT_H);

        {
            UniqueTimedMutex lock(this->i2cMutex, std::defer_lock);
            if (lock.try_lock_for(Sensor::I2C_TIMEOUT_MS)) {
            
                wire.requestFrom(this->address, 6);
                for (size_t i = 0; i < 3; ++i) {
                    if (wire.available() >= 2) {
                        int16_t rawValue = (wire.read() << 8) | wire.read();
                        buffer[i] = (rawValue / this-> sensitivity) - this->gyroOffsets[i];
                    }
                }
            } else {
                // TODO: Some logging - will handle later after base functionality is working
            }
        }
        
        return buffer;
    }

    /**
     * Reads multiple gyro samples and averages them.
     * This method collects a specified number of samples and computes the average for each axis.
     * @param samples The number of samples to average.
     * @return An averaged MPU_XYZ containing the mean values for each axis.
     */
    MPU_XYZ readGyroSampled(uint16_t samples = MAX_SAMPLES) const {
        samples = std::clamp(samples, (uint16_t)30, MAX_SAMPLES);
        std::array<std::vector<float>, 3> gyroSamples;
        for (size_t i = 0; i < 3; ++i) {
            gyroSamples[i].reserve(samples);
        }
        for (int i = 0; i < samples; ++i) {
            MPU_XYZ sample = readGyro();
            for (size_t j = 0; j < 3; ++j) {
                gyroSamples[j].push_back(sample[j]);
            }
        }

        MPU_XYZ filteredGyro = {0.0f, 0.0f, 0.0f};
        for (size_t i = 0; i < 3; ++i) {
            std::array<float, 3> quartiles = this->quartiles(gyroSamples[i]);
            float q1 = quartiles[0];
            float q3 = quartiles[2];
            std::vector<float> filteredValues = this->removeOutliers(gyroSamples[i], q1, q3);
            if (!filteredValues.empty()) filteredGyro[i] = mean(filteredValues);
        }

        return filteredGyro;
    }

};

#endif // MPU6050_H