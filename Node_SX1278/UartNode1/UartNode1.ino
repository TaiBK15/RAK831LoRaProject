#include <SoftwareSerial.h>
#include <TimerOne.h>
//#define DEBUG_SERIAL

#define HEADER          0xC0
#define NODE_AddressH   0xBB
#define NODE_AddressL   0x11
#define NODE_Channel    0x17

#define RELAY_NODE_ADDR_H   0xBB
#define RELAY_NODE_ADDR_L   0x33
#define RELAY_NODE_CHANNEL  0x17

#define SPEED           0x1D  // 19.2 KBPS
#define OPTION          0xC4  //20dBm

#define GW_AddressH     0x05
#define GW_AddressL     0x01
#define GW_Channel      0x17

#define M0pin   A0
#define M1pin   A1
#define AUXpin  A2
#define SOFT_RX 7
#define SOFT_TX 6
#define TIMEOUT_RET   2

#define LED_TX_PIN   3
#define LED_RX_PIN   5

SoftwareSerial LoraSerial(SOFT_RX, SOFT_TX);

void setup() {
   

    // Led rx,tx 
    ledConfig();

    //Serial Init
    #ifdef DEBUG_SERIAL
    Serial.begin(9600);
    Serial.println("Serial are ready");
    #endif // DEBUG_SERIAL

    // uart lora config
    if (!e32Config()) while(1);
    
      //Timer Setting
    Timer1.initialize(10000000); //5s 1 time
    Timer1.attachInterrupt(timerIsr);
    Serial.println("Timer1 started!");
}
/*-----------------------------------------------
    LOOP
------------------------------------------------*/
void loop() {
    uint8_t data_len = 0;
    uint8_t data[data_len];
    while (data_len == 0)
        data_len = e32Listen();
    if (data_len > 0) {
        ledToggle(1, false, true, 500);
        e32Fetch(data_len, data);
    }
}
/*-----------------------------------------------
    timerIsr
------------------------------------------------*/
void timerIsr(){
    uint8_t FAKE_PAYLOAD[] = {RELAY_NODE_ADDR_H, RELAY_NODE_ADDR_L, RELAY_NODE_CHANNEL,NODE_AddressH, NODE_AddressL, 0xBB, 0xCC};
    e32Send(FAKE_PAYLOAD, sizeof(FAKE_PAYLOAD));
}
/*-----------------------------------------------
    FUNCTIONS
-------------------------------------------------*/
bool AUX_HL;
bool ReadAUX() {
    int val = analogRead(AUXpin);
    if(val<50){
        AUX_HL = LOW;
    }else {
        AUX_HL = HIGH;
    }
    return AUX_HL;
}

int WaitAUX_H() {
    int ret_var = 0;
    uint8_t cnt = 0;
    while ((ReadAUX() == LOW) && (cnt++ < 100)){
        #ifdef DEBUG_SERIAL
        Serial.print(".");
        #endif // DEBUG_SERIAL
        delay(100);
    }
    if (cnt >= 100 ) {
        ret_var = TIMEOUT_RET;
        #ifdef DEBUG_SERIAL
        Serial.println("Timeout!");
        #endif // DEBUG_SERIAL
    }
    return ret_var;
}

void ledConfig() {
    pinMode(LED_TX_PIN, OUTPUT);
    pinMode(LED_RX_PIN, OUTPUT);
}

void ledToggle(uint8_t count, bool ledTx, bool ledRx, int delayms) {
    for(int i=0; i<count; i++){
        if (ledTx) digitalWrite(LED_TX_PIN, HIGH);
        if (ledRx) digitalWrite(LED_RX_PIN, HIGH);
        delay(delayms);
        if (ledTx) digitalWrite(LED_TX_PIN, LOW);
        if (ledRx) digitalWrite(LED_RX_PIN, LOW);    
        delay(delayms);
    }
}

bool e32Config() {
    // pin Mode
    pinMode(M0pin, OUTPUT);
    pinMode(M1pin, OUTPUT);
    pinMode(AUXpin, INPUT);
    LoraSerial.begin(9600);

    // configure
    digitalWrite(M0pin, HIGH);
    digitalWrite(M1pin, HIGH);
    delay(10);
    if(WaitAUX_H() == TIMEOUT_RET) {exit(0);}
    uint8_t CMD[6] = {HEADER, NODE_AddressH, NODE_AddressL, SPEED, NODE_Channel, OPTION};
    LoraSerial.write(CMD, 6);
    WaitAUX_H();
    delay(1200);
    #ifdef DEBUG_SERIAL
    Serial.println("Setting Configure has been sent!");
    #endif // DEBUG_SERIAL

    //Read Set configure
    while(LoraSerial.available()){LoraSerial.read();} //Clean Uart Buffer
    uint8_t READCMD[3] = {0xC1, 0xC1, 0xC1};
    LoraSerial.write(READCMD, 3);
    WaitAUX_H();
    delay(50);
    #ifdef DEBUG_SERIAL
    Serial.println("Reading Configure has been sent!");
    #endif // DEBUG_SERIAL
    uint8_t readbuf[6];
    uint8_t Readcount, idx, buff;
    Readcount = LoraSerial.available();
    if (Readcount){
        #ifdef DEBUG_SERIAL
        Serial.println("Setting Configure is:  ");
        #endif // DEBUG_SERIAL
        for(idx = 0; idx < Readcount; idx++) {
            readbuf[idx] = LoraSerial.read();
            delay(10);
        }
        #ifdef DEBUG_SERIAL
        Serial.print("Shutdown Mode: ");Serial.println(0xFF & readbuf[0],HEX);
        Serial.print("Address High: ");Serial.println(0xFF & readbuf[1],HEX);
        Serial.print("Address Low: ");Serial.println(0xFF & readbuf[2],HEX);
        Serial.print("Speed: ");Serial.println(0xFF & readbuf[3],HEX);
        Serial.print("Channel: ");Serial.println(0xFF & readbuf[4],HEX);
        Serial.print("Option: ");Serial.println(0xFF & readbuf[5],HEX);
        #endif // DEBUG_SERIAL

        if ((readbuf[0] == HEADER) && (readbuf[1] == NODE_AddressH ) && (readbuf[2] == NODE_AddressL )  
                      && (readbuf[3] == SPEED) && (readbuf[4] == NODE_Channel ) && (readbuf[5] == OPTION) ) {
            ledToggle(3, true, true, 500);
            digitalWrite(M0pin, LOW);
            digitalWrite(M1pin, LOW);
            #ifdef DEBUG_SERIAL
            Serial.println("---------------SETTING DONE-------------");
            #endif // DEBUG_SERIAL
            return true;
        }
    }
    return false;
}

int e32Listen() {
    delay(100);
    int data_len;
    data_len = LoraSerial.available();
    if (data_len > 0) {
        #ifdef DEBUG_SERIAL
        Serial.print("ReceiveMsg: ");  Serial.print(data_len);  Serial.println(" bytes.");
        #endif // DEBUG_SERIAL
        ledToggle(1, false, true, 500);
    }
    return data_len;
}

void e32Fetch(int data_len, uint8_t *pdata) {
    if (data_len > 0) {
        for(int i=0;i<data_len;i++) {
            pdata[i] = LoraSerial.read();
            #ifdef DEBUG_SERIAL
            Serial.print(pdata[i]);
            Serial.print(" ");
            #endif // DEBUG_SERIAL
        }
        #ifdef DEBUG_SERIAL
        Serial.println("");
        #endif // DEBUG_SERIAL
    }
}

void e32Send(uint8_t *payload, uint8_t nb_byte) {
    // uint8_t FrameSend[8] = {GW_AddressH, GW_AddressL, GW_Channel, NODE_AddressH, NODE_AddressL, 0x01, 0x02, 0x03};
    LoraSerial.write(payload, nb_byte);
    #ifdef DEBUG_SERIAL
    Serial.println("Data hasbeen sent!");
    #endif // DEBUG_SERIAL
    ledToggle(1, true, false, 500);
    delay(10);

}
