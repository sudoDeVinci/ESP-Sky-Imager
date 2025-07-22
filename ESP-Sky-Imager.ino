#include <Wire.h>
#include "SHT31D.hpp"


void setup() {
    Serial.begin(115200);
    Wire.begin();

    SHT31 sensor(SHT31_ADDRESS, 400000, true, Wire);
    
    if (sensor.readTempHum()) {
        Serial.print("Temperature: ");
        Serial.print(sensor.getTemperature());
        Serial.println(" °C");
        
        Serial.print("Humidity: ");
        Serial.print(sensor.getHumidity());
        Serial.println(" %");
    } else {
        Serial.println("Failed to read from SHT31 sensor.");
    }
}

void loop(){

}

