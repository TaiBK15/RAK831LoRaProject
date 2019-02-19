#include <SPI.h>
#include <LoRa.h>

//Pin control led 7 seg
#define latchPin  A1
#define clockPin  A0
#define dataPin  A2

void Display_LED_7_seg(unsigned long Giatri, byte SoLed = 2);

/*Array 10 digits from 0 - 9
  for LED CATHODE COMMON*/
const byte Seg[10] = {
  0x3F,//0 - các thanh từ a-f sáng
  0x06,//1 - chỉ có 2 thanh b,c sáng
  0x5B,//2
  0x4F,//3
  0x66,//4
  0x6D,//5
  0x7D,//6
  0x07,//7
  0x7F,//8
  0x6F,//9
};

static unsigned long pkg_num = 0;
  
void setup() {
  //Init control-led Pin
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Receiver");

  // SET FREQUENCY
  if (!LoRa.begin(433.5E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  // CONFIGURATION
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setPreambleLength(8);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  Serial.println("Setting done!");
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    //Increase package receive number
    pkg_num = pkg_num + 1;
    Display_LED_7_seg(pkg_num, 2);

    // read packet
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}

void Display_LED_7_seg(unsigned long Giatri, byte SoLed = 2) {
  
  byte *array= new byte[SoLed];
  for (byte i = 0; i < SoLed; i++) {
    //Lấy các chữ số từ phải quá trái
    array[i] = (byte)(Giatri % 10UL);
    Giatri = (unsigned long)(Giatri /10UL);
  }
  digitalWrite(latchPin, LOW);
  for (int i = SoLed - 1; i >= 0; i--)
    shiftOut(dataPin, clockPin, MSBFIRST, Seg[array[i]]); 
  
  digitalWrite(latchPin, HIGH);
  free(array);
}
