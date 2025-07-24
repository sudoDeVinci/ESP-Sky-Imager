#pragma once

#include <Wire.h>
#include "I2CManager.hpp"
#include <array>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include <mutex>


using UniqueTimedMutex = std::unique_lock<std::timed_mutex>;


class I2CSensor {
    public:

        virtual bool init() {
            if (!I2CManager::getInstance().registerSensor(*this)) {
                _is_initialized = false;
                return false;
            }
            
            if (!deviceSpecificSetup()) {
                _is_initialized = false;
                return false;
            }

            _is_initialized = true;
            return true;
        }
    
       /**
        * Pure virtual function to be implemented by derived sensor classes.
        * This function is called periodically to update the sensor's readings.
        * Only some sensors do this - other returnthe reading on demand.
        */
        virtual bool update(void) = 0;
    
        uint8_t getAddress() const { return _i2c_addr; }
        int getBusNum() const { return _bus_num; }
        int getSdaPin() const { return _sda_pin; }
        int getSclPin() const { return _scl_pin; }
        uint32_t getMinClock() const { return _min_clock_hz; }
        uint32_t getMaxClock() const { return _max_clock_hz; }
        bool isInitialized() const { return _is_initialized; }
        void setWire(TwoWire* wire) { _wire = wire; }
    
    protected:

        I2CSensor(
            uint8_t addr,
            uint8_t bus_num,
            uint8_t sda,
            uint8_t scl,
            uint32_t min_clk,
            uint32_t max_clk)
            :
            _i2c_addr(addr),
            _bus_num(bus_num),
            _sda_pin(sda),
            _scl_pin(scl),
            _min_clock_hz(min_clk),
            _max_clock_hz(max_clk) {}
    
        /**
         * Pure virtual function to be implemented by derived sensor classes.
         * This function is called during initialization to perform device-specific setup.
         * @return True if the setup is successful, false otherwise.
         */
        virtual bool deviceSpecificSetup() = 0;
    
        bool _is_initialized = false;
        uint8_t _i2c_addr;
        uint8_t _bus_num;
        uint8_t _sda_pin;
        uint8_t _scl_pin;
        uint32_t _min_clock_hz;
        uint32_t _max_clock_hz;
        mutable std::timed_mutex _i2cMutex;
        static constexpr std::chrono::milliseconds I2C_TIMEOUT_MS{100};
        static constexpr TickType_t I2C_DELAY_MS = 5 / portTICK_PERIOD_MS;
        static constexpr TickType_t I2C_INIT_DELAY_MS = 250 / portTICK_PERIOD_MS;

        /**
         * Pointer to the TwoWire instance used for I2C communication.
         * This will be set during registration.
         */
        TwoWire* _wire = nullptr;


        /**
         * Writes a single byte to the specified register.
         * @param reg The register address to write to.
         * @param value The byte value to write.
         */
        void writeToReg(uint8_t reg, uint8_t value) const {
            UniqueTimedMutex lock(_i2cMutex, std::defer_lock);
            if (lock.try_lock_for(I2C_TIMEOUT_MS)) {
                _wire->beginTransmission(_i2c_addr);
                _wire->write(reg);
                _wire->write(value);
                _wire->endTransmission();
            } else {
                // TODO: Some logging - will handle later after base functionality is working
            }
            vTaskDelay(I2C_DELAY_MS);
        }

        /**
         * Writes a single byte to the specified register without any values.
         * This is useful for operations that only require a register address.
         * @param reg The register address to write to.
         */
        void writeToReg(uint8_t reg) const {
            UniqueTimedMutex lock(_i2cMutex, std::defer_lock);
            if (lock.try_lock_for(I2C_TIMEOUT_MS)) {
                _wire->beginTransmission(_i2c_addr);
                _wire->write(reg);
                _wire->endTransmission();
            } else {
                // TODO: Some logging - will handle later after base functionality is working
            }
            vTaskDelay(I2C_DELAY_MS);
        }
        
        /**
         * Writes a command to the sensor.
         * The command is a 16-bit value split into two bytes.
         * @param cmd The command to write, represented as a 16-bit unsigned integer.
         */
        void writeCommand(uint16_t cmd) const {
            std::array<uint8_t, 2> cmdBytes = {
                static_cast<uint8_t>(cmd >> 8), // High byte
                static_cast<uint8_t>(cmd & 0xFF) // Low byte
            };

            UniqueTimedMutex lock(_i2cMutex, std::defer_lock);
            if (lock.try_lock_for(I2C_TIMEOUT_MS)) {
                _wire->beginTransmission(_i2c_addr);
                _wire->write(cmdBytes.data(), cmdBytes.size());
                _wire->endTransmission();
            } else {
                // TODO: Some logging - will handle later after base functionality is working
            }

            vTaskDelay(I2C_DELAY_MS);
        }

        /**
         * Performs a CRC8 calculation on the supplied values.
         * @param data  Pointer to the data to use when calculating the CRC8.
         * @param len   The number of bytes in 'data'.
         * @return The computed CRC8 value.
         */
        uint8_t crc8(const uint8_t *data, int len) const {
            const uint8_t POLYNOMIAL(0x31);
            uint8_t crc(0xFF);

            for (int j = len; j; --j) {
                crc ^= *data++;
                for (int i = 8; i; --i) {
                    crc = (crc & 0x80) ? (crc << 1) ^ POLYNOMIAL : (crc << 1);
                }
            }
            return crc;
        }

        /**
         * Find the mean of an array of numerical type T.
         * @param arr The array of type T with size N.
         * @return The mean value of the array.
         */
        template <typename T, size_t N>
        float mean(const std::array<T, N> &arr) const {
            long sum = std::accumulate(arr.begin(), arr.end(), 0L);
            return sum / (float)N;
        }

        /**
         * Find the mean of an vector of numerical type T.
         * @param arr The vector of type T with size N.
         * @return The mean value of the array.
         */
        template <typename T>
        float mean(const std::vector<T> &vec) const {
            long sum = std::accumulate(vec.begin(), vec.end(), 0L);
            return sum / (float)vec.size();
        }

        /**
         * Find the standard deviation of an array of numerical type T.
         * @tparam T The type of the elements in the array.
         * @tparam N The size of the array.
         * @param arr The array of type T with size N.
         * @param meanValue The mean value to use for the calculation. If NaN, it will be calculated from the array.
         * @return The standard deviation of the array.
         */
        template <typename T, size_t N>
        float stddev(const std::array<T, N> &arr, const float meanValue = NAN) const {
            float mval = std::isnan(meanValue) ? mean(arr) : meanValue;
            if (N == 0) return 0.0f;

            float sum = 0.0f;
            for (size_t i = 0; i < N; ++i) {
                sum += (arr[i] - mval) * (arr[i] - mval);
            }
            return sqrt(sum / (float)N);
        }

        /**
         * Find the standard deviation of a vector of numerical type T.
         * NOTE: If the meanValue is NaN, it will be calculated from the vector.
         * @tparam T The type of the elements in the vector.
         * @param vec The vector of type T.
         * @param meanValue The mean value to use for the calculation. If NaN, it will be calculated from the array.
         * @return The standard deviation of the vector.
         */
        template <typename T>
        float stddev(const std::vector<T> &vec, const float meanValue = NAN) const {
            float mval = std::isnan(meanValue) ? mean(vec) : meanValue;
            if (vec.empty()) return 0.0f;

            float sum = 0.0f;
            for (const T& num : vec) {
                sum += (num - mval) * (num - mval);
            }
            return sqrt(sum / (float)vec.size());
        }

        /**
         * Find the quartiles of an array of numerical type T.
         * NOTE: This function mutates the input array by sorting it.
         * @tparam T The type of the elements in the array.
         * @tparam N The size of the array.
         * @param arr The array of type T with size N.
         * @return An array containing the first quartile, median, and third quartile.
         */
        template <typename T, size_t N>
        std::array<float, 3> quartiles(std::array<T, N> &arr) const {
            std::sort(arr.begin(), arr.end());
            float q1 = arr[N / 4];
            float q3 = arr[(3 * N) / 4];
            float median = arr[N / 2];
            
            std::array<float, 3> quartiles = {q1, median, q3};
            return quartiles;
        }

        /**
         * Find the quartiles of an array of numerical type T.
         * NOTE: This function mutates the input array by sorting it.
         * @tparam T The type of the elements in the array.
         * @param vec The array of type T with size N.
         * @return An array containing the first quartile, median, and third quartile.
         */
        template <typename T>
        std::array<float, 3> quartiles(std::vector<T> &vec) const {
            std::sort(vec.begin(), vec.end());
            size_t N = vec.size();
            float q1 = vec[N / 4];
            float q3 = vec[(3 * N) / 4];
            float median = vec[N / 2];
            
            std::array<float, 3> quartiles = {q1, median, q3};
            return quartiles;
        }

        /**
         * Remove outliers from an array of numerical type T using the IQR method.
         * @param arr The array of type T.
         * @param q1 The first quartile.
         * @param q3 The third quartile.
         * @return A vector containing the filtered values without outliers.
         */
        template <typename T, size_t N>
        std::vector<T> removeOutliers(
            const std::array<T, N> &arr,
            float q1,
            float q3
        ) const {
            float iqr = q3 - q1;
            float lower_bound = q1 - 1.5 * iqr;
            float upper_bound = q3 + 1.5 * iqr;

            std::vector<T> filtered;
            filtered.reserve(N);

            std::copy_if(
                arr.begin(),
                arr.end(),
                std::back_inserter(filtered),
                [lower_bound, upper_bound](const T& value){
                    return value >= lower_bound && value <= upper_bound;
                }
            );

            return filtered;
        }

        /**
         * Remove outliers from a vector of numerical type T using the IQR method.
         * @param vec The vector of type T.
         * @param q1 The first quartile.
         * @param q3 The third quartile.
         * @return A vector containing the filtered values without outliers.
         */
        template <typename T>
        std::vector<T> removeOutliers(
            const std::vector<T> &vec,
            float q1,
            float q3
        ) const {
            float iqr = q3 - q1;
            float lower_bound = q1 - 1.5 * iqr;
            float upper_bound = q3 + 1.5 * iqr;

            std::vector<T> filtered;
            filtered.reserve(vec.size());

            std::copy_if(
                vec.begin(),
                vec.end(),
                std::back_inserter(filtered),
                [lower_bound, upper_bound](const T& value) {
                    return value >= lower_bound && value <= upper_bound;
                }
            );

            return filtered;
        }
    };