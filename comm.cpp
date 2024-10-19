#include "comm.h"


/**
 * TIME RELATED FUNCTIONALITY
 */


/**
 * Set the internal clock via NTP server.
 */
void setClock() {
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
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  debug(F("Current time: "));
  debug(asctime(&timeinfo));
}

/**
  * Get the current time and format the timestamp as MySQL DATETIME.
  * timeinfo is an empty struct whihc is filled by calling getLocalTime().
  * Big thanks to Andreas Spiess:
  * https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32/blob/master/NTP_Example/NTP_Example.ino
  *
  *  If tm_year is not equal to 0xFF, it is assumed that valid time information has been received.
  */
char* getTime(tm *timeinfo, time_t *now, int timer) {
  uint32_t start = millis();
  debug("Getting time!");

  do {
    time(now);
    localtime_r(now, timeinfo);
    debug(".");
    delay(random(150, 550));
  } while (((millis() - start) <= (1000 * timer)) && (timeinfo -> tm_year <= 1970));
  debugln("Done");
  if (timeinfo -> tm_year == 1970) return "None";

  char timestamp[30];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(now));
  return timestamp;
}


/**
 * Check if the current time is between 5 PM and 6 AM.
 * If so, enter deep sleep mode until 6 AM.
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
 * Connect to wifi Network and apply SSL certificate.
 * Attempt to match the SSID of nearby netwokrs with an SSID in the networkInfo file.
 * If a match is found, connect to the network and apply the SSL certificate.
 */
bool wifiSetup(NetworkInfo* network, Sensors::Status *stat) {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  // Read the networkinfo file and get the lisgt of network ssids and passwords.
  const String nwinfo = readFile(SD_MMC, "/networkInfo.json");
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
 * Send a request to the server and return the response as a string.
 */
String getResponse(HTTPClient *HTTP, int httpCode) {
  if (httpCode > 0) return HTTP -> getString();
  else return HTTP -> errorToString(httpCode);
}

/**
 * Send byte buffer to server via HTTPClient.
 * Got gist of everything from klucsik at:
 * https://gist.github.com/klucsik/711a4f072d7194842840d725090fd0a7
 */
void send(HTTPClient* https, NetworkInfo* network, const String& timestamp, uint8_t* buf, size_t len) {
  https -> setConnectTimeout(READ_TIMEOUT);
  https -> addHeader(network -> headers.CONTENT_TYPE, network -> mimetypes.IMAGE_JPG);
  https -> addHeader(network -> headers.MAC_ADDRESS, WiFi.macAddress());
  https -> addHeader(network -> headers.TIMESTAMP, timestamp);

  const int httpCode = https -> POST(buf, len);

  debugln(getResponse(https, httpCode));  
}
