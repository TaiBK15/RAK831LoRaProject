#include <SPI.h>
#include <LoRa.h>

uint8_t counter = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Sender");

  // SET FREQUENCY
  if (!LoRa.begin(433E6, Serial)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // CONFIGURATION
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.setSyncWord(0x34);
  LoRa.enableCrc();
  Serial.println("Setting done!");
}

void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);
  uint8_t buffer_x[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x0A, counter};
  // send packet
  LoRa.beginPacket();
  LoRa.write(buffer_x, 10);
  LoRa.endPacket();

  counter++;

    delay(5000);
}
