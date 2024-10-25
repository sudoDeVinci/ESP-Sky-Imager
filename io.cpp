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

void writejpg(fs::FS &fs, const char* path, const uint8_t* data, size_t size) {
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    debugln("Failed to open file in writing mode");
    return;
  }
  if (file.write(data, size) != size) debugln("Failed to write to file");
  else debugln("File written successfully");