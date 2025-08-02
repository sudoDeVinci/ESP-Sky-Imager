#include "io.hpp"
#include "communications.hpp"
#include "SensorContainer.hpp"

void setup() {
    Serial.begin(115200);
    while(!Serial) {
        delay(100);
    }

    debugln("Starting ESP-Sky-Imager...");

    fs::FS& fs = determineFileSystem();
    
    SensorContainer sense = SensorContainer();
    EnvironmentalReading reading = EnvironmentalReading();

    WifiSpec intfce = WifiSpec();
    intfce.wifiConnectionSetup(fs, sense.status);

    // we should be connected to WiFi now
    intfce.setClock();
    const std::string host = "http://192.168.0.33:5000";
    const std::string cert = "NAN";
    const std::string apikey = "NAN";

    ServerInfo server = ServerInfo(host, cert, apikey);

    intfce.getInternalTime(20);
    server.websiteReachable();
    server.sendStatuses(intfce, sense.status, intfce.timeinfo);
    server.sendReading(intfce, reading, intfce.timeinfo);
    server.getQnh(intfce);
    server.getFirmwareVersion(intfce);
}


void loop(){
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

