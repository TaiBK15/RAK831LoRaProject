#include <String.h>
#include <RAK811.h>
#include <SoftwareSerial.h>

#define WORK_MODE LoRaP2P  //  LoRaWAN or LoRaP2P
//#define DEBUG_SERIAL
#define TEST_DISTANCE

#define SOFT_RX 6
#define SOFT_TX 7

//Define pin control led
#define latchPin  12
#define clockPin  13
#define dataPin  11

SoftwareSerial Serial1(SOFT_RX, SOFT_TX);
void displayLed7Seg(unsigned long Giatri, byte SoLed = 2);

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

uint8_t cnt = 0;
// Global variable
String FREQ = "433000000";  // frequency 433-470Mhz --> RAK812

//Config sync word = 0x12 --> P2P
String key = "public_net";
String value = "off";

String period = "10000";
String address = "BB01";
String ACK_PAYLOAD = "04040404040404";
String package = address + ACK_PAYLOAD;

uint8_t cnt_pkg = 100;
 

// Declaring Serial used
RAK811 RAKLoRa(Serial1,Serial);

void setUART()
{
  Serial1.begin(9600);
  delay(100);
  Serial1.println("UART ready!");
 }
void wisnodeConfig()
{
  
   //set up serial 1, connecting with WISNODE
    Serial.begin(115200); 
    delay(100);
    if(RAKLoRa.rk_setWorkingMode(WORK_MODE))
    {
//      Serial1.println("Set working mode successly");
      if(RAKLoRa.rk_setConfig(key,value))
         {
//          Serial1.println("Set sync word OK");
         RAKLoRa.rk_initP2P(FREQ,12,0,1,8,14);
         }
    }
}

//*****************************TEST SEND**********************
void setup() 
{ 
   pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
   
   #ifdef DEBUG_SERIAL
      setUART();
   #endif
   wisnodeConfig();
//   setModeSend();
   displayLed7Seg(0,2);
    setModeReceive();
   
}
//========================================
//Main void
void loop() {
//    wisnodeSend(1, package);
    wisnodeReceive();
    displayLed7Seg(cnt,2);
//    delay(3000);
}

//-----------------------------------
void displayLed7Seg(unsigned long Giatri, byte SoLed = 2) {
  
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
//      Serial1.println("Send successfully");
      return true;
  }
  else
  {
//      Serial1.println("Send fail!");
      return false;
  }   
}
//*********************************************************
bool setModeReceive()
{
//   Serial.println("-------------Recv mode-------------");
   if (RAKLoRa.rk_stopSendP2PData())
   {
      RAKLoRa.rk_recvP2PData(1);
//      Serial.println("Set receive mode is OK --> Wait for message");
      return true;
   }
   else
   {
//      Serial.println("Stop sending fail!");
      return false;
   }
}
String wisnodeReceive()
{
   String sig;
   while(Serial.available()>0)
   {
    sig = Serial.readStringUntil('\n');
    cnt++;
//    Serial.println(sig);
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
