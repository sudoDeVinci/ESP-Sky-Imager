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
  if(fs.exists(LOG_FILE)) {
    debugln("Log file already exists");
    return;
  }

  File file = fs.open(LOG_FILE, FILE_WRITE, true);
  if(!file){
    debugln("Failed to open log file for writing");
    return;
  }

  if(file.print('{"readings":[]}')) debugln("Log file Initialised"); 
  file.close();
}

/**
 * Initialize the cache file. 
 */
void initCacheFile (fs::FS &fs) {
  if(fs.exists(CACHE_FILE)) {
    debugln("Cache file already exists");
    return;
  }

  File file = fs.open(CACHE_FILE, FILE_WRITE, true);
  if(!file){
    debugln("Failed to open cache file for writing");
    return;
  }
  if(file.print('{"NTP":"","SERVER":"","QNH":{"value":0, "timestamp":""}}')) debugln("Cache file Initialised"); 
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
void updateCache (fs::FS &fs, const char* timestamp, const char* field, const char* subfield = nullptr) {
  const char* cache = readFile(fs, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (error) {
    debugln("Failed to read cache file");
    return;
  }

  if (subfield == nullptr) doc[field] = timestamp;
  else doc[field][subfield] = timestamp;

  File file = fs.open(CACHE_FILE, FILE_WRITE);
  if(!file){
    debugln("Failed to open cache file for writing");
    return;
  }
}
