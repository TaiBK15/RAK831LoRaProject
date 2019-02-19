#include "RAK811.h"
#include <String.h>
#define WORK_MODE LoRaP2P  //  LoRaWAN or LoRaP2P
#define CODE  PING  // PING or PONG
char* data = "11111";
String FREQ = "434000000";  // frequency 433-470Mhz --> RAK812
String ret;
RAK811 RAKLoRa(Serial,Serial1);

void setup() {
  // set up serial 0
  Serial.begin(9600);
  while(Serial.read()>= 0) {}
  while(!Serial);
  Serial.println("StartUP");  
  delay(200);
  Serial1.begin(115200); // Note: Please manually set the baud rate of the WisNode RAK812 device to 115200
  delay(100);
  Serial.println(RAKLoRa.rk_getVersion());
  delay(200);
  Serial.println(RAKLoRa.rk_getBand());
  delay(200);
  
}

void loop() {
  // Set up mode P2P for Wisnode 
    if (RAKLoRa.rk_setWorkingMode(WORK_MODE))
  {
    Serial.println("You set Working mode is OK!");
    //Set up configuration for Wisnode
    if (RAKLoRa.rk_initP2P(FREQ,7,0,1,8,14))
    {
      Serial.println("You init P2P parameter is OK!");
      while (1)
      {
                //Mode Sending
         Serial.println(RAKLoRa.sendRawCommand(F("at+txc=10,100,111111")));
         delay(2000);
         while(Serial1.available()==0)
         {}
         String ret = Serial1.readStringUntil('\n');
         Serial.println(ret);
         delay(10000);

        
      }
    }
  }
}
