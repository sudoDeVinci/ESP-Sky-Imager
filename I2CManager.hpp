#pragma once

#include <Wire.h>
#include <vector>
#include <map>

// Forward declaration to avoid circular dependency
class I2CSensor;

struct BusInfo {
    bool is_initialized = false;
    int sda_pin = -1;
    int scl_pin = -1;
    uint32_t current_clock = 0;
    std::vector<I2CSensor*> devices;
    TwoWire* wire = nullptr;
};

class I2CManager {
public:
    // Get the singleton instance
    static I2CManager& getInstance() {
        static I2CManager instance;
        return instance;
    }

    // Deleted copy and assignment operators to preserve singleton property
    I2CManager(I2CManager const&) = delete;
    void operator=(I2CManager const&) = delete;

    /**
     * @brief Registers a sensor with the manager.
     * @param sensor Ref to the sensor instance.
     * @return True if registration is successful, false otherwise.
     */
    bool registerSensor(I2CSensor& sensor);


private:
    std::map<int, BusInfo> _buses;
    
    I2CManager() {
        _buses[0].wire = &Wire;
        _buses[1].wire = &Wire1;
    }
};