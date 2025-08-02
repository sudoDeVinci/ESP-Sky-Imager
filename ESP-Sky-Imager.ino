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
    WifiSpec intfce = WifiSpec();
    intfce.wifiConnectionSetup(fs, sense.status);
}


void loop(){
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

