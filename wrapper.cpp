#include "wrapper.h"

/**
 * Try to get the current time.
 * 1. Check the cache for the last time we queried NTP server. - if it is over 6 hours, query the server.
 * 1.2. Query the NTP server - Update cache if succesfful.
 * 2. Get the current time for the system.
 * 
 * @param  fs: The file system reference to use for the cache.
 * @param now: The time struct to fill with the current time.
 * @param stat: The status struct to check if we have wifi connection.
 */
void fetchCurrentTime(fs::FS &fs, tm *now, Sensors::Status *stat) {
  // Get the current time.
  getTime(now, 5);

  // Read the last synced time from the cache.
  const char *cache = readFile(fs, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (error) {
    debugln("Failed to read cache file");
    return;
  } else debugln("Cache read successfully");

  const char *timestamp = doc["NTP"];
  debug("Cached time is: ");
  debugln(timestamp);

  if (strcmp(timestamp, "None") == 0) {
    // Parse the timestamp from the cache.
    struct tm cacheTime = {0};
    strptime(timestamp, "%Y-%m-%d %H:%M:%S", &cacheTime);

    // Check if the cache is older than 6 hours.
    // Timestamps are in the format of "%Y-%m-%d %H:%M:%S"
    double seconds = difftime(mktime(now), mktime(&cacheTime));
    if (seconds > 21600) {
      // Cache is older than 6 hours, set clock and get  right time.
      debugln("Cache is older than 6 hours, updating time...");
      if (!stat->WIFI) {
        debugln("No wifi connection, cannot update time.");
        return;
      }
      setClock(now);
      updateCache(fs, formattime(now), "NTP");
    }
  } else {
    // Cache is empty, set clock and get right time.
    debugln("Cache is empty, updating time...");
    if (!stat->WIFI) {
      debugln("No wifi connection, cannot update time.");
      return;
    }
    setClock(now);
    updateCache(fs, formattime(now), "NTP");
  }
}


/**
 * Get the QNH from the api if there is internet.
 * 1. Check if we have internet.
 * 2. If connection, get the QNH from the api., then update the cache.
 * 3. If no connection, return the cached value.
 * 
 * @param fs: The file system reference to use for the cache.
 * @param timestamp: The timestamp to update the cache with.
 * @param network: The network struct to check if we have wifi connection.
 */
double fetchQNH(fs::FS &fs, const char* timestamp, NetworkInfo *network) {
  double qnh = UNDEFINED;

  // Check if we have internet.
  if (WiFi.status() == WL_CONNECTED) {
    // Get the QNH from the server.
    double qnh = getQNH(network);
    if (qnh != UNDEFINED) {
      // Update the cache with the new QNH.
      updateCache(fs, qnh, "QNH", "value");
      updateCache(fs, timestamp, "QNH", "timestamp");
    }
  }

  // Read the QNH from the cache.
  const char *cache = readFile(SD_MMC, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (error || strcmp(doc["QNH"]["timestamp"], "None") == 0) {
    debugln("Failed to read QNH from cache");
    return qnh;
  } else debugln("Cached QNH read as : ");
  
  qnh = doc["QNH"]["value"];
  debugln(qnh);
  return qnh; 
}

