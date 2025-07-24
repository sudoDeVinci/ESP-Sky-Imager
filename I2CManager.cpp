#include "I2CManager.hpp"
#include "I2CSensor.hpp"

bool I2CManager::registerSensor(I2CSensor& sensor) {
    uint8_t bus_num = sensor.getBusNum();
    if (_buses.find(bus_num) == _buses.end()) {
        // debug serial print: "Bus not found"
        return false;
    }

    BusInfo& bus_info = _buses[bus_num];

    // Check if the bus is initialized - if not, initialize it
    if (!bus_info.is_initialized) {
        bus_info.sda_pin = sensor.getSdaPin();
        bus_info.scl_pin = sensor.getSclPin();
        bus_info.wire->begin(bus_info.sda_pin, bus_info.scl_pin);
        bus_info.is_initialized = true;
    }

    /**
     * Now technically, if the bus was never initialized, and this sensor
     * is the first one, the following checks would be redundant.
     * However, there may be a case where the bus was perhaps ended
     * and re-initialized, whereby there are still sensors registered.
     */


     /**
      * Check if the sensor is on the right pins being used by the bus
      * it is being registered to - if not, return false.
      */
    if (sensor.getSdaPin() != bus_info.sda_pin || sensor.getSclPin() != bus_info.scl_pin) {
        // debug serial print: "Sensor pins do not match bus pins"
        return false;
    }

    /**
     * Check if the sensor address conflicts with any existing sensors
     * on the bus - if so, return false.
     */
    if (std::any_of(
            bus_info.devices.cbegin(),
            bus_info.devices.cend(),
            [&sensor](const I2CSensor* existing_sensor) {
                return existing_sensor->getAddress() == sensor.getAddress();
            }
        )
    ) {
        // debug serial print: "Sensor address conflict"
        return false;
    }
    


    // Negatiating Clock speeds for sensor along selected bus.
    uint32_t new_potential_clock = sensor.getMaxClock();

    /**
     * If the sensor's potential max clock speed is higher than the bus's current highest,
     * Use the bus's current highest clock speed as the new potential max.
     */
    for (const I2CSensor* existing_sensor : bus_info.devices) {
        if (new_potential_clock > existing_sensor->getMaxClock()) {
            new_potential_clock = existing_sensor->getMaxClock();
        }

    }


    /**
     * Check if this new speed is too low for any existing sensors on the bus.
     */
    if (std::any_of(
        bus_info.devices.cbegin(),
        bus_info.devices.cend(),
        [&new_potential_clock](const I2CSensor* existing_sensor) {
            return new_potential_clock < existing_sensor->getMinClock();
        }
    )){
        // debug serial print: "Sensor address conflict"
        return false;
    }

    /**
     * Check if the new potential clock speed is within the sensor's limits.
     */
    if (new_potential_clock < sensor.getMinClock()) {
        // debug serial print: "New clock speed out of sensor's limits"
        return false;
    }

    /**
     * Everything checks out so now we add the sensor to the bus.
     * 1. set the clock on the wire.
     * 2. set the clock on the bus info.
     * 3. Add the sensor to the bus's device list.
     * 4. Set the wire on the sensor.
     */
    bus_info.wire->setClock(new_potential_clock);
    bus_info.current_clock = new_potential_clock;
    bus_info.devices.push_back(&sensor);
    sensor.setWire(bus_info.wire);

    return true;
};
