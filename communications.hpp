#pragma once

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
#define CLRF "\r\n"

struct ServerInfo {
    const std::string host;
    const std::string certificate;

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
        static constexpr const char* INDEX = "/";
        static constexpr const char* IMAGE = "/api/images";
        static constexpr const char* REGISTER = "/api/register";
        static constexpr const char* READING = "/api/reading";
        static constexpr const char* STATUS = "/api/status";
        static constexpr const char* UPDATE = "/api/update";
        static constexpr const char* UPGRADE = "/api/upgrade";
        static constexpr const char* TEST = "/api/test";
        static constexpr const char* QNH = "/api/QNH";
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
    bool websiteReachable(HTTPClient& webclient, NetworkInterface& netIntf) const;

    /**
     * Send the status of the sensors to the server.
     * @param webclient The HTTP client to use for the request.
     * @param netIntf The network interface settings to use for the request.
     * @param status The status of the sensors to send.
     */
    void sendStatuses(
        HTTPClient& webclient,
        NetworkInterface& netIntf,
        const SensorContainer::Status& status
    ) const;

    /**
     * Send the readings to the server.
     * @param webclient The HTTP client to use for the request.
     * @param netIntf The network interface settings to use for the request.
     * @param reading The environmental reading to send.
     * @param status The status of the sensors to send.
     */
    void sendReadings(
        HTTPClient& webclient,
        NetworkInterface& netIntf,
        const EnvironmentalReading& reading,
        const SensorContainer::Status& status
    ) const;
};


struct NetworkInterface {
    std::string ssid;
    std::string password;
    WiFiClientSecure *client;
    tm TIMEINFO;
    IPAddress gateway;
    IPAddress DNS;

    /**
     * Set the internal clock of the ESP32 to the current time using NTP AND fill the timeinfo struct with that time.
     * Big thanks to Andreas Spiess.
     * @param timeinfo: tm struct to hold the time information.
     */
    void setClock(tm& timeinfo);

    bool wifiSetup(const &SensorContainer::Status status);   
};