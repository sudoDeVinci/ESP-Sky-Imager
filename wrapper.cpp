#include "wrapper.h"

/**
 * Try to get the current time.
 * 1. Check the cache for the last time we queried NTP server. - if it is over 6 hours, query the server.
 * 1.2. Query the NTP server - Update cache if succesfful.
 * 2. Get the current time for the system.
 */
void fetchCurrentTime(fs::FS &fs, tm *now) {
    const char* cache = readFile(fs, CACHE_FILE); 
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cache);
    if (error) {
        debugln("Failed to read cache file");
        return;
    }

    // Read the last synced time from the cache.
    const char* timestamp = doc["NTP"] | "";
    if (timestamp == "") {
        debugln("No timestamp found in cache");
        return;
    }

    // Parse the timestamp from the cache.
    struct tm cacheTime = {0};
    strptime(timestamp, "%Y-%m-%d %H:%M:%S", &cacheTime);

    // Get the current time.
    getTime(now, 5);

    // Check if the cache is older than 6 hours.
    // Timestamps are in the format of "%Y-%m-%d %H:%M:%S"
    double seconds = difftime(mktime(now), mktime(&cacheTime));
    if (seconds > 21600) {
        // Cache is older than 6 hours, set clock and get  right time.
        setClock(now);
        updateCache(fs, formattime(now), "NTP", "");
    }
}