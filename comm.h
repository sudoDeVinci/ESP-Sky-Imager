#ifndef COMM_H
#define COMM_H

#include "sensors.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <map>

#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1

/**
 * Timeout in milliseconds for connecting to / reading from server.
*/
#define CONN_TIMEOUT 10000
#define READ_TIMEOUT 5000

/**
 * EOL chars used. 
 */
#define CLRF "\r\n"

/**
 * Struct to hold network details in contiguous memory.
 * Many details are read from config files. 
 */
struct NetworkInfo {
  const char* SSID;
  const char* PASS;
  const char* const CERT = R"(
    -----BEGIN CERTIFICATE-----
    MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
    TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
    cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
    WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
    ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
    MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
    h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
    0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
    A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
    T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
    B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
    B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
    KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
    OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
    jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
    qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
    rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
    HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
    hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
    ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
    3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
    NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
    ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
    TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
    jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
    oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
    4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
    mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
    emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
    -----END CERTIFICATE-----
    )";

  const char* const HOST = "https://devinci.cloud";
  IPAddress GATEWAY;
  IPAddress DNS;
  WiFiClientSecure *CLIENT;
  tm TIMEINFO;

 /**
  * MIME types for the different types of packets.
  */
  struct MIMEType {
    const char* const IMAGE_JPG = "image/jpeg";
    const char* const APP_FORM = "application/x-www-form-urlencoded";
  } mimetypes;

  /**
   * Routes on the Server. 
   */
  struct Route {
    const char* const INDEX = "/";
    const char* const IMAGE = "/api/images";
    const char* const REGISTER = "/api/register";
    const char* const READING = "/api/reading";
    const char* const STATUS = "/api/status";
    const char* const UPDATE = "/api/update";
    const char* const UPGRADE = "/api/upgrade";
    const char* const TEST = "/api/test";
    const char* const QNH = "/api/QNH";
  } routes;

  /**
   * Headers for the HTTP requests.
   */
  struct Header {
    const char* const CONTENT_TYPE = "Content-Type";
    const char* const MAC_ADDRESS = "MAC-Address";
    const char* const TIMESTAMP = "timestamp"; 
  } headers;
};

/**
 * Set the internal clock of the ESP32 to the current time.
 */
void setClock(tm *timeinfo);

/**
 * Format the timestamp as MySQL DATETIME.
 * If the year is 1970, return "None".
 * 
 * @param timeinfo: tm struct within global Network struct to store the time information.
 * @return char*: timestamp in MySQL DATETIME format.
 */
char* formattime(tm* timeinfo);

/**
  * Get the current time and format the timestamp as MySQL DATETIME.
  * timeinfo is an empty struct whihc is filled by calling getLocalTime().
  * Big thanks to Andreas Spiess:
  * https://github.com/SensorsIot/NTP-time-for-ESP8266-and-ESP32/blob/master/NTP_Example/NTP_Example.ino
  *
  *  If tm_year is not equal to 0xFF, it is assumed that valid time information has been received.
  */
void getTime(tm *timeinfo, int timer);

/**
 * Check if the current time is between 5 PM and 6 AM.
 * If so, enter deep sleep mode until 6 AM.
 */
void checkAndSleep(tm *timeinfo);

/**
 * Connect to wifi Network and apply SSL certificate.
 * Attempt to match the SSID of nearby netwokrs with an SSID in the networkInfo file.
 * If a match is found, connect to the network and apply the SSL certificate.
 */
bool wifiSetup(NetworkInfo* network, Sensors::Status *stat);

/**
 * Check if the website is reachable before trying to communicate further.
 */
bool websiteReachable(HTTPClient* https, NetworkInfo* network, const char* timestamp);

/**
 * Send statuses of sensors to HOST on specified PORT. 
 */
void sendStats(HTTPClient* https, NetworkInfo* network, Sensors::Status *stat, const char* timestamp);

/**
 * Send readings from weather sensors to HOST on specified PORT. 
 */
void sendReadings(HTTPClient* https, NetworkInfo* network, Reading* readings);

/**
 * Send image from weather station to server. 
 */
void sendImage(HTTPClient* https, NetworkInfo* network, uint8_t* buf, size_t len, const char* timestamp);

/**
 * Parse the QNH from the server response.
 */
double parseQNH(const char* json);

/**
 * Get the Sea Level Pressure from the server.
*/
double getQNH(NetworkInfo* network);

/**
 * Update the board firmware via the update server.
 */
void OTAUpdate(NetworkInfo* network, const String& firmware_version);


#endif // COMM_H
