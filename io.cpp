#include "io.h"

/**
 * Initialize the sdcard file system. 
//  */
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
 * Read the conf file and return a String.
 */
const char* readFile(fs::FS &fs, const char * path) {
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