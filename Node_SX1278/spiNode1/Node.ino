//#include <SPI.h>
//#include <LoRa.h>
//
//void setup() {
//  /* Setup Serial */
//  Serial.begin(9600);
//  while (!Serial);
//
//  /* Setup Frequency */
//  if (!LoRa.begin(433.1E6, Serial)) {
//    Serial.println("Starting LoRa failed!");
//    while (1);
//  }
//
//  /* Configure LoRa packets */
//  LoRa.setSpreadingFactor(7);
//  LoRa.setSignalBandwidth(125E3);
//  LoRa.setCodingRate4(5);
//  LoRa.setPreambleLength(8);
//  LoRa.setSyncWord(0x34);
//  LoRa.enableCrc();
//  Serial.println("Setting done!");
//
//  /* Register the receive callback */
//  LoRa.onReceive(onReceive);
//
//  /* Put the radio into receive mode */
//  LoRa.receive();
//}
//
///* loop: Tx Packets */
//void loop() {
//  delay(5000);
//  LoRa.idle();
//  Serial.println("Sending packet to gateway... ");
//  uint8_t fake_data[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x0A, 0x09};
//  // send packet
//  LoRa.beginPacket();
//  LoRa.write(fake_data, 10);
//  LoRa.endPacket();
//  LoRa.receive();
//}
//
///* Handle received packets */
//void onReceive(int packetSize) {
//  // received a packet
//  Serial.print("Received packet '");
//  // read packet
//  for (int i = 0; i < packetSize; i++) {
//    Serial.print((char)LoRa.read());
//  }
//  // print RSSI of packet
//  Serial.print("' with RSSI ");
//  Serial.println(LoRa.packetRssi());
//}
