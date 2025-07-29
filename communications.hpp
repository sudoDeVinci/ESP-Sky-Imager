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
    IPAddress GATEWAY;
    IPAddress DNS;

    /**
     * MIME types for the different types of packets.
     */
    struct MIMEType {
    const std::string IMAGE_JPG = "image/jpeg";
    const std::string APP_FORM = "application/x-www-form-urlencoded";
    } mimetypes;

    /**
     * Routes on the Server. 
     */
    struct Route {
        const std::string INDEX = "/";
        const std::string IMAGE = "/api/images";
        const std::string REGISTER = "/api/register";
        const std::string READING = "/api/reading";
        const std::string STATUS = "/api/status";
        const std::string UPDATE = "/api/update";
        const std::string UPGRADE = "/api/upgrade";
        const std::string TEST = "/api/test";
        const std::string QNH = "/api/QNH";
    } routes;

    struct Header {
        const std::string CONTENT_TYPE = "Content-Type";
        const std::string CONTENT_LENGTH = "Content-Length";
        const std::string MACADDRESS = "X-MAC-Address";
        const std::string TIMESTAMP = "X-Timestamp";
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
    std::string certificate;
    WiFiClientSecure *CLIENT;
    tm TIMEINFO;

    /**
     * Set the internal clock of the ESP32 to the current time using NTP AND fill the timeinfo struct with that time.
     * Big thanks to Andreas Spiess.
     * @param timeinfo: tm struct to hold the time information.
     */
    void setClock(tm& timeinfo);

    bool wifiSetup(const &SensorContainer::Status status);   
};