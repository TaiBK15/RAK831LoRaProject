#include <Wire.h>
#include <BH1750FVI.h>

BH1750FVI LightSensor;

void setup() {
    Serial.begin(9600);
    LightSensor.begin();
    LightSensor.SetAddress(Device_Address_L);
    LightSensor.SetMode(Continuous_H_resolution_Mode);
    Serial.println("Running...");
}

void loop() {
  uint16_t lux = LightSensor.GetLightIntensity();// Get Lux value
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lux");
  delay(1000);
}