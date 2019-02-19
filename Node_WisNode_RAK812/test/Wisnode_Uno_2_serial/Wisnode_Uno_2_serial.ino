#include <String.h>
#include <RAK811.h>
#include <SoftwareSerial.h>

#define WORK_MODE LoRaP2P  //  LoRaWAN or LoRaP2P
#define DEBUG_SERIAL
#define TEST_DISTANCE

#define SOFT_RX 6
#define SOFT_TX 7

SoftwareSerial Serial1(SOFT_RX, SOFT_TX);

// Global variable
String FREQ = "433500000";  // frequency 433-470Mhz --> RAK812

//Config sync word = 0x12 --> P2P
String key = "public_net";
String value = "off";

String period = "10000";
String address = "BB01";
String ACK_PAYLOAD = "04040404040404";
String package = address + ACK_PAYLOAD;

uint8_t cnt_pkg = 100;
 

// Declaring Serial used
RAK811 RAKLoRa(Serial,Serial1);

void setUART()
{
  Serial.begin(9600);
  delay(100);
  Serial.println("UART ready!");
 }
void wisnodeConfig()
{
   //set up serial 1, connecting with WISNODE
    Serial1.begin(115200); 
    delay(100);
    if(RAKLoRa.rk_setWorkingMode(WORK_MODE))
    {
      Serial.println("Set working mode successly");
      if(RAKLoRa.rk_setConfig(key,value))
         {
          Serial.println("Set sync word OK");
         RAKLoRa.rk_initP2P(FREQ,7,0,1,8,14);
         }
    }
}

//*****************************TEST SEND**********************
void setup() 
{  
   #ifdef DEBUG_SERIAL
      setUART();
   #endif
   wisnodeConfig();
   setModeSend();
}
//========================================
//Main void
void loop() {
    wisnodeSend(1, package);
    delay(3000);
}

//*****************************TEST RECEIVE**********************
//void setup() 
//{  
//   #ifdef DEBUG_SERIAL
//      setUART();
//   #endif
//   wisnodeConfig();
//   setModeReceive();
//}
//void loop()
//{
//  String buf = "";
//  while(buf.length() == 0)
//  {
//    buf = wisnodeReceive();
//  }
//  if( buf.substring(0,4) == address )
//  {
//    //Serial.println("///Execute task///");
//  }
//  
//}




//*****************************************************
bool setModeSend()
{
  //Serial.println("-------------Send mode-------------");
  if (!RAKLoRa.rk_stopRecvP2PData())
      //Serial.println("stop receiving fail!"); 
      return false;
  else
      return true;
}
bool wisnodeSend(int cnt_pkg, String mess)
{
  if (RAKLoRa.rk_sendP2PData(cnt_pkg, period, mess))
  {
      Serial.println("Send successfully");
      return true;
  }
  else
  {
      Serial.println("Send fail!");
      return false;
  }   
}
//*********************************************************
bool setModeReceive()
{
   Serial.println("-------------Recv mode-------------");
   if (RAKLoRa.rk_stopSendP2PData())
   {
      RAKLoRa.rk_recvP2PData(1);
      Serial.println("Set receive mode is OK --> Wait for message");
      return true;
   }
   else
   {
      Serial.println("Stop sending fail!");
      return false;
   }
}
String wisnodeReceive()
{
   String sig;
   static uint8_t cnt;
   while(Serial1.available()>0)
   {
    sig = Serial1.readStringUntil('\n');
    cnt++;
    Serial.println(sig);
   }
   if(cnt == cnt_pkg)
   {
    Serial.println("Tranmission Success");
   }
   return sig;
}
//*********************************************************
     
//----------------------------------
// Define time delay function
void Delayms(float delayTime, void (func)()){
    unsigned long endTime = millis() + delayTime;
    while(millis() < endTime)
    {
      func();
      
    }
}

//-----------------------------------
//Buffer data
String store_data(char in_char)
{
   static String data = "";
   data = data + in_char;
   if (in_char == '\n')
      data = "";
   return data;

}
