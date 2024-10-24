#include "comm.h"


/**
 * TIME RELATED FUNCTIONALITY
 */


/**
 * Set the internal clock via NTP server.
 */
void setClock(tm *timeinfo) {
  configTime(0, 0, "pool.ntp.org");

  debug(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    debug(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  debugln();
  gmtime_r(&nowSecs, timeinfo);
  debug(F("Current time: "));
  debug(asctime(timeinfo));
}

/**
  * Get the current time and format the timestamp as MySQL DATETIME.
  * timeinfo is an empty struct whihc is filled by calling getLocalTime().
  * Big thanks to Andreas Spiess:
  * https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32/blob/master/NTP_Example/NTP_Example.ino
  *
  *  If tm_year is not equal to 0xFF, it is assumed that valid time information has been received.
  * 
  * @param timeinfo*: tm struct within global Network struct to store the time information.
  * @param timer: time in seconds to wait for the time to be received.
  * 
  * @return char*: timestamp in MySQL DATETIME format.
  */
void getTime(tm *timeinfo, int timer) {
  uint32_t start = millis();
  time_t now;
  debug("Getting time!");

  do {
    time(&now);
    localtime_r(&now, timeinfo);
    debug(".");
    delay(random(150, 550));
  } while (((millis() - start) <= (1000 * timer)) && (timeinfo -> tm_year <= 1970));
  debugln("Done");
}

/**
 * Format the timestamp as MySQL DATETIME.
 * If the year is 1970, return "None".
 * 
 * @param timeinfo: tm struct within global Network struct to store the time information.
 * @return char*: timestamp in MySQL DATETIME format.
 */
char* formattime(tm* now) {
  if (now -> tm_year == 1970) return "None";
  char timestamp[30];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", now);
  return timestamp;
}

/**
 * Check if the current time is between 5 PM and 6 AM.
 * If so, enter deep sleep mode until 6 AM.
 * 
 * @param timeinfo*: tm struct within global Network struct to store the time information.
 */
void checkAndSleep(tm *timeinfo) {
    int currentHour = timeinfo->tm_hour;
    int currentMinute = timeinfo->tm_min;
    int currentSecond = timeinfo->tm_sec;

    int wakeHour = 6;  // 6 AM
    int sleepStartHour = 21;  // 5 PM

    // Check if the current time is between 5 PM and 6 AM
    if (currentHour >= sleepStartHour || currentHour < wakeHour) {
      int sleepDuration = 0;  // in seconds

      if (currentHour >= sleepStartHour) {
          // Current time is after 5 PM, calculate time until midnight
          sleepDuration = (24 - currentHour + wakeHour) * 3600 - currentMinute * 60 - currentSecond;
      } else {
          // Current time is before 6 AM, calculate remaining time until 6 AM
          sleepDuration = (wakeHour - currentHour) * 3600 - currentMinute * 60 - currentSecond;
      }

      // Log sleep duration
      debugln("Outside normal operating times.");
      debug("Sleeping for ");
      debug(sleepDuration / 3600);
      debug(" hours, ");
      debug((sleepDuration % 3600) / 60);
      debug(" minutes, and ");
      debug(sleepDuration % 60);
      debugln(" seconds.");

      // Enter deep sleep mode for the calculated duration
      esp_sleep_enable_timer_wakeup(sleepDuration * 1000000LL);  // Convert to microseconds
      esp_deep_sleep_start();
    }
}

/**
 * WIFI RELATED FUNCTIONALITY
 */

/**
 * Connect to a WiFi network.
 * If the connection attempt fails, return false.
 */
bool connect(const char* ssid, const char* pass, NetworkInfo* network, Sensors::Status *stat) {
  debugln("Connecting to WiFi Network ");
  WiFi.begin(ssid, pass);                                   
            
  uint8_t connect_count = 0; 

  // Wait for the WiFi connection to be established with a timeout of 10 attempts.
  while (WiFi.status() != WL_CONNECTED && connect_count < 10) {
    delay(random(250, 550));
    debug(".");
    connect_count++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    // Connection attempt failed after 10 tries, print an error message and return 1
    debugln("Could not connect to Wifi.");
    stat -> WIFI = false;
    return false;
  }

  stat -> WIFI = true;
  debugln("Connected!: -> " + WiFi.macAddress());
  network -> SSID = ssid;
  network -> PASS = pass;
  return true;
}

/**
 * Connect to wifi Network and apply SSL certificate.
 * Attempt to match the SSID of nearby netwokrs with an SSID in the networkInfo file.
 * If a match is found, connect to the network and apply the SSL certificate.
 */
bool wifiSetup(NetworkInfo* network, Sensors::Status *stat) {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  // Read the networkinfo file and get the lisgt of network ssids and passwords.
  const char* nwinfo = readFile(SD_MMC, NETWORK_FILE);
  JsonDocument jsoninfo;
  deserializeJson(jsoninfo, nwinfo);
  const JsonArray networks = jsoninfo["networks"];

  // Scan surrounding networks.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int n = WiFi.scanNetworks();
  debugln("Scan done");

  for (int i = 0; i < n; i++) {
    const char* ssid = WiFi.SSID(i).c_str();
    const char* networkSSID;
    const char* networkPassword;
    for (JsonVariant networkJson: networks) {
      networkSSID = networkJson["SSID"];
      networkPassword = networkJson["PASS"];
      if (ssid == networkSSID) {
        debug("Connecting to WiFi Network ");
        debugln(ssid);
        if (connect(networkSSID, networkPassword, network, stat)) {
          return true;
        }
      }
    }
  }
    return false;
}

/**
 * Send a request to the server and return the response as a string.
 */
const char* getResponse(HTTPClient *HTTP, int httpCode) {
  if (httpCode > 0) return HTTP -> getString().c_str();
  else return HTTP -> errorToString(httpCode).c_str();
}

/**
 * Send byte buffer to server via HTTPClient.
 * Got gist of everything from klucsik at:
 * https://gist.github.com/klucsik/711a4f072d7194842840d725090fd0a7
 */
const char* send(HTTPClient* https, NetworkInfo* network, const char* timestamp, uint8_t* buf, size_t len) {
  https -> setConnectTimeout(READ_TIMEOUT);
  https -> addHeader(network -> headers.CONTENT_TYPE, network -> mimetypes.IMAGE_JPG);
  https -> addHeader(network -> headers.MAC_ADDRESS, WiFi.macAddress());
  https -> addHeader(network -> headers.TIMESTAMP, timestamp);

  const int httpCode = https -> POST(buf, len);

  return getResponse(https, httpCode);  
}

/**
 * Send message to server via HTTPClient.
 * Got gist of everything from klucsik at:
 * https://gist.github.com/klucsik/711a4f072d7194842840d725090fd0a7
 */
const char* send(HTTPClient* https, NetworkInfo* network, const char* timestamp) {
    https -> setConnectTimeout(READ_TIMEOUT);
    https -> addHeader(network -> headers.CONTENT_TYPE, network -> mimetypes.APP_FORM);
    https -> addHeader(network -> headers.MAC_ADDRESS, WiFi.macAddress());
    https -> addHeader(network -> headers.TIMESTAMP, timestamp);

    const int httpCode = https -> GET();

    return getResponse(https, httpCode);
}

/**
 * Check if the website is reachable before trying to communicate further.
 */
bool websiteReachable(HTTPClient* https, NetworkInfo* network, const char* timestamp) {
  size_t length = strlen(network->HOST) + strlen(network->routes.INDEX) + 1;
  char url[length];
  strcpy(url, network->HOST);
  strcat(url, network->routes.INDEX);

  https -> begin(url, network -> CERT);

  const int httpCode = https -> GET();

  // Check if the response code is 200 (OK)
  if (httpCode == 200) {
    https -> end();
    debugln("Website reachable");
    return true;
  } else {
    debug("Website unreachable: ");
    debug(https -> errorToString(httpCode));
    https -> end();
    return false;
  }
}

/**
 * Send statuses of sensors to HOST on specified PORT. 
 */
void sendStats(HTTPClient* https, NetworkInfo* network, Sensors::Status *stat, const char* timestamp) {
    debugln("\n[STATUS]");

    const char* const sht = stat -> SHT ? "true" : "false";
    const char* const bmp = stat -> BMP ? "true" : "false";
    const char* const cam = stat -> CAM ? "true" : "false";

    size_t length = strlen("sht=") + strlen(sht) + 
                    strlen("&bmp=") + strlen(bmp) +
                    strlen("&cam=") + strlen(cam) + 1;
    
    char values[length];
    strcpy(values, "sht=");
    strcat(values, sht);
    strcat(values, "&bmp=");
    strcat(values, bmp);
    strcat(values, "&cam=");
    strcat(values, cam);

    size_t len = strlen(network -> HOST) + strlen(network -> routes.STATUS) + length + 2;
    char url[len];
    strcpy(url, network -> HOST);
    strcat(url, network -> routes.STATUS);
    strcat(url, "?");
    strcat(url, values);

    https -> begin(url, network -> CERT);

    debugln(url);

    const char* reply = send(https, network, timestamp);
    debugln(reply);
    https -> end();
}

/**
 * Send readings from weather sensors to HOST on specified PORT. 
 */
void sendReadings(HTTPClient* https, NetworkInfo* network, Reading* readings) {
  debugln("\n[READING]");

  // Temporary buffers to store the formatted values
  char tempBuffer[20];
  char humBuffer[20];
  char presBuffer[20];
  char dewBuffer[20];

  // Format each double value into the temporary buffers with snprintf
  snprintf(tempBuffer, sizeof(tempBuffer), "%.5f", readings->temperature);
  snprintf(humBuffer, sizeof(humBuffer), "%.5f", readings->humidity);
  snprintf(presBuffer, sizeof(presBuffer), "%.5f", readings->pressure);
  snprintf(dewBuffer, sizeof(dewBuffer), "%.5f", readings->dewpoint);

  // Calculate the total length by summing up the lengths of all strings
  size_t length = strlen("temperature=") + strlen(tempBuffer) + 
                  strlen("&humidity=") + strlen(humBuffer) +
                  strlen("&pressure=") + strlen(presBuffer) +
                  strlen("&dewpoint=") + strlen(dewBuffer) + 1;
  
  char values[length];
  strcpy(values, "temperature=");
  strcat(values, tempBuffer);
  strcat(values, "&humidity=");
  strcat(values, humBuffer);
  strcat(values, "&pressure=");
  strcat(values, presBuffer);
  strcat(values, "&dewpoint=");
  strcat(values, dewBuffer);

  size_t len = strlen(network -> HOST) + strlen(network -> routes.READING) + length + 2;
  char url[len];
  strcpy(url, network -> HOST);
  strcat(url, network -> routes.READING);
  strcat(url, "?");
  strcat(url, values);

  https -> begin(url, network->CERT);

  debugln(url);
  
  const char* reply = send(https, network, readings -> timestamp);
  debugln(reply);

  https -> end();
}

/**
 * Parse the QNH from the server response.
 */
double parseQNH(const char* json) {
  JsonDocument doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, json);

  // Test if parsing succeeds.
  if (error) {
    debug("Failed to parse QNH json response error :-> ");
    debugln(error.f_str());
    return UNDEFINED;
  }

  const double qnh = doc["metar"]["qnh"];
  return qnh;
}

/**
 * Get the Sea Level Pressure from the server.
 */
double getQNH(NetworkInfo* network) {
  debugln("\n[GETTING SEA LEVEL PRESSURE]");

  HTTPClient https;
  const char* const HOST = "https://api.metar-taf.com";
  const char* const ROUTE = "/metar";
  const char* const version = "2.3";
  const char* const locale = "en-US";
  const char* const airport = "ESMX";

  const char* metarinfo = readFile(SD_MMC, NETWORK_FILE);
  JsonDocument jsoninfo;
  DeserializationError error = deserializeJson(jsoninfo, metarinfo);
  if (error) {
    debug("Failed to read metarinfo file error :-> ");
    debugln(error.f_str());
    return UNDEFINED;
  }
  const char* const key = jsoninfo["metar_api_key"];

  size_t length = strlen("api_key=") + strlen(key) + 
                  strlen("&v=") + strlen(version) +
                  strlen("&locale=") + strlen(locale) +
                  strlen("&id=") + strlen(airport) + 
                  strlen("&station_id=") + strlen(airport) +
                  strlen("&test=0") + 1;

  char values[length];
  strcpy(values, "api_key=");
  strcat(values, key);
  strcat(values, "&v=");
  strcat(values, version);
  strcat(values, "&locale=");
  strcat(values, locale);
  strcat(values, "&id=");
  strcat(values, airport);
  strcat(values, "&station_id=");
  strcat(values, airport);
  strcat(values, "&test=0");

  size_t len = strlen(HOST) + strlen(ROUTE) + length + 2;
  char url[len];
  strcpy(url, HOST);
  strcat(url, ROUTE);
  strcat(url, "?");
  strcat(url, values);

  https.begin(url);

  const int httpCode = https.GET();
  const char* reply = getResponse(&https, httpCode);
  return parseQNH(reply);
}

/**
 * Send image from weather station to server. 
 */
void sendImage(HTTPClient* https, NetworkInfo* network, uint8_t* buf, size_t len, const char* timestamp) {
  debugln("\n[IMAGE]");
  size_t length = (strlen(network -> HOST) + strlen(network -> routes.IMAGE) + 2);
  char url[length];
  strcpy(url, network -> HOST);
  strcat(url, network -> routes.IMAGE);
  
  https -> begin(url, network -> CERT);

  debugln(url);

  send(https, network, timestamp, buf, len);

  https -> end();
}

/**
 * Update the board firmware via the update server.
 */
void OTAUpdate(NetworkInfo* network, const String& firmware_version) {
  debugln("\n[UPDATES]");

  size_t length = strlen(network -> HOST) + strlen(network -> routes.UPDATE) + 1;
  char url[length];
  strcpy(url, network -> HOST);
  strcat(url, network -> routes.UPDATE);
  WiFiClient* client = network -> CLIENT;

  // Start the OTA update process
  debug("Grabbing updates from: ");
  debugln(url);

  // Connect to the update server
  t_httpUpdate_return ret = httpUpdate.update(*client, url, firmware_version);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      debugf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      debugln("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      debugln("HTTP_UPDATE_OK");
      break;
  }
}
