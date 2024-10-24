#include "wrapper.h"

/**
 * Try to get the current time.
 * 1. Check the cache for the last time we queried NTP server. - if it is over 6 hours, query the server.
 * 1.2. Query the NTP server - Update cache if succesfful.
 * 2. Get the current time for the system.
 */
void fetchCurrentTime(fs::FS &fs, tm *now, Sensors::Status *stat) {
    // Get the current time.
    getTime(now, 5);

    // Read the last synced time from the cache.
    const char* cache = readFile(fs, CACHE_FILE); 
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cache);
    if (error) {
        debugln("Failed to read cache file");
        return;
    } else {
        debugln("Cache read successfully");
    }

    const char* timestamp = doc["NTP"];
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
          if (!stat -> WIFI) {
              debugln("No wifi connection, cannot update time.");
              return;
          }
          setClock(now);
          updateCache(fs, formattime(now), "NTP", "");
      }
    } else {
        // Cache is empty, set clock and get right time.
        debugln("Cache is empty, updating time...");
        if (!stat -> WIFI) {
                debugln("No wifi connection, cannot update time.");
                return;
        }
        setClock(now);
        updateCache(fs, formattime(now), "NTP", "");
    }
}