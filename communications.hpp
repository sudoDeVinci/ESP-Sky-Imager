#pragma once

#include "io.hpp"
#include "SensorContainer.hpp"
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <vector>
#include <array>
#include "FS.h"
#include <cstdint>
#include <string>
#include <time.h>

#define CONN_TIMEOUT 10000
#define READ_TIMEOUT 5000
#define RETRY_COUNT 30
#define CLRF "\r\n"


struct WifiSpec {
    bool isConnected = false;
    std::string ssid;
    std::string password;
    tm timeinfo;

    WifiSpec(
        const std::string& ssid = "",
        const std::string& password = ""
    ): ssid(ssid), password(password){}


    /**
     * Check if two structs are equal.
     */
    bool operator==(const WifiSpec& other) const {
        return (this->ssid == other.ssid &&
                this->password == other.password
            );
    }

    inline bool connected(void) const {
        return this->isConnected;
    }


    /**
     * Set the internal clock of the ESP32 to the current time using NTP AND fill the timeinfo struct with that time.
     * Big thanks to Andreas Spiess.
     * @param timeinfo: tm struct to hold the time information.
     */
    void setClock(void);


    /**
     * Get the current time from the onboard clock.
     * This function will keep trying to get the time until it succeeds or the timer runs out.
     * It will print a dot every 150-250 milliseconds to indicate progress.
     * If the time is not set after the timer runs out, it will print an error message.
     * @param timeinfo: tm struct to hold the time information.
     * @param timer: The maximum time to wait for the time to be set, in seconds.
     * @return true if the time was successfully set, false otherwise.
     */
    bool getInternalTime(int timer);


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
    bool connect(
        const std::string& ssid,
        const std::string& password,
        SensorContainer::Status& status,
        uint8_t retryCount
    );


    bool wifiConnectionSetup(fs::FS& fs, SensorContainer::Status& status);   


    /**
     * Send a JSON string to the server.
     * @param webclient The HTTP client to use for the request.
     * @param url The URL to send the JSON data to.
     * @param jsonData The JSON data to send.
     * @return The response from the server as a string.
     */
    std::string sendJson(
        const std::string& url,
        const std::string& jsonData,
        const std::string& timestamp
    ) const;


    std::string getJson(
        const std::string& url
    ) const;
};

struct ServerInfo {
    const std::string host;
    const std::string certificate;
    const std::string apikey;

    ServerInfo(
        const std::string& host = "",
        const std::string& certificate = "",
        const std::string& apikey = ""
    ) : certificate(certificate), 
        apikey(apikey),
        host(host) {
    }

    /**
     * MIME types for the different types of packets.
     */
    struct MIMEType {
        static constexpr const char* IMAGE_JPG = "image/jpeg";
        static constexpr const char* APP_FORM = "application/x-www-form-urlencoded";
        static constexpr const char* APP_JSON = "application/json";
        static constexpr const char* APP_OCTET_STREAM = "application/octet-stream";
        static constexpr const char* TEXT_PLAIN = "text/plain";
    };

    /**
     * Routes on the Server. 
     */
    struct Route {
        static constexpr const char* INDEX = "/api/check";
        static constexpr const char* IMAGE = "/api/image";
        static constexpr const char* REGISTER = "/api/register";
        static constexpr const char* READING = "/api/reading";
        static constexpr const char* STATUS = "/api/status";
        static constexpr const char* UPDATE = "/api/update";
        static constexpr const char* VERSION = "/api/version";
        static constexpr const char* QNH = "/api/qnh";
    };

    struct Header {
        static constexpr const char* CONTENT_TYPE = "Content-Type";
        static constexpr const char* CONTENT_LENGTH = "Content-Length";
        static constexpr const char* MACADDRESS = "X-MAC-Address";
        static constexpr const char* TIMESTAMP = "X-Timestamp";
    };

    /**
     * Check if the server is reachable.
     * @param webclient The HTTP client to use for the request.
     * @param netIntf The network interface settings to use for the request.
     * @return true if the server is reachable, false otherwise.
     */
    bool websiteReachable(void) const;

    /**
     * Send the status of the sensors to the server.
     * @param webclient The HTTP client to use for the request.
     * @param netIntf The network interface settings to use for the request.
     * @param status The status of the sensors to send.
     */
    void sendStatuses(
        WifiSpec& netIntf,
        const SensorContainer::Status& status,
        const tm& timeinfo 
    ) const;

    /**
     * Send a reading to the server.
     * @param netIntf The network interface settings to use for the request.
     * @param jsonData The JSON data to send.
     * @param timeinfo The time information to include in the request.
     */
    void sendReading(
        WifiSpec& netIntf,
        const EnvironmentalReading& envdata,
        const tm& timeinfo
    ) const;

    /**
     * Get the QNH value from the server as JSON.
     * @param netInf The network interface settings to use for the request.
     * @return The QNH value as a string.
     */
    std::string getQnh(WifiSpec& netInf) const;

    /**
     * Get the firmware version from the server as JSON.
     * @param netInf The network interface settings to use for the request.
     * @return The firmware version as a string.
     */
    std::string getFirmwareVersion(WifiSpec& netInf) const;
};



