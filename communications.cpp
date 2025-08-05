#include "communications.hpp"


/**
 * Connect to the WiFi network.
 * This function attempts to connect to the specified WiFi network using the provided SSID and password.
 * It will retry the connection a specified number of times before giving up.
 * 
 * @param ssid The SSID of the WiFi network to connect to.
 * @param password The password for the WiFi network.
 * @param status The status object to update with the connection status.
 * @param retryCount The number of times to retry the connection if it fails.
 * @return true if the connection was successful, false otherwise.
 */
[[nodiscard]]
bool WifiSpec::connect(
    const std::string& ssid,
    const std::string& password,
    SensorContainer::Status& status,
    uint8_t retryCount
) {
    debugf("Connecting to WiFi network: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    uint8_t connectCount = 0;
    while (WiFi.status() != WL_CONNECTED && connectCount < retryCount) {
        vTaskDelay(250 / portTICK_PERIOD_MS);
        debug(".");
        connectCount++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        debugln("Failed to connect to WiFi.");
        status.WIFI = false;
        return false;
    }

    status.WIFI = true;
    debugf("Connected to WiFi. \nMAC: %s, \nIP: %s\n",
           WiFi.macAddress().c_str(),
           WiFi.localIP().toString().c_str()
    );

    this->ssid = ssid;
    this->password = password;
    this->isConnected = true;

    return true;
}


/**
 * @brief Read from the known networks json file and connect to the first matching network.
 * @param fs The filesystem to read the network information from.
 * @param status The status object to update with the connection status.
 * @return true if a network was found and connected to, false otherwise.
 */
bool WifiSpec::wifiConnectionSetup(fs::FS& fs, SensorContainer::Status& status) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.disconnect();

    const std::string nwinfo = readFile(fs, NETWORKS_FILE);
    if (nwinfo.empty()) {
        debugln("No network information found. Please set up the WiFi network.");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, nwinfo);
    if (error) {
        debugf("Failed to parse network file: %s\n", error.c_str());
        return false;
    }

    const JsonArray networks = doc["networks"];
    int netcount = WiFi.scanNetworks();

    for (int i = 0; i < netcount; i++) {
        std::string ssid = std::string(WiFi.SSID(i).c_str());
        for (JsonVariant netJson : networks) {
            std::string netssid = netJson["ssid"];
            std::string netpwd = netJson["pass"];
            if (ssid == netssid) {
                if (connect(netssid, netpwd, status, RETRY_COUNT)) {
                    return true;
                }
            }
        }
    }

    return false;
}



/**
 * Get the current time from the onboard clock.
 * This function will keep trying to get the time until it succeeds or the timer runs out.
 * It will print a dot every 150-250 milliseconds to indicate progress.
 * If the time is not set after the timer runs out, it will print an error message.
 * @param timeinfo: tm struct to hold the time information.
 * @param timer: The maximum time to wait for the time to be set, in seconds.
 * @return true if the time was successfully set, false otherwise.
 */
bool WifiSpec::getInternalTime(int timer) {
    uint32_t start = millis();
    time_t now;
    debug("Getting time!");
  
    do {
      time(&now);
      localtime_r(&now, &timeinfo);
      debug(".");
      vTaskDelay(random(150, 250)/portTICK_PERIOD_MS);
    } while (((millis() - start) <= (1000 * timer)) && (timeinfo.tm_year < 71));  // 71 = year 1971
    debugln("Done");
    
    if (timeinfo.tm_year < 71) {
        debugln("Failed to get time from NTP server.");
        return false;
    }

    debugf("Current time: %s\n", asctime(&timeinfo));
    return true;
}


/**
 * Set the internal clock of the ESP32 to the current time using NTP AND fill the timeinfo struct with that time.
 * Big thanks to Andreas Spiess.
 * @param timeinfo: tm struct to hold the time information.
 */
void WifiSpec::setClock(void) {
    configTime(0, 0, "pool.ntp.org");
    debug(F("Waiting for NTP time sync: "));
    time_t nowSecs = time(nullptr);
    while (nowSecs < 1577836800) {  // January 1, 2020 - reasonable minimum
        vTaskDelay(500/portTICK_PERIOD_MS);
        debug(F("."));
        nowSecs = time(nullptr);
    }
  
    debugln();
    gmtime_r(&nowSecs, &timeinfo);
    debug(F("Current time: "));
    debug(asctime(&timeinfo));
}

/**
 * Some a JSON string to the server as a POST request.
 * @param url The URL to send the JSON data to.
 * @param webclient The HTTP client to use for the request.
 * @param jsonData The JSON data to send.
 * @param timestamp The timestamp to include in the request headers.
 * @return The response from the server as a string.
 */
std::string WifiSpec::sendJson(
    const std::string& url,
    const std::string& jsonData
) {

    getInternalTime(5);

    HTTPClient webclient;
    webclient.begin(url.c_str());

    char contentlength[10];
    snprintf(contentlength, sizeof(contentlength), "%zu", jsonData.length());

    webclient.setConnectTimeout(CONN_TIMEOUT);
    webclient.addHeader(ServerInfo::Header::CONTENT_TYPE, ServerInfo::MIMEType::APP_JSON);
    webclient.addHeader(ServerInfo::Header::CONTENT_LENGTH, contentlength);
    webclient.addHeader(ServerInfo::Header::MACADDRESS, WiFi.macAddress().c_str());
    webclient.addHeader(ServerInfo::Header::TIMESTAMP, formatTime(timeinfo).c_str());
    webclient.addHeader(ServerInfo::Header::FIRMWARE_VERSION, VERSION);

    int httpCode = webclient.POST(jsonData.c_str());
    if (httpCode < 0) {
        char errorMsg[100];
        snprintf(errorMsg, sizeof(errorMsg), "{\"error\: %d, \"message\": %s}\n",
                 httpCode,
                 webclient.errorToString(httpCode).c_str());
        webclient.end();
        return std::string(errorMsg);
    }

    std::string response = webclient.getString().c_str();
    
    webclient.end();
    return response;
}


std::string WifiSpec::getJson(const std::string& url) {

    getInternalTime(5);

    HTTPClient webclient;
    webclient.begin(url.c_str());
    webclient.setConnectTimeout(CONN_TIMEOUT);
    webclient.addHeader(ServerInfo::Header::CONTENT_TYPE, ServerInfo::MIMEType::APP_JSON);
    webclient.addHeader(ServerInfo::Header::MACADDRESS, WiFi.macAddress().c_str());
    webclient.addHeader(ServerInfo::Header::TIMESTAMP, formatTime(timeinfo).c_str());
    webclient.addHeader(ServerInfo::Header::FIRMWARE_VERSION, VERSION);

    int httpCode = webclient.GET();
    if (httpCode < 0) {
        char errorMsg[200];
        snprintf(errorMsg, sizeof(errorMsg), "{\"error\: %d, \"message\": %s}\n",
                 httpCode,
                 webclient.errorToString(httpCode).c_str());
        webclient.end();
        return std::string(errorMsg);
    }

    std::string response = webclient.getString().c_str();
    
    webclient.end();
    return response;
}


 /**
 * Check if the server is reachable.
 * @param webclient The HTTP client to use for the request.
 * @param netIntf The network interface settings to use for the request.
 * @return true if the server is reachable, false otherwise.
 */
[[nodiscard]]
bool ServerInfo::websiteReachable(void) const {

    size_t length = this->host.length() + strlen(Route::INDEX) + 10;
    char url[length];
    snprintf(url, length, "%s%s", this->host.c_str(), Route::INDEX);

    debugf("Checking if server is reachable at %s\n", url);

    HTTPClient webclient;
    webclient.setTimeout(CONN_TIMEOUT);
    webclient.begin(url);
    const int httpCode = webclient.GET();
    webclient.end();

    if (httpCode == 200) {
        debugln(">> Server is reachable.\n");
        return true;
    }
    
    debugf(">> Server is not reachable. HTTP code: %d\n\n", httpCode);
    return false;
}


/**
 * Send the status of the sensors to the server.
 * @param webclient The HTTP client to use for the request.
 * @param netIntf The network interface settings to use for the request.
 * @param status The status of the sensors to send.
 */
void ServerInfo::sendStatuses(
    WifiSpec& netIntf,
    const SensorContainer::Status& status
) const {

    size_t length = this->host.length() + strlen(Route::STATUS) + 10;
    char url[length];
    snprintf(url, length, "%s%s", this->host.c_str(), Route::STATUS);

    debugf("Sending Statuses to %s\n", url);
    std::string jsonData = status.toJson();
    std::string response = netIntf.sendJson(url, jsonData);
    debugf("Response: %s\n", response.c_str());
}

/**
 * Send a reading to the server.
 * @param netIntf The network interface settings to use for the request.
 * @param jsonData The JSON data to send.
 * @param timeinfo The time information to include in the request.
 */
void ServerInfo::sendReading(
    WifiSpec& netIntf,
    const EnvironmentalReading& envdata
) const {

    size_t length = this->host.length() + strlen(Route::READING) + 10;
    char url[length];
    snprintf(url, length, "%s%s", this->host.c_str(), Route::READING);

    debugf("Sending Reading to %s\n", url);
    std::string jsonData = envdata.toJson();
    std::string response = netIntf.sendJson(url, jsonData);
    debugf("Response: %s\n", response.c_str());
}

/**
 * Get the qnh from the server as JSON.
 * @param netInf The network interface settings to use for the request.
 * @return The qnh as a string.
 */
std::string ServerInfo::getQnh(
    WifiSpec& netInf
) const {
    
    size_t length = this->host.length() + strlen(Route::QNH) + 10;
    char url[length];
    snprintf(url, length, "%s%s", this->host.c_str(), Route::QNH);

    debugf("Getting QNH from %s\n", url);
    std::string response = netInf.getJson(url);
    debugf("Response: %s\n", response.c_str());
    return response;
}


/**
 *Get the firmware version from the server as JSON.
 * @param netInf The network interface settings to use for the request.
 * @return The firmware version as a string.
 */
std::string ServerInfo::getFirmwareVersion(WifiSpec& netInf) const {
    size_t length = this->host.length() + strlen(Route::VERSIONING) + 10;
    char url[length];
    snprintf(url, length, "%s%s", this->host.c_str(), Route::VERSIONING);

    debugf("Getting firmware version from %s\n", url);
    std::string response = netInf.getJson(url);
    debugf("Response: %s\n", response.c_str());
    return response;
}

