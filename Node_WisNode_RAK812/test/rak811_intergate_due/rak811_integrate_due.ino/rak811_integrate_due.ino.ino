
#include <String.h>
#include <RAK811.h>

#define WORK_MODE LoRaP2P  //  LoRaWAN or LoRaP2P
//#define DEBUG_SERIAL
#define TEST_DISTANCE

// Global variable
String FREQ = "433500000";  // frequency 433-470Mhz --> RAK812

//Config sync word = 0x12 --> P2P
String key = "public_net";
String value = "off";

String period = "10000";
String address = "BB01";
String DATA_PAYLOAD = "04040404040404";
String package = address + DATA_PAYLOAD;
 

// Declaring Serial used
RAK811 RAKLoRa(Serial);

void wisnodeConfig()
{
   //set up serial 1, connecting with WISNODE
    Serial.begin(115200); 
    delay(100);
    if(RAKLoRa.rk_setWorkingMode(WORK_MODE))
    {
      if(RAKLoRa.rk_setConfig(key,value))
         {
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
  delay(2000);
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
      //Serial.println("Send successfully");
      return true;
  }
  else
  {
      //Serial.println("Send fail!");
      return false;
  }   
}
//*********************************************************
bool setModeReceive()
{
   //Serial.println("-------------Recv mode-------------");
   if (RAKLoRa.rk_stopSendP2PData())
   {
      RAKLoRa.rk_recvP2PData(1);
      ////Serial.println("Set receive mode is OK --> Wait for message");
      return true;
   }
   else
   {
      //Serial.println("Stop sending fail!");
      return false;
   }
}
String wisnodeReceive()
{
   String sig;
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

