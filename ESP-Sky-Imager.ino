#include <Wire.h>
#include "SHT31D.hpp"
#include "MPU6050.hpp"

#define BUS0_SDA 41
#define BUS0_SCL 42


MPU_XYZ gyroreading;
MPU6050 mpu(0, BUS0_SDA, BUS0_SCL);
SHT31 sht(0, BUS0_SDA, BUS0_SCL);


void setup() {
    Serial.begin(115200);
    
    if (!mpu.init()) {
        Serial.println("Failed to initialize MPU6050!");
        while (1) { delay(1000); }
    } else {
        Serial.println("MPU6050 initialized successfully.");
    }

    if (!sht.init()) {
        Serial.println("Failed to initialize SHT31D!");
        while (1) { delay(1000); }
    } else {
        Serial.println("SHT31D initialized successfully.");
    }
}


void loop(){
    gyroreading = mpu.readGyro();
    Serial.printf("Gyro X: %.2f, Y: %.2f, Z: %.2f\n", gyroreading[0], gyroreading[1], gyroreading[2]);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
