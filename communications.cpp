#include "communications.hpp"


 /**
 * Check if the server is reachable.
 * @param webclient The HTTP client to use for the request.
 * @param netIntf The network interface settings to use for the request.
 * @return true if the server is reachable, false otherwise.
 */
bool ServerInfo::websiteReachable(
    HTTPClient& webclient,
    NetworkInterface& netIntf
) const {
    /**
     * Enough space for https://<host>/<route> + null terminator.
     */
    size_t length = this->host.length() + strlen(Route::INDEX) + 10;
    char url[length];
    snprintf(
        url,
        length,
        "https://%s%s",
        this->host.c_str(),
        Route::INDEX
    );

    webclient.begin(url, this->certificate.c_str());
    const int httpCode = webclient.GET();
    webclient.end();

    if (httpCode == 200) {
        debugln(">> Server is reachable.");
        return true;
    }
    
    debugf(">> Server is not reachable. HTTP code: %d", httpCode);
    return false;
}


std::string sendJson(
    HTTPClient& webclient,
    NetworkInterface& netIntf,
    const std::string& url,
    const std::string& jsonData
) {
    webclient.begin(url.c_str(), netIntf.client);
    webclient.addHeader(ServerInfo::Header::CONTENT_TYPE, contentType);
    webclient.addHeader(ServerInfo::Header::CONTENT_LENGTH, std::to_string(jsonData.length()));

    int httpCode = webclient.POST(jsonData);
    if (httpCode < 0) {
        debugf("Failed to send data: %s\n", webclient.errorToString(httpCode).c_str());
        return "";
    }

    String response = webclient.getString();
    debugf("Response: %s\n", response.c_str());
    
    webclient.end();
    return response.c_str();
}


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
) const {

}






