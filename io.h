#pragma once

#include "FS.h"
#include "SD_MMC.h"
#include <cstdint>
#include <string>
#include <time.h>

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

#define LOG_FILE "/log.json"
#define CACHE_FILE "/cache.json"
#define NETWORK_FILE "/networks.json"


/**
 * @brief A class to represent a timestamped cache update.
 * This class is used to store data and its associated timestamp for cache updates.
 */
template <typename T>
class Cacheupdate {
    const T data;
    const std::string& timestamp;
};


const std::string& formatTime(tm* now);


bool sdmmcInit(void);


fs::FS& determineFileSystem(void);


void initLogfile(fs::FS& fs);


void clearLogs(fs::FS& fs);


void initCachefile(fs::FS& fs);


const std::string& readFile(fs::FS& fs, const std::string& path);


template <typename T>
void updateCache(fs::FS& fs, const Cacheupdate<T>& update);





