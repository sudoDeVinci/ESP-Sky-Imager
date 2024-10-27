#include "io.h"

/**
 * Attempt to initialize the sdcard file system. 
 * @return True if the sdcard was successfully mounted, false otherwise.
 */
bool sdmmcInit(){
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

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  debugf("SD_MMC Card Size: %lluMB\n", cardSize);  
  debugf("Total space: %lluMB\r\n", SD_MMC.totalBytes() / (1024 * 1024));
  debugf("Used space: %lluMB\r\n", SD_MMC.usedBytes() / (1024 * 1024));
  return true;
}

/**
 * Determine the file system to use for IO.
 * If we can't use the sdcard, use the local file system.
 * @return The file system reference to use for IO. 
 */
fs::FS* DetermineFileSystem() {
  if(sdmmcInit()) {
    debugln("SD_MMC mounted");
    return &SD_MMC;
  }
  if(LittleFS.begin(true)) {
    debugln("LittleFS mounted");
    return &LittleFS;
  }
  debugln("Failed to mount any file system");
  return nullptr;
}

/**
 * Sleep for a specified number of minutes. 
 * @param mins: The number of minutes to sleep for.
 */
void deepSleepMins(double mins) {
  esp_sleep_enable_timer_wakeup(mins*60000000);
  esp_deep_sleep_start();
}

/**
 * Initialize the log file.
 * @param fs: The file system reference to use for the log file. 
 */
void initLogFile (fs::FS &fs) {
  // Check if the log file exists, if not create it.
  if(fs.exists(LOG_FILE)) return;
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
void initCacheFile (fs::FS &fs) {
  // Check if the log file exists, if not create it.
  if(fs.exists(CACHE_FILE)) return;
  File file = fs.open(CACHE_FILE, FILE_WRITE, true);
  if(!file){
    debugln("Failed to open log file for writing");
    return;
  }
  
  JsonDocument doc;
  doc["NTP"] = "None";
  doc["SERVER"] = "None";
  doc.createNestedObject("QNH");
  doc["QNH"]["value"] = 0;
  doc["QNH"]["timestamp"] = "None";
  if( serializeJson(doc, file) == 0) debugln("Failed to write to log file");
  else debugln("Log file Initialised");
  file.close();
}

/**
 * Read the conf file and return a dynamically allocated const char*.
 * WARNING: Dynamically allocated char array for file output.
 * @param fs: The file system reference to use for the cache.
 * @param path: The path to the file to read.
 * 
 * @return The contents of the file as a char array.
 */
const char* readFile(fs::FS &fs, const char * path) {
  debugf("\nReading file: %s\r\n", path);

  String output;
  File file = fs.open(path);

  if (!file || file.isDirectory()) {
    debugf("- failed to open %s for reading\r\n", path);
    return nullptr;  // Return nullptr on failure
  }

  while (file.available()) {
    output.concat((char)file.read());
  }

  file.close();

  // Dynamically allocate memory to hold the return string
  char* result = new char[output.length() + 1];
  strcpy(result, output.c_str());
  return result;  // Return the dynamically allocated memory
}

/**
 * Format the timestamp as MySQL DATETIME.
 * If the year is 1970, return "None".
 * WARNING: Allocating new char[40] every time this function is called.
 * 
 * @param timeinfo: tm struct within global Network struct to store the time information.
 * 
 * @return char*: timestamp in MySQL DATETIME format.
 */
char* formattime(tm* now) {
  char *timestamp = new char[30];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", now);
  return timestamp;
}

/**
 * Update the timstamp cache file with a new timestamp.
 * @param fs: The file system reference to use for the cache.
 * @param timestamp: The new timestamp to write to the cache field.
 * @param field: The field to write the timestamp to.
 */
void updateCache (fs::FS &fs, char* timestamp, const char* field) {
  const char* cache = readFile(fs, CACHE_FILE);
  debugf("Updating cache field: %s with timestamp: %s\n", field, timestamp);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  delete[] cache;
  if (error) {
    debugln("Failed to read cache file");
    return;
  }

  doc[field] = timestamp;

  File file = fs.open(CACHE_FILE, FILE_WRITE);
  if(!file){
    debugln("Failed to open cache file for writing");
    return;
  }
  if( serializeJson(doc, file) == 0) debugln("Failed to update cache file");
  else debugln("Cache file updated");
  file.close();
}

/**
 * Update a numaerical cache field.
 * @param fs: The file system reference to use for the cache.
 * @param value: The new value to write to the cache field.
 * @param field: The field to write the value to.
 */
void updateCache (fs::FS &fs, double value, const char* field) {
  if (isnan(value) || isinf(value)) {
    debugln("Invalid value");
    return;
  }

  String fld = String(field);
  const char* cache = readFile(fs, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (error) {
    debugln("Failed to read cache file");
    return;
  }

  doc[fld] = value;

  File file = fs.open(CACHE_FILE, FILE_WRITE);
  if(!file){
    debugln("Failed to open cache file for writing");
    return;
  }
  if( serializeJson(doc, file) == 0) debugln("Failed to update cache file");
  else debugln("Cache file updated");
  file.close();
}

/**
 * Update a nested cache field.
 * @param fs: The file system reference to use for the cache.
 * @param update: The new value - timestamp pair to write to the cache field.
 * @param field: The field to write the value to.
 */
void updateCache (fs::FS &fs, cacheUpdate* update, const char* field) {
  const char* cache = readFile(fs, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (error) {
    debugln("Failed to read cache file");
    return;
  }

  String fld = String(field);
  String ts = String(update -> timestamp);

  if (isnan(update -> value)) doc[fld]["value"] = update->value;

  if (ts.length() > 0) doc[fld]["timestamp"] = ts;

  File file = fs.open(CACHE_FILE, FILE_WRITE);
  if(!file){
    debugln("Failed to open cache file for writing");
    return;
  }
  if( serializeJson(doc, file) == 0) debugln("Failed to update cache file");
  else debugln("Cache file updated");
  file.close();
}

/**
 * Empty the "readings" array in the log file.
 * @param fs: The file system reference to use.
 */
void clearLog(fs::FS &fs) {
  const char* cache = readFile(fs, LOG_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (error) {
    debugln("Failed to read log file");
    return;
  }

  doc["readings"].clear();

  File file = fs.open(LOG_FILE, FILE_WRITE);
  if(!file){
    debugln("Failed to open log file for writing");
    return;
  }

  if( serializeJson(doc, file) == 0) debugln("Failed to clear log file");
  else debugln("Log file cleared");
  file.close();
}

/**
 * Replace a substring with another substring in a char array.
 * https://forum.arduino.cc/t/replace-and-remove-char-arrays/485806/5 
 */
void str_replace(char *src, char *oldchars, char *newchars) { // utility string function
  char *p = strstr(src, oldchars);
  char buf[MAX_STRING_LENGTH];
  do {
    if (p) {
      memset(buf, '\0', strlen(buf));
      if (src == p) {
        strcpy(buf, newchars);
        strcat(buf, p + strlen(oldchars));
      } else {
        strncpy(buf, src, strlen(src) - strlen(p));
        strcat(buf, newchars);
        strcat(buf, p + strlen(oldchars));
      }
      memset(src, '\0', strlen(src));
      strcpy(src, buf);
    }
  } while (p && (p = strstr(src, oldchars)));
}

/**
 * Write a jpg file to the file system.
 * @param fs: The file system reference to use.
 * @param timestamp: The timestamp to use for the file name.
 * @param fb: The camera frame buffer to write to the file.
 */
void writejpg(fs::FS &fs, tm* timestamp, camera_fb_t* fb) {
  char* path = formattime(timestamp);
  File file = fs.open(path, FILE_WRITE);
  delete[] path;
  if(!file){
    debugln("Failed to open file in writing mode");
    return;
  }
  if (file.write(fb -> buf, fb -> len) != fb -> len) debugln("Failed to write to file");
  else debugln("File written successfully");
}

/**
 * Delete a jpg file from the file system.
 * @param fs: The file system reference to use.
 * @param timestamp: The timestamp to use for the file name.
 * 
 * @return True if the file was deleted successfully, false otherwise.
 */
bool deletejpg(fs::FS &fs, tm* timestamp) {
  char path[35];
  strftime(path, sizeof(path), "/%Y_%m_%d_%H_%M_%S.jpg", timestamp);
  debugf("Deleting file: %s\n", path);
  return fs.remove(path);
}

/**
 * Read a jpg file from the file system.
 * @param fs: The file system reference to use.
 * @param timestamp: The timestamp to use for the file name.
 * @param fb: The camera frame buffer to read the file to.
 * 
 * @return True if the file was read successfully, false otherwise.
 */  
bool readjpg(fs::FS &fs, tm* timestamp, camera_fb_t* fb) {
  if (!fb) {
    return false;
  }

  char* filename = formattime(timestamp);
  File file = fs.open(filename, FILE_READ);
  delete[] filename;
  
  if (!file) {
    debugln("Failed to open file");
    return false;
  }
  
  size_t fileSize = file.size();
  
  // Clean up existing buffer if present
  if (fb->buf) delete[] fb->buf;
  
  uint8_t* buffer = new uint8_t[fileSize];
  if (!buffer) {
    file.close();
    return false;
  }
  
  size_t bytesRead = file.read(buffer, fileSize);
  file.close();
  
  if (bytesRead != fileSize) {
    delete[] buffer;
    return false;
  }
  
  fb->buf = buffer;
  fb->len = fileSize;
  fb->width = 2560;  // FRAMESIZE_QHD
  fb->height = 1440;
  fb->format = PIXFORMAT_JPEG;
  
  return true;
}
