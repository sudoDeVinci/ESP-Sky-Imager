#pragma once

#include <ArduinoJson.h>
#include "FS.h"
#include <LittleFS.h>
#include "SD_MMC.h"
#include <vector>
#include <array>
#include <cstdint>
#include <string>
#include <time.h>

#define DEBUG 1
#define VERSION "1.0.0"

#ifdef DEBUG
    #define debug(...) Serial.print(__VA_ARGS__)
    #define debugln(...) Serial.println(__VA_ARGS__)
    #define debugf(...) Serial.printf(__VA_ARGS__)
#else
    #define debug(...)
    #define debugln(...)
    #define debugf(...)
#endif


#define SD_MMC_CMD  38
#define SD_MMC_CLK  39
#define SD_MMC_D0   40

#define LOG_FILE "//log.json"
#define CACHE_FILE "//cache.json"
#define SERVER_FILE "//server.json"
#define NETWORKS_FILE "//networks.json"

std::string formatTime(const tm& now);


/**
 * @brief A struct to represent a timestamped cache update.
 * This class is used to store data and its associated timestamp for cache updates.
 * @param field The field name within the cache to update.
 * @param data Whatever data we want to add into the cache.
 * @param timestamp Timestamp string for the update
 */
template <typename T>
struct CacheUpdate {
    const std::string field;
    const T data;
    const std::string timestamp;

    CacheUpdate(
        const std::string& field,
        const T& data,
        const std::string& timestamp)
        :
        field(field),
        data(data),
        timestamp(timestamp) {}
};


/**
 * Attempt to initialize the sdcard file system. 
 * @return True if the sdcard was successfully mounted, false otherwise.
 */
bool sdmmcInit(void);


/**
 * Determine the file system to use based on the SD card status.
 * @return The file system to use for file operations.
 */
fs::FS& determineFileSystem(void);


/**
 * Initialize the log file in the specified file system.
 * @param fs The file system to use for logging.
 */
void initLogfile(fs::FS& fs);


/**
 * Initialize the cache file. 
 * @param fs: The file system reference to use for the cache.
 */ 
void initCachefile(fs::FS& fs);


/**
 * @brief   Read the contents of a file from the specified file system.
 * @warning This dynamically allocates memory for the file contents - for our use case, this is fine though.
 * @param   fs The file system to read from.
 * @param   path The path to the file to read.
 * @return  The contents of the file as a string.
 */
std::string readFile(fs::FS& fs, const std::string& path);


/**
 * @brief Update the cache with a new data entry.
 * @param fs The file system to use for the cache.
 * @param update The cache update containing data and timestamp.
 * @return true if the update is successful, otherwise false.
 */
template <typename T>
inline bool updateCache(fs::FS& fs, const CacheUpdate<T>& update) {
    std::string cacheContent = readFile(fs, CACHE_FILE);
    if (cacheContent.empty()) {
        debugln("Cache file is empty or could not be read.");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cacheContent);
    if (error) {
        debugf("Failed to parse cache file: %s\n", error.c_str());
        return false;
    }

    if (!doc.containsKey(update.field)) {
        debugf("Field %s does not exist in cache, creating it.\n", update.field.c_str());
        doc.createNestedObject(update.field);
    }

    doc[update.field]["value"] = update.data;
    doc[update.field]["timestamp"] = update.timestamp;

    File file = fs.open(CACHE_FILE, FILE_WRITE);
    if (!file) {
        debugln("Failed to open cache file for writing.");
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        debugln("Failed to write updated cache to file.");
        return false;
    }

    file.close();
    debugf("Cache file %s updated with field %s.\n", CACHE_FILE, update.field.c_str());
    return true;
}


