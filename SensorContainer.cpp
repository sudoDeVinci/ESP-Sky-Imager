#include "SensorContainer.hpp"

/**
 * @brief Write a new log entry to the log file.
 * @param fs The file system to use for the log.
 * @param reading The EnvironmentalReading object containing sensor data.
 * @return True if the log was successfully updated, false otherwise.
 */
[[nodiscard]]
bool writeLog(fs::FS& fs, const EnvironmentalReading& reading) {
    std::string logContent = readFile(fs, LOG_FILE);
    if (logContent.empty()) {
        debugln("Log file is empty or could not be read.");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, logContent);
    if (error) {
        debugf("Failed to parse log file: %s\n", error.c_str());
        return false;
    }

    JsonArray readings = doc["readings"];
    JsonObject newReading = readings.createNestedObject();
    
    newReading["timestamp"] = reading.timestamp;
    newReading["temperature"] = reading.temperature;
    newReading["humidity"] = reading.humidity;
    newReading["pressure"] = reading.pressure;
    newReading["altitude"] = reading.altitude;
    newReading["dewpoint"] = reading.dewpoint;

    File file = fs.open(LOG_FILE, FILE_WRITE);
    if (!file) {
        debugln("Failed to open log file for writing.");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        debugln("Failed to write updated log to file.");
        file.close();
        return false;
    }

    debugln("Log updated successfully.");
    file.close();

    return true;
}

/**
 * Convert a JsonArray to a vector of EnvironmentalReading objects.
 * @param jsonarray The JsonArray to convert.
 * @return A vector of EnvironmentalReading objects.
 */
std::vector<EnvironmentalReading> arrayFromJson(const JsonArray jsonarray) {
    std::vector<EnvironmentalReading> readings;
    readings.reserve(jsonarray.size());

    for (const auto& item : jsonarray) {

        EnvironmentalReading reading(
            item["timestamp"].as<std::string>(),
            item["temperature"].as<double>(),
            item["humidity"].as<double>(),
            item["pressure"].as<double>(),
            item["altitude"].as<double>(),
            item["dewPoint"].as<double>()
        );

        readings.push_back(reading);
    }

    return readings;
}



