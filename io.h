#pragma once
#ifndef IO_H
#define IO_H

#include <Arduino.h>
#include <vector>
#include <time.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SD_MMC.h"

#define DEBUG 1

#if DEBUG == 1
#define debug(...) Serial.print(__VA_ARGS__)
#define debugln(...) Serial.println(__VA_ARGS__)
#define debugf(...) Serial.printf(__VA_ARGS__)
#else
#define debug(...)
#define debugln(...)
#define debugf(...)
#endif

#define SD_MMC_CMD  38 //Please do not modify it.
#define SD_MMC_CLK  39 //Please do not modify it. 
#define SD_MMC_D0   40 //Please do not modify it.

#define LOG_FILE "/log.json"
#define CACHE_FILE "/cache.json"
#define NETWORK_FILE "/network.json"

struct cacheUpdate {
  const double value;
  const char* timestamp;
};

/**
 * Initialize the sdcard file system. 
 */
void sdmmcInit(void);

/**
 * Read a file from the sdcard. 
 */
const char* readFile(fs::FS &fs, const char * path);

/**
 * Initialize the log file. 
 */
void initLogFile (fs::FS &fs);

/**
 * Initialize the cache file. 
 */
void initCacheFile (fs::FS &fs);

/**
 * Read the conf file and return a String.
 */
const char* readFile (fs::FS &fs, const char * path);

/**
 * Update the timstamp cache file with a new timestamp.
 */
void updateCache (fs::FS &fs, const char* timestamp, const char* field);

/**
 * Update a numaerical cache field / subfield.
 */
void updateCache (fs::FS &fs, double value, const char* field);

/**
 * Update a numaerical cache field / subfield.
 */
void updateCache (fs::FS &fs, cacheUpdate* update, const char* field);

#endif