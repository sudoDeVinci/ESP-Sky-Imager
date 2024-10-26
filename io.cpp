#include "io.h"

/**
 * Initialize the sdcard file system. 
 */
void sdmmcInit(void){
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    debugln("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
      debugln("No SD_MMC card attached");
      return;
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  debugf("SD_MMC Card Size: %lluMB\n", cardSize);  
  debugf("Total space: %lluMB\r\n", SD_MMC.totalBytes() / (1024 * 1024));
  debugf("Used space: %lluMB\r\n", SD_MMC.usedBytes() / (1024 * 1024));
}


/**
 * Sleep for a specified number of minutes. 
 */
void deepSleepMins(double mins) {
  esp_sleep_enable_timer_wakeup(mins*60000000);
  esp_deep_sleep_start();
}

/**
 * Initialize the log file. 
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
 * Read the conf file and return a String.
 */
const char* readFile (fs::FS &fs, const char * path) {
  debugf("\nReading file: %s\r\n", path);

  String output = "";

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    debugf("- failed to open %s for reading\r\n", path);
    return output.c_str();
  }

  while(file.available()){
    char ch = file.read();
    output.concat(ch);
  }

  debugln();
  file.close();
  return output.c_str();
}

/**
 * Update the timstamp cache file with a new timestamp.
 */
void updateCache (fs::FS &fs, const char* timestamp, const char* field) {
  const char* cache = readFile(fs, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
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
 * Update a numaerical cache field / subfield.
 */
void updateCache (fs::FS &fs, double value, const char* field) {
  const char* cache = readFile(fs, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (error) {
    debugln("Failed to read cache file");
    return;
  }

  doc[field] = value;

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
 * Update a numaerical cache field / subfield.
 */
void updateCache (fs::FS &fs, cacheUpdate* update, const char* field) {
  const char* cache = readFile(fs, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (error) {
    debugln("Failed to read cache file");
    return;
  }

  doc[field]["value"] = update->value;
  doc[field]["timestamp"] = update->timestamp;

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
 */
void writejpg(fs::FS &fs, tm* timestamp, camera_fb_t* fb) {
  char path[35];
  strftime(path, sizeof(path), "/%Y_%m_%d_%H_%M_%S.jpg", timestamp);
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    debugln("Failed to open file in writing mode");
    return;
  }
  if (file.write(fb -> buf, fb -> len) != fb -> len) debugln("Failed to write to file");
  else debugln("File written successfully");
}

/**
 * Read a jpg file from the file system.
 */
bool readjpg(fs::FS &fs, tm* timestamp, camera_fb_t* fb) {
    if (!fb) {
        return false;
    }

    char filename[35];
    strftime(filename, sizeof(filename), "/%Y_%m_%d_%H_%M_%S.jpg", timestamp);
    
    File file = fs.open(filename, FILE_READ);
    if (!file) {
        Serial.println("Failed to open file");
        return false;
    }
    
    size_t fileSize = file.size();
    
    // Clean up existing buffer if present
    if (fb->buf) {
        delete[] fb->buf;
    }
    
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