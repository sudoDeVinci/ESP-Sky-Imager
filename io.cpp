#include "io.hpp"


/**
 * @brief Format the current time into a string.
 * @param now Reference to a tm structure containing the current time.
 * @return A formatted string representing the current time.
 */
std::string formatTime(const tm& now) {
    // "YYYY-MM-DD HH:MM:SS" + null terminator
    char timestamp[35];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &now);
    return std::string(timestamp);
}


/**
 * Attempt to initialize the sdcard file system. 
 * @return True if the sdcard was successfully mounted, false otherwise.
 */
bool sdmmcInit(void){
    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
    if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
      debugln("Card Mount Failed");
      return false;
    }
    uint8_t cardType = SD_MMC.cardType();
    if(cardType == CARD_NONE){
        debugln("No SD_MMC card attached");
        return false;
    }

    return true;
}


/**
 * Determine the file system to use based on the SD card status.
 * @return The file system to use for file operations.
 */
fs::FS& determineFileSystem(void) {
    if(sdmmcInit()) {
        debugln("SD_MMC mounted");
        return SD_MMC;
    }
    
    LittleFS.begin(true);
    debugln("LittleFS mounted");
    return LittleFS;
}


/**
 * Initialize the log file in the specified file system.
 * @param fs The file system to use for logging.
 */
void initLogfile(fs::FS& fs) {
    if (fs.exists(LOG_FILE)) return;
    File file = fs.open(LOG_FILE, FILE_WRITE, true);
    if(!file){
        debugln("Failed to open log file for writing");
        return;
    }   

    JsonDocument doc;
    doc.createNestedArray("readings");
    if( serializeJson(doc, file) == 0) debugln("Failed to write to log file");
    else debugln("Log file Initialised");
    file.close();
}


/**
 * Initialize the cache file. 
 * @param fs: The file system reference to use for the cache.
 */ 
void initCachefile(fs::FS &fs) {
    // Check if the cache file exists, if not create it.
    if(fs.exists(CACHE_FILE)) return;
    File file = fs.open(CACHE_FILE, FILE_WRITE, true);
    if(!file){
      debugln("Failed to open cache file for writing");
      return;
    }
    
    JsonDocument doc;
    doc["NTP"] = "None";
    doc["SERVER"] = "None";
    doc.createNestedObject("QNH");
    doc["QNH"]["value"] = 0;
    doc["QNH"]["timestamp"] = "None";
    if( serializeJson(doc, file) == 0) debugln("Failed to write to cache file");
    else debugln("Cache file Initialised");
    file.close();
}


/**
 * @brief   Read the contents of a file from the specified file system.
 * @warning This dynamically allocates memory for the file contents - for our use case, this is fine though.
 * @param   fs The file system to read from.
 * @param   path The path to the file to read.
 * @return  The contents of the file as a string.
 */
std::string readFile(fs::FS& fs, const std::string& path) {
    debugf("\nReading file: %s\r\n", path.c_str());
    
    File file = fs.open(path.c_str(), FILE_READ);
    if(!file || file.isDirectory()){
        debugf("Failed to open file %s for reading\n", path.c_str());
        return "";  // Return empty string on error
    }

    size_t fileSize = file.size();
    std::string content;
    content.reserve(fileSize);  // Now reserve() makes sense!
    
    while(file.available()) {
        content += (char)file.read();
    }
    file.close();

    debugf("Read %d bytes from file %s\n", content.size(), path.c_str());
    return content;
}


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
    newReading["dewPoint"] = reading.dewPoint;

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


/**
 * Convert a JsonArray to a vector of SensorContainer::Status objects.
 * @param jsonarray The JsonArray to convert.
 * @return A vector of SensorContainer::Status objects.
 */
std::vector<SensorContainer::Status> statusFromJson(const JsonArray jsonarray) {
    std::vector<SensorContainer::Status> statuses;
    statuses.reserve(jsonarray.size());

    for (const auto& item : jsonarray) {
        SensorContainer::Status status;
        status.SHT = item["SHT"].as<bool>();
        status.BMP = item["BMP"].as<bool>();
        status.WIFI = item["WIFI"].as<bool>();

        statuses.push_back(status);
    }

    return statuses;
}











