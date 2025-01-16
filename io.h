#pragma once
#ifndef IO_H
#define IO_H

#include <Arduino.h>
#include <vector>
#include <time.h>
#include <ArduinoJson.h>
#include "esp_camera.h"
#include "FS.h"
#include <LittleFS.h>
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

#define MAX_STRING_LENGTH 256
#define MAX_PATH_LENGTH 32

#define SD_MMC_CMD  38 //Please do not modify it.
#define SD_MMC_CLK  39 //Please do not modify it. 
#define SD_MMC_D0   40 //Please do not modify it.

#define LOG_FILE "/log.json"
#define CACHE_FILE "/cache.json"
#define NETWORK_FILE "/networks.json"

struct cacheUpdate {
  const double value;
  const char* timestamp;
};

/**
 * Attempt to initialize the sdcard file system. 
 * @return True if the sdcard was successfully mounted, false otherwise.
 */
bool sdmmcInit(void);

/**
 * Determine the file system to use for IO.
 * If we can't use the sdcard, use the local file system.
 * @return The file system reference to use for IO. 
 */
fs::FS* DetermineFileSystem(void);

/**
 * Sleep for a specified number of minutes. 
 * @param mins: The number of minutes to sleep for.
 */
void deepSleepMins(double mins);

/**
 * Initialize the log file.
 * @param fs: The file system reference to use for the log file. 
 */
void initLogFile (fs::FS &fs);

/**
 * Initialize the cache file. 
 * @param fs: The file system reference to use for the cache.
 */               
void initCacheFile (fs::FS &fs);

/**
 * Read the conf file and return a dynamically allocated const char*.
 * WARNING: Dynamically allocated char array for file output.
 * @param fs: The file system reference to use for the cache.
 * @param path: The path to the file to read.
 * 
 * @return The contents of the file as a char array.
 */
const char* readFile (fs::FS &fs, const char * path);

/**
 * Format the timestamp as MySQL DATETIME.
 * If the year is 1970, return "None".
 * WARNING: Allocating new char[40] every time this function is called.
 * @param timeinfo: tm struct within global Network struct to store the time information.
 * 
 * @return char*: timestamp in MySQL DATETIME format.
 */
char* formattime(tm* now);

/**
 * Update the timstamp cache file with a new timestamp.
 * @param fs: The file system reference to use for the cache.
 * @param timestamp: The new timestamp to write to the cache field.
 * @param field: The field to write the timestamp to.
 */
void updateCache (fs::FS &fs, char* timestamp, const char* field);

/**
 * Update a numaerical cache field.
 * @param fs: The file system reference to use for the cache.
 * @param value: The new value to write to the cache field.
 * @param field: The field to write the value to.
 */
void updateCache (fs::FS &fs, double value, const char* field);

/**
 * Update a nested cache field.
 * @param fs: The file system reference to use for the cache.
 * @param update: The new value - timestamp pair to write to the cache field.
 * @param field: The field to write the value to.
 */
void updateCache (fs::FS &fs, cacheUpdate* update, const char* field);

/**
 * Empty the "readings" array in the log file.
 * @param fs: The file system reference to use.
 */
void clearLog(fs::FS &fs);

/**
 * Replace a substring with another substring in a char array.
 * https://forum.arduino.cc/t/replace-and-remove-char-arrays/485806/5 
 * @param src: The source string to search.
 * @param oldchars: The substring to replace.
 * @param newchars: The substring to replace with.
 */
void str_replace(char *src, char *oldchars, char *newchars);

/**
 * Write a jpg file to the file system.
 * @param fs: The file system reference to use.
 * @param timestamp: The timestamp to use for the file name.
 * @param fb: The camera frame buffer to write to the file.
 */
void writejpg(fs::FS &fs, tm* timestamp, camera_fb_t* fb);

/**
 * Delete a jpg file from the file system.
 * @param fs: The file system reference to use.
 * @param timestamp: The timestamp to use for the file name.
 * 
 * @return True if the file was deleted successfully, false otherwise.
 */
bool deletejpg(fs::FS &fs, tm* timestamp);

/**
 * Read a jpg file from the file system.
 * @param fs: The file system reference to use.
 * @param timestamp: The timestamp to use for the file name.
 * @param fb: The camera frame buffer to read the file to.
 * 
 * @return True if the file was read successfully, false otherwise.
 */
bool readjpg(fs::FS &fs, tm* timestamp, camera_fb_t* fb);

#endif