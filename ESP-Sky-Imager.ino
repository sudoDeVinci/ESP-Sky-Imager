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
    ServerInfo server = ServerInfo::fromFile(fs);

    intfce.getInternalTime(20);
    server.websiteReachable();
    server.sendStatuses(intfce, sense.status);
    server.sendReading(intfce, reading);
    server.getQnh(intfce);
    server.getFirmwareVersion(intfce);
}


void loop(){
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

