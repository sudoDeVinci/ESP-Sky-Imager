#include "wrapper.h"



/**
 * Get the QNH from the api if there is internet.
 * 1. read the cache for the last time we queried the api.
 * 2. If the cache is older than 2 hours, query the api.
 * 3. If connection, get the QNH from the api, then update the cache.
 * 4. If no connection, return the cached value.
 * 
 * @param fs: The file system reference to use for the cache.
 * @param timestamp: The timestamp to update the cache with.
 * @param network: The network struct to check if we have wifi connection.
 */
double fetchQNH(fs::FS &fs, tm* now, NetworkInfo *network) {
  double qnh = UNDEFINED;

  // Read the last synced time from the cache.
  const char *cache = readFile(fs, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (!error) {
    debugln("Failed to read cache file");

    const char* qnhTS = doc["QNH"]["timestamp"];
    qnh = doc["QNH"]["value"];
    debug("Cached QNH timestamp is: ");
    debugln(qnhTS);

    // If timestamp is not "None", try to parse and check age
    if (strcmp(qnhTS, "None") != 0) {
      struct tm cacheTime = {0};
      if (strptime(qnhTS, "%Y-%m-%d %H:%M:%S", &cacheTime)) {
        constexpr int CACHE_TIMEOUT_SECONDS = 7200;  // 2 hours
        double timeDiff = difftime(mktime(now), mktime(&cacheTime));
        // cache is still valid - return
        if (timeDiff <= CACHE_TIMEOUT_SECONDS) return qnh;
        debugln("Cache is older than 2 hours, updating QNH...");
      }
    // If timestamp is "None", update the cache.
    } else debugln("Cache is empty, updating time...");
  }
  // Update time if we have WiFi
  if (WiFi.status() != WL_CONNECTED) {
    debugln("No wifi connection, cannot update time.");
    return qnh;
  }

  //Get the latest QNH from the API
  qnh = getQNH(network);

  // Update the cache with the new time.
  cacheUpdate update = {qnh, formattime(now)};
  updateCache(fs, &update, "QNH");

  return qnh;
}


/**
 * Try to get the current time from the NTP server.
 * 1. Read the current system time.
 * 2. Check the cache for the last time we queried NTP server. - if it is over 6 hours, query the server.
 * 3. Query the NTP server - Update cache if succesfful.
 * 
 * @param  fs: The file system reference to use for the cache.
 * @param now: The time struct to fill with the current time.
 * @param stat: The status struct to check if we have wifi connection.
 */
void fetchCurrentTime(fs::FS &fs, tm *now, Sensors::Status *stat) {
  if (!now || !stat) {
    debugln("Invalid parameters");
    return;
  }

  // Get the current time according to the system.
  getTime(now, 10);

  // Read the cache file.
  const char* cache = readFile(fs, CACHE_FILE);
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cache);
  if (error) {
    debugln("Failed to parse cache JSON");
    return;
  }
  debugln("Cache read successfully");

  // Specifically read the last time we queried the NTP server.
  const char* timestamp = doc["NTP"];
  debug("Cached time is: ");
  debugln(timestamp);

  // If timestamp is not "None", try to parse and check age
  if (strcmp(timestamp, "None") != 0) {
    struct tm cacheTime = {0};
    if (strptime(timestamp, "%Y-%m-%d %H:%M:%S", &cacheTime)) {
      constexpr int CACHE_TIMEOUT_SECONDS = 21600;  // 6 hours
      double timeDiff = difftime(mktime(now), mktime(&cacheTime));
      // cache is still valid - return
      if (timeDiff <= CACHE_TIMEOUT_SECONDS) return;
      debugln("Cache is older than 6 hours, updating time...");
    }
  // If timestamp is "None", update the cache.
  } else debugln("Cache is empty, updating time...");

  // Update time if we have WiFi
  if (!stat->WIFI) {
    debugln("No wifi connection, cannot update time.");
    return;
  }

  // Get the current time from the NTP server.
  setClock(now);
  // Update the cache with the new time.
  updateCache(fs, formattime(now), "NTP");
}



void sendReadings(fs::FS &fs, tm* now, Sensors* sensors, NetworkInfo* network) {
  if (!sensors) {
    debugln("Invalid parameters");
    return;
  }

  // Get the QNH
  double qnh = fetchQNH(fs, &now, network);

  // Get the sensor readings
  Reading reading = readAll(&sensors.status, &sensors -> SHT, &sensors -> BMP);
  reading.timestamp = formattime(now);

  // Send the readings to the server
  if (!sensors -> status.WIFI) {
    debugln("NO CONNECTION!  ->  Going to sleep!...");
    return;
  }

  
  {
    // Attempting to scope the http client to keep it alive in relation to the wifi client.
    HTTPClient http;

    // Check if the site is reachable.
    if (!websiteReachable(&https, network, reading.timestamp)) {
      debugln("Website is not reachable, saving to log file");
      appendReading(fs, &reading);

      // Take image and save to sd card
      if (sensors -> status.CAM) {
        camera_fb_t * fb = nullptr;
        debugln("Taking image...");
        camera_fb_t * fb = nullptr;
          for(int i = 0; i < 3; i++) {
            fb = esp_camera_fb_get();
            esp_camera_fb_return(fb);
            delay(100);
          }
        fb = esp_camera_fb_get();

        char* filepath = formattime(now);
        str_replace(filepath, " ", "-");
        str_replace(filepath, ":", "-");
        String path = "/" + String(filepath) + ".jpg";
        writejpg(fs, path.c_str(), fb -> buf, fb -> len);
        esp_camera_fb_return(fb);
        esp_err_t deinitErr = sensors -> cameraTeardown();
        debugln("WEBSITE UNREACHABLE!  ->  Going to sleep!...");
        delay(50);
        deepSleepMins(SLEEP_MINS);
      }
    }

    // send statuses to the server.
    void sendStats(&http,network, &sensors -> status, reading.timestamp);
    delay(20);

    // send readings to the server.
    void sendReadings(&http, network, &reading);
    delay(20);

    // send image to the server.
    camera_fb_t * fb = nullptr;
    debugln("Taking image...");
    camera_fb_t * fb = nullptr;
      for(int i = 0; i < 3; i++) {
        fb = esp_camera_fb_get();
        esp_camera_fb_return(fb);
        delay(100);
      }
    fb = esp_camera_fb_get();
    sendImage(&http, network, fb -> buf, fb -> len, reading.timestamp);
    delay(20);
    esp_camera_fb_return(fb);
    esp_err_t deinitErr = sensors -> cameraTeardown();
  }
}  